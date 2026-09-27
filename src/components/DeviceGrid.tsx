'use client';

import React, { useState } from 'react';
import { 
  CloudRain, 
  Lightbulb, 
  Flame, 
  Volume2, 
  Calendar, 
  Clock, 
  Lock, 
  Timer, 
  RotateCw 
} from 'lucide-react';
import { DeviceKey, DEVICE_DEFINITIONS, DeviceConfig } from '@/lib/types';
import { mqttService } from '@/lib/mqtt-client';

interface DeviceGridProps {
  devicesState: Record<DeviceKey, boolean>;
  autoTempMode: boolean;
  autoHumidityMode: boolean;
  onOpenSchedule: (device: DeviceConfig) => void;
}

export const DeviceGrid: React.FC<DeviceGridProps> = ({
  devicesState,
  autoTempMode,
  autoHumidityMode,
  onOpenSchedule,
}) => {
  const [timerModalDev, setTimerModalDev] = useState<DeviceConfig | null>(null);
  const [timerDuration, setTimerDuration] = useState<number>(30);
  const [cycleOn, setCycleOn] = useState<number>(10);
  const [cycleOff, setCycleOff] = useState<number>(60);
  const [modeTab, setModeTab] = useState<'timer' | 'cycle'>('timer');

  const getDeviceIcon = (category: string) => {
    switch (category) {
      case 'mist':
        return <CloudRain className="w-5 h-5 text-sky-500" />;
      case 'light':
        return <Lightbulb className="w-5 h-5 text-amber-500" />;
      case 'heater':
        return <Flame className="w-5 h-5 text-orange-500" />;
      case 'speaker':
        return <Volume2 className="w-5 h-5 text-purple-500" />;
      default:
        return <CloudRain className="w-5 h-5" />;
    }
  };

  const isDeviceLocked = (key: DeviceKey) => {
    if (['phun_suong_1', 'phun_suong_2', 'phun_suong_3', 'phun_suong_4'].includes(key)) {
      return autoHumidityMode;
    }
    if (key === 'suoi_1') {
      return autoTempMode;
    }
    return false;
  };

  const handleToggle = (dev: DeviceConfig, nextState: boolean) => {
    if (isDeviceLocked(dev.key)) {
      alert(`Thiết bị ${dev.name} đang ở chế độ TỰ ĐỘNG! Hãy gạt TẮT chế độ tự động ở bảng phía trên nếu muốn điều khiển thủ công.`);
      return;
    }
    mqttService.setDeviceState(dev.key, nextState);
  };

  const handleApplyTimer = () => {
    if (!timerModalDev) return;
    if (modeTab === 'timer') {
      mqttService.setDeviceTimer(timerModalDev.key, timerDuration);
    } else {
      mqttService.setDeviceCycle(timerModalDev.key, cycleOn, cycleOff);
    }
    setTimerModalDev(null);
  };

  return (
    <div className="bg-white border border-slate-200 rounded-xl p-5 shadow-sm mb-6">
      <div className="flex items-center justify-between mb-4 pb-3 border-b border-slate-100">
        <div>
          <h2 className="text-base font-bold text-slate-800">
            BẢNG ĐIỀU KHIỂN & PHẢN HỒI 10 THIẾT BỊ
          </h2>
          <p className="text-xs text-slate-500">
            Lệnh điều khiển ghi @B_0#HDX0.0 &rarr; HDX0.9 • Phản hồi thực tế đọc @B_0#HDX1.0 &rarr; HDX1.9
          </p>
        </div>
      </div>

      <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-4">
        {DEVICE_DEFINITIONS.map((dev) => {
          const isRunning = Boolean(devicesState[dev.key]);
          const locked = isDeviceLocked(dev.key);

          return (
            <div
              key={dev.key}
              className={`border rounded-xl p-4 transition-all flex flex-col justify-between ${
                isRunning
                  ? 'bg-sky-50/40 border-sky-300 shadow-sm'
                  : 'bg-white border-slate-200 hover:border-slate-300'
              }`}
            >
              <div>
                {/* Header item */}
                <div className="flex items-start justify-between gap-2 mb-2">
                  <div className="flex items-center gap-2">
                    <div className="p-2 bg-slate-100 rounded-lg">
                      {getDeviceIcon(dev.category)}
                    </div>
                    <div>
                      <h4 className="font-bold text-sm text-slate-800">{dev.name}</h4>
                      <span className="text-[11px] text-slate-400 font-mono">
                        {dev.cmdAddr} &bull; {dev.statusAddr}
                      </span>
                    </div>
                  </div>

                  <label className="switch">
                    <input
                      type="checkbox"
                      checked={isRunning}
                      disabled={locked}
                      onChange={(e) => handleToggle(dev, e.target.checked)}
                    />
                    <span className="slider"></span>
                  </label>
                </div>

                {/* Status Badges */}
                <div className="flex items-center gap-2 my-2">
                  <span
                    className={`text-xs px-2.5 py-0.5 rounded-full font-bold uppercase tracking-wider ${
                      isRunning
                        ? 'bg-emerald-100 text-emerald-800 border border-emerald-300 animate-pulse'
                        : 'bg-slate-100 text-slate-600 border border-slate-200'
                    }`}
                  >
                    {isRunning ? 'ĐANG BẬT' : 'TẮT'}
                  </span>

                  {locked && (
                    <span className="text-[11px] px-2 py-0.5 rounded bg-amber-100 text-amber-800 font-semibold flex items-center gap-1">
                      <Lock className="w-3 h-3" />
                      Tự động
                    </span>
                  )}
                </div>
              </div>

              {/* Action buttons */}
              <div className="mt-3 pt-3 border-t border-slate-100 flex items-center gap-2">
                <button
                  onClick={() => setTimerModalDev(dev)}
                  className="flex-1 py-1.5 px-2 bg-slate-100 hover:bg-slate-200 text-slate-700 text-xs font-semibold rounded-lg flex items-center justify-center gap-1 transition-colors"
                  title="Hẹn giờ bật X giây hoặc chu kỳ bật/nghỉ"
                >
                  <Clock className="w-3.5 h-3.5 text-slate-500" />
                  <span>Hẹn giờ</span>
                </button>

                <button
                  onClick={() => onOpenSchedule(dev)}
                  className="flex-1 py-1.5 px-2 bg-sky-50 hover:bg-sky-100 text-sky-700 text-xs font-semibold rounded-lg flex items-center justify-center gap-1 border border-sky-200 transition-colors"
                  title={`Cài đặt tối đa ${dev.maxSlots} khung giờ hẹn trong ngày`}
                >
                  <Calendar className="w-3.5 h-3.5 text-sky-600" />
                  <span>Lịch ({dev.maxSlots})</span>
                </button>
              </div>
            </div>
          );
        })}
      </div>

      {/* Modal Hẹn Giờ Nhanh / Chu Kỳ */}
      {timerModalDev && (
        <div className="fixed inset-0 z-50 bg-black/50 backdrop-blur-sm flex items-center justify-center p-4">
          <div className="bg-white rounded-2xl max-w-sm w-full p-5 shadow-2xl border border-slate-200 animate-in fade-in zoom-in-95">
            <h3 className="text-base font-bold text-slate-900 mb-1">
              Hẹn Giờ / Chu Kỳ: {timerModalDev.name}
            </h3>
            <p className="text-xs text-slate-500 mb-4">
              Lệnh nạp tức thời qua MQTT xuống V-Box và PLC
            </p>

            <div className="flex border-b border-slate-200 mb-4">
              <button
                onClick={() => setModeTab('timer')}
                className={`flex-1 py-2 text-xs font-bold border-b-2 flex items-center justify-center gap-1 ${
                  modeTab === 'timer'
                    ? 'border-sky-600 text-sky-600'
                    : 'border-transparent text-slate-500'
                }`}
              >
                <Timer className="w-3.5 h-3.5" />
                Hẹn Giờ Tắt (Đếm Ngược)
              </button>
              <button
                onClick={() => setModeTab('cycle')}
                className={`flex-1 py-2 text-xs font-bold border-b-2 flex items-center justify-center gap-1 ${
                  modeTab === 'cycle'
                    ? 'border-sky-600 text-sky-600'
                    : 'border-transparent text-slate-500'
                }`}
              >
                <RotateCw className="w-3.5 h-3.5" />
                Chu Kỳ Tuần Hoàn
              </button>
            </div>

            {modeTab === 'timer' ? (
              <div className="space-y-3">
                <label className="text-xs font-semibold text-slate-700">
                  Thời gian bật trước khi tự tắt (giây):
                </label>
                <input
                  type="number"
                  min="1"
                  max="86400"
                  value={timerDuration}
                  onChange={(e) => setTimerDuration(Number(e.target.value))}
                  className="w-full px-3 py-2 border border-slate-300 rounded-lg text-center font-bold text-lg text-slate-900 focus:outline-sky-500"
                />
                <div className="flex gap-2">
                  {[10, 30, 60, 300, 600].map((s) => (
                    <button
                      key={s}
                      onClick={() => setTimerDuration(s)}
                      className="flex-1 py-1 bg-slate-100 hover:bg-slate-200 text-[11px] rounded text-slate-700 font-medium"
                    >
                      {s < 60 ? `${s}s` : `${s / 60}m`}
                    </button>
                  ))}
                </div>
              </div>
            ) : (
              <div className="space-y-3 text-xs text-slate-700">
                <div>
                  <label className="font-semibold block mb-1">Thời gian BẬT (giây):</label>
                  <input
                    type="number"
                    min="1"
                    max="3600"
                    value={cycleOn}
                    onChange={(e) => setCycleOn(Number(e.target.value))}
                    className="w-full px-3 py-2 border border-slate-300 rounded-lg text-center font-bold text-slate-900 focus:outline-sky-500"
                  />
                </div>
                <div>
                  <label className="font-semibold block mb-1">Thời gian NGHỈ (giây):</label>
                  <input
                    type="number"
                    min="1"
                    max="7200"
                    value={cycleOff}
                    onChange={(e) => setCycleOff(Number(e.target.value))}
                    className="w-full px-3 py-2 border border-slate-300 rounded-lg text-center font-bold text-slate-900 focus:outline-sky-500"
                  />
                </div>
              </div>
            )}

            <div className="mt-6 flex items-center justify-end gap-2">
              <button
                onClick={() => setTimerModalDev(null)}
                className="px-4 py-2 bg-slate-100 hover:bg-slate-200 text-slate-700 text-xs font-semibold rounded-lg transition-colors"
              >
                Hủy
              </button>
              <button
                onClick={handleApplyTimer}
                className="px-4 py-2 bg-sky-600 hover:bg-sky-700 text-white text-xs font-semibold rounded-lg shadow-sm transition-colors"
              >
                Kích Hoạt Lệnh
              </button>
            </div>
          </div>
        </div>
      )}
    </div>
  );
};
