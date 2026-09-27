'use client';

import React, { useState, useEffect } from 'react';
import { X, Plus, Trash2, Download, Upload, Cloud, Check, Clock } from 'lucide-react';
import { DeviceConfig, ScheduleSlot } from '@/lib/types';
import { mqttService } from '@/lib/mqtt-client';
import { supabase } from '@/lib/supabase';

interface ScheduleModalProps {
  device: DeviceConfig | null;
  onClose: () => void;
}

export const ScheduleModal: React.FC<ScheduleModalProps> = ({ device, onClose }) => {
  const [slots, setSlots] = useState<ScheduleSlot[]>([]);
  const [statusMsg, setStatusMsg] = useState<string>('');
  const [isLoading, setIsLoading] = useState<boolean>(false);

  useEffect(() => {
    if (!device) return;

    // 1. Thử tải lịch từ Supabase hoặc localStorage
    const saved = localStorage.getItem(`vbox_sched_${device.key}`);
    if (saved) {
      try {
        setSlots(JSON.parse(saved));
      } catch (e) {
        // Fallback mặc định
      }
    } else {
      // Mặc định tạo 1 khung mẫu
      setSlots([
        { start: '07:00', stop: '07:15', enable: true },
        { start: '11:30', stop: '11:45', enable: true },
        { start: '17:00', stop: '17:20', enable: true },
      ]);
    }

    // 2. Lắng nghe phản hồi lịch thực tế từ V-Box
    const unsub = mqttService.onSchedules((devKey, incomingSlots) => {
      if (devKey === device.key || devKey === 'all') {
        if (incomingSlots && incomingSlots.length > 0) {
          setSlots(incomingSlots);
          showStatus(`Đã nhận ${incomingSlots.length} khung giờ thực tế từ V-Box`);
        }
      }
    });

    // 3. Tự động gửi lệnh đọc từ V-Box
    mqttService.queryDeviceSchedules(device.key);

    return () => unsub();
  }, [device]);

  if (!device) return null;

  const showStatus = (msg: string) => {
    setStatusMsg(msg);
    setTimeout(() => setStatusMsg(''), 4000);
  };

  const handleAddSlot = () => {
    if (slots.length >= device.maxSlots) {
      alert(`Thiết bị này hỗ trợ tối đa ${device.maxSlots} khung giờ.`);
      return;
    }
    setSlots([
      ...slots,
      { start: '08:00', stop: '08:15', enable: true },
    ]);
  };

  const handleRemoveSlot = (index: number) => {
    setSlots(slots.filter((_, i) => i !== index));
  };

  const handleUpdateSlot = (index: number, field: keyof ScheduleSlot, val: any) => {
    const updated = [...slots];
    updated[index] = { ...updated[index], [field]: val };
    setSlots(updated);
  };

  // Nạp lịch xuống V-Box qua MQTT
  const handleDeployToVbox = () => {
    // Lưu vào LocalStorage
    localStorage.setItem(`vbox_sched_${device.key}`, JSON.stringify(slots));

    // Gửi qua MQTT
    const ok = mqttService.setDeviceSchedules(device.key, slots);
    if (ok) {
      showStatus(`Đã nạp ${slots.length} khung giờ xuống V-Box & PLC!`);
    } else {
      showStatus('Lỗi: Chưa kết nối MQTT Broker!');
    }
  };

  // Đọc danh sách thực tế từ V-Box
  const handleFetchFromVbox = () => {
    setIsLoading(true);
    mqttService.queryDeviceSchedules(device.key);
    setTimeout(() => setIsLoading(false), 1500);
  };

  // Lưu lên Supabase
  const handleSaveToCloud = async () => {
    if (!supabase) {
      showStatus('Supabase chưa cấu hình URL/Key. Đã lưu bộ nhớ cục bộ.');
      localStorage.setItem(`vbox_sched_${device.key}`, JSON.stringify(slots));
      return;
    }

    try {
      setIsLoading(true);
      // Xóa các slot cũ của device này
      await supabase.from('device_schedules').delete().eq('device_key', device.key);

      // Thêm danh sách mới
      const records = slots.map((s, idx) => {
        const [sh, sm] = s.start.split(':').map(Number);
        const [eh, em] = s.stop.split(':').map(Number);
        return {
          device_key: device.key,
          slot_index: idx,
          enabled: s.enable !== false,
          start_time: s.start,
          stop_time: s.stop,
          start_h: sh || 0,
          start_m: sm || 0,
          end_h: eh || 0,
          end_m: em || 0,
          day: s.day || 0,
          month: s.month || 0,
        };
      });

      await supabase.from('device_schedules').insert(records);
      showStatus('Đã đồng bộ lịch lên đám mây Supabase!');
    } catch (err: any) {
      showStatus(`Lỗi lưu Supabase: ${err?.message}`);
    } finally {
      setIsLoading(false);
    }
  };

  return (
    <div className="fixed inset-0 z-50 bg-black/50 backdrop-blur-sm flex items-center justify-center p-4">
      <div className="bg-white rounded-2xl max-w-2xl w-full max-h-[90vh] flex flex-col shadow-2xl border border-slate-200 animate-in fade-in zoom-in-95">
        {/* Header Modal */}
        <div className="p-5 border-b border-slate-200 flex items-center justify-between">
          <div>
            <div className="flex items-center gap-2">
              <Clock className="w-5 h-5 text-sky-600" />
              <h3 className="text-base font-bold text-slate-900">
                Lịch Hẹn Thời Gian Thực: {device.name}
              </h3>
            </div>
            <p className="text-xs text-slate-500 mt-0.5">
              Hỗ trợ tối đa <b>{device.maxSlots} khung giờ</b> trong ngày • Tự động kích hoạt theo đồng hồ RTC V-Box
            </p>
          </div>

          <button
            onClick={onClose}
            className="p-2 text-slate-400 hover:text-slate-700 hover:bg-slate-100 rounded-lg transition-colors"
          >
            <X className="w-5 h-5" />
          </button>
        </div>

        {/* Status message */}
        {statusMsg && (
          <div className="mx-5 mt-3 p-2.5 bg-emerald-50 text-emerald-800 text-xs font-semibold rounded-lg border border-emerald-200 flex items-center gap-2">
            <Check className="w-4 h-4 text-emerald-600" />
            <span>{statusMsg}</span>
          </div>
        )}

        {/* Body list of slots */}
        <div className="p-5 overflow-y-auto flex-1 space-y-3">
          {slots.length === 0 ? (
            <div className="text-center py-10 text-slate-400 text-xs">
              Chưa có khung giờ nào được cài đặt. Hãy bấm nút <b>Thêm Khung Giờ</b> bên dưới.
            </div>
          ) : (
            slots.map((slot, idx) => (
              <div
                key={idx}
                className="flex flex-wrap items-center justify-between gap-3 p-3 bg-slate-50 border border-slate-200 rounded-xl"
              >
                <div className="flex items-center gap-2">
                  <span className="w-6 h-6 rounded-full bg-slate-200 text-slate-700 font-bold text-xs flex items-center justify-center">
                    {idx + 1}
                  </span>
                  <label className="text-xs font-semibold text-slate-600">Bật:</label>
                  <input
                    type="time"
                    value={slot.start}
                    onChange={(e) => handleUpdateSlot(idx, 'start', e.target.value)}
                    className="px-2 py-1 bg-white border border-slate-300 rounded text-xs font-bold text-slate-800 focus:outline-sky-500"
                  />
                  <span className="text-slate-400">&rarr;</span>
                  <label className="text-xs font-semibold text-slate-600">Tắt:</label>
                  <input
                    type="time"
                    value={slot.stop}
                    onChange={(e) => handleUpdateSlot(idx, 'stop', e.target.value)}
                    className="px-2 py-1 bg-white border border-slate-300 rounded text-xs font-bold text-slate-800 focus:outline-sky-500"
                  />
                </div>

                <div className="flex items-center gap-3">
                  <label className="flex items-center gap-1.5 text-xs text-slate-600 cursor-pointer">
                    <input
                      type="checkbox"
                      checked={slot.enable !== false}
                      onChange={(e) => handleUpdateSlot(idx, 'enable', e.target.checked)}
                      className="rounded border-slate-300 text-sky-600 focus:ring-sky-500"
                    />
                    <span>Kích hoạt</span>
                  </label>

                  <button
                    onClick={() => handleRemoveSlot(idx)}
                    className="p-1.5 text-rose-500 hover:text-rose-700 hover:bg-rose-50 rounded transition-colors"
                    title="Xóa khung giờ này"
                  >
                    <Trash2 className="w-4 h-4" />
                  </button>
                </div>
              </div>
            ))
          )}

          {slots.length < device.maxSlots && (
            <button
              onClick={handleAddSlot}
              className="w-full py-2.5 border-2 border-dashed border-slate-300 hover:border-sky-500 text-slate-600 hover:text-sky-600 text-xs font-bold rounded-xl flex items-center justify-center gap-1.5 transition-colors"
            >
              <Plus className="w-4 h-4" />
              <span>Thêm Khung Giờ Mới ({slots.length}/{device.maxSlots})</span>
            </button>
          )}
        </div>

        {/* Footer Actions */}
        <div className="p-4 border-t border-slate-200 bg-slate-50 flex flex-wrap items-center justify-between gap-2 rounded-b-2xl">
          <div className="flex items-center gap-2">
            <button
              onClick={handleFetchFromVbox}
              disabled={isLoading}
              className="px-3 py-2 bg-white hover:bg-slate-100 text-slate-700 text-xs font-semibold rounded-lg border border-slate-300 flex items-center gap-1.5 transition-colors"
              title="Đọc các khung giờ thực tế đang lưu trong bộ nhớ V-Box"
            >
              <Download className="w-3.5 h-3.5 text-slate-500" />
              <span>Đọc Từ V-Box</span>
            </button>

            <button
              onClick={handleSaveToCloud}
              disabled={isLoading}
              className="px-3 py-2 bg-white hover:bg-slate-100 text-slate-700 text-xs font-semibold rounded-lg border border-slate-300 flex items-center gap-1.5 transition-colors"
              title="Lưu trữ bản sao lịch hẹn lên Supabase Cloud Database"
            >
              <Cloud className="w-3.5 h-3.5 text-purple-600" />
              <span>Lưu Cloud</span>
            </button>
          </div>

          <div className="flex items-center gap-2">
            <button
              onClick={onClose}
              className="px-4 py-2 bg-slate-200 hover:bg-slate-300 text-slate-700 text-xs font-semibold rounded-lg transition-colors"
            >
              Đóng
            </button>

            <button
              onClick={handleDeployToVbox}
              className="px-4 py-2 bg-sky-600 hover:bg-sky-700 text-white text-xs font-bold rounded-lg shadow-sm flex items-center gap-1.5 transition-colors"
            >
              <Upload className="w-3.5 h-3.5" />
              <span>Nạp Xuống V-Box</span>
            </button>
          </div>
        </div>
      </div>
    </div>
  );
};
