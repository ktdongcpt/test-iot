'use client';

import React, { useState, useEffect } from 'react';
import { Sliders, Droplets, Flame, Check, HelpCircle } from 'lucide-react';
import { mqttService } from '@/lib/mqtt-client';

interface AutoControlPanelProps {
  autoTempMode: boolean;
  autoHumidityMode: boolean;
  tempThreshold: number;
  humThreshold: number;
  psOnSec: number;
  psOffSec: number;
}

export const AutoControlPanel: React.FC<AutoControlPanelProps> = ({
  autoTempMode,
  autoHumidityMode,
  tempThreshold,
  humThreshold,
  psOnSec,
  psOffSec,
}) => {
  const [tempTh, setTempTh] = useState<number>(tempThreshold);
  const [humTh, setHumTh] = useState<number>(humThreshold);
  const [onSec, setOnSec] = useState<number>(psOnSec);
  const [offSec, setOffSec] = useState<number>(psOffSec);
  const [onUnit, setOnUnit] = useState<'s' | 'm'>('s');
  const [offUnit, setOffUnit] = useState<'s' | 'm'>('m');
  const [savedMsg, setSavedMsg] = useState<string>('');

  useEffect(() => {
    setTempTh(tempThreshold);
  }, [tempThreshold]);

  useEffect(() => {
    setHumTh(humThreshold);
  }, [humThreshold]);

  useEffect(() => {
    setOnSec(psOnSec);
  }, [psOnSec]);

  useEffect(() => {
    setOffSec(psOffSec);
  }, [psOffSec]);

  const showNotification = (msg: string) => {
    setSavedMsg(msg);
    setTimeout(() => setSavedMsg(''), 3000);
  };

  const handleToggleAutoHumidity = (checked: boolean) => {
    mqttService.updateAutoSettings({ auto_humidity: checked });
    showNotification(checked ? 'Đã BẬT Tự động Độ ẩm' : 'Đã TẮT Tự động (Chuyển lịch RTC)');
  };

  const handleToggleAutoTemp = (checked: boolean) => {
    mqttService.updateAutoSettings({ auto_temp: checked });
    showNotification(checked ? 'Đã BẬT Tự động Nhiệt độ' : 'Đã TẮT Tự động (Chuyển lịch RTC)');
  };

  const handleSaveHumiditySettings = () => {
    const finalOnSec = onUnit === 'm' ? onSec * 60 : onSec;
    const finalOffSec = offUnit === 'm' ? offSec * 60 : offSec;

    mqttService.updateAutoSettings({
      hum_threshold: Number(humTh),
      ps_on_sec: Number(finalOnSec),
      ps_off_sec: Number(finalOffSec),
    });
    showNotification('Đã lưu cấu hình Phun sương tự động');
  };

  const handleSaveTempSettings = () => {
    mqttService.updateAutoSettings({
      temp_threshold: Number(tempTh),
    });
    showNotification('Đã lưu cấu hình Sưởi 1 tự động');
  };

  return (
    <div className="bg-white border border-slate-200 rounded-xl p-5 shadow-sm mb-6">
      <div className="flex items-center justify-between mb-4 pb-3 border-b border-slate-100">
        <div className="flex items-center gap-2">
          <Sliders className="w-5 h-5 text-sky-600" />
          <h2 className="text-base font-bold text-slate-800">
            CÀI ĐẶT CHẾ ĐỘ TỰ ĐỘNG THEO CẢM BIẾN (AUTO SENSOR)
          </h2>
        </div>
        {savedMsg && (
          <span className="text-xs px-3 py-1 bg-emerald-100 text-emerald-800 font-semibold rounded-full flex items-center gap-1 animate-pulse">
            <Check className="w-3.5 h-3.5" />
            {savedMsg}
          </span>
        )}
      </div>

      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        {/* Cột 1: Phun sương theo độ ẩm */}
        <div className="bg-slate-50 border border-slate-200 rounded-xl p-4 flex flex-col justify-between">
          <div>
            <div className="flex items-center justify-between mb-3">
              <div className="flex items-center gap-2">
                <Droplets className="w-5 h-5 text-sky-600" />
                <span className="font-bold text-slate-800 text-sm">
                  4 Bơm Phun Sương (Độ Ẩm)
                </span>
              </div>
              <label className="switch">
                <input
                  type="checkbox"
                  checked={autoHumidityMode}
                  onChange={(e) => handleToggleAutoHumidity(e.target.checked)}
                />
                <span className="slider"></span>
              </label>
            </div>

            <div className="text-xs mb-4">
              <span
                className={`px-2.5 py-1 rounded-full font-semibold ${
                  autoHumidityMode
                    ? 'bg-sky-100 text-sky-800'
                    : 'bg-slate-200 text-slate-600'
                }`}
              >
                {autoHumidityMode ? 'Đang chạy: Tự động cảm biến' : 'Đang chạy: Lịch hẹn RTC'}
              </span>
            </div>

            {/* Ngưỡng độ ẩm */}
            <div className="space-y-3 text-xs text-slate-700">
              <div className="flex items-center justify-between gap-2">
                <label className="font-medium">Bật phun khi Độ ẩm &lt;</label>
                <div className="flex items-center gap-1">
                  <input
                    type="number"
                    min="10"
                    max="100"
                    step="1"
                    value={humTh}
                    onChange={(e) => setHumTh(Number(e.target.value))}
                    className="w-20 px-2 py-1 border border-slate-300 rounded text-center font-bold text-slate-800 focus:outline-sky-500 bg-white"
                  />
                  <span className="font-semibold text-slate-500">%</span>
                </div>
              </div>

              {/* Thời gian phun mỗi lần */}
              <div className="flex items-center justify-between gap-2">
                <label className="font-medium">Thời gian phun mỗi lần:</label>
                <div className="flex items-center gap-1">
                  <input
                    type="number"
                    min="5"
                    max="3600"
                    value={onSec}
                    onChange={(e) => setOnSec(Number(e.target.value))}
                    className="w-20 px-2 py-1 border border-slate-300 rounded text-center font-bold text-slate-800 focus:outline-sky-500 bg-white"
                  />
                  <select
                    value={onUnit}
                    onChange={(e) => setOnUnit(e.target.value as 's' | 'm')}
                    className="px-2 py-1 border border-slate-300 rounded text-xs bg-white text-slate-700"
                  >
                    <option value="s">giây</option>
                    <option value="m">phút</option>
                  </select>
                </div>
              </div>

              {/* Thời gian nghỉ lan tỏa ẩm */}
              <div className="flex items-center justify-between gap-2">
                <label className="font-medium">Nghỉ lan tỏa (khóa bơm):</label>
                <div className="flex items-center gap-1">
                  <input
                    type="number"
                    min="0"
                    max="7200"
                    value={offSec}
                    onChange={(e) => setOffSec(Number(e.target.value))}
                    className="w-20 px-2 py-1 border border-slate-300 rounded text-center font-bold text-slate-800 focus:outline-sky-500 bg-white"
                  />
                  <select
                    value={offUnit}
                    onChange={(e) => setOffUnit(e.target.value as 's' | 'm')}
                    className="px-2 py-1 border border-slate-300 rounded text-xs bg-white text-slate-700"
                  >
                    <option value="s">giây</option>
                    <option value="m">phút</option>
                  </select>
                </div>
              </div>
            </div>
          </div>

          <div className="mt-4 pt-3 border-t border-slate-200">
            <button
              onClick={handleSaveHumiditySettings}
              className="w-full py-2 bg-sky-600 hover:bg-sky-700 text-white rounded-lg text-xs font-semibold shadow-sm transition-colors"
            >
              Lưu Cài Đặt Phun Sương
            </button>
            <p className="text-[11px] text-slate-500 mt-2 leading-relaxed">
              • Chu kỳ 3 trạng thái: <b>Phun</b> &rarr; <b>Nghỉ lan tỏa khóa bơm</b> &rarr; <b>Chờ đủ ẩm</b>.<br />
              • Khi gạt TẮT, cả 4 bơm sẽ tự động trở về hoạt động theo các khung giờ hẹn RTC.
            </p>
          </div>
        </div>

        {/* Cột 2: Sưởi 1 theo nhiệt độ */}
        <div className="bg-slate-50 border border-slate-200 rounded-xl p-4 flex flex-col justify-between">
          <div>
            <div className="flex items-center justify-between mb-3">
              <div className="flex items-center gap-2">
                <Flame className="w-5 h-5 text-amber-600" />
                <span className="font-bold text-slate-800 text-sm">
                  Sưởi 1 (Nhiệt Độ)
                </span>
              </div>
              <label className="switch">
                <input
                  type="checkbox"
                  checked={autoTempMode}
                  onChange={(e) => handleToggleAutoTemp(e.target.checked)}
                />
                <span className="slider"></span>
              </label>
            </div>

            <div className="text-xs mb-4">
              <span
                className={`px-2.5 py-1 rounded-full font-semibold ${
                  autoTempMode
                    ? 'bg-amber-100 text-amber-800'
                    : 'bg-slate-200 text-slate-600'
                }`}
              >
                {autoTempMode ? 'Đang chạy: Tự động cảm biến' : 'Đang chạy: Lịch hẹn RTC'}
              </span>
            </div>

            <div className="space-y-3 text-xs text-slate-700">
              <div className="flex items-center justify-between gap-2">
                <label className="font-medium">Bật Sưởi 1 khi Nhiệt độ &lt;</label>
                <div className="flex items-center gap-1">
                  <input
                    type="number"
                    min="0"
                    max="60"
                    step="0.5"
                    value={tempTh}
                    onChange={(e) => setTempTh(Number(e.target.value))}
                    className="w-20 px-2 py-1 border border-slate-300 rounded text-center font-bold text-slate-800 focus:outline-amber-500 bg-white"
                  />
                  <span className="font-semibold text-slate-500">°C</span>
                </div>
              </div>
            </div>
          </div>

          <div className="mt-4 pt-3 border-t border-slate-200">
            <button
              onClick={handleSaveTempSettings}
              className="w-full py-2 bg-amber-600 hover:bg-amber-700 text-white rounded-lg text-xs font-semibold shadow-sm transition-colors"
            >
              Lưu Cài Đặt Sưởi 1
            </button>
            <p className="text-[11px] text-slate-500 mt-2 leading-relaxed">
              • Khi gạt BẬT: Nhiệt độ &lt; ngưỡng cài sẽ lập tức bật Sưởi 1; khi nhiệt độ đạt sẽ tự ngắt.<br />
              • Khi gạt TẮT: Sưởi 1 sẽ quay lại chạy theo lịch hẹn RTC.
            </p>
          </div>
        </div>
      </div>
    </div>
  );
};
