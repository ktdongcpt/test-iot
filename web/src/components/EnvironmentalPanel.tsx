'use client';

import React from 'react';
import { Thermometer, Droplets, Flame, Waves, AlertTriangle, ShieldCheck } from 'lucide-react';

interface EnvironmentalPanelProps {
  temperature: number;
  humidity: number;
  tempThreshold: number;
  humThreshold: number;
  autoTempMode: boolean;
  autoHumidityMode: boolean;
  psCycleState: number; // 0: IDLE, 1: SPRAYING, 2: RESTING
  psOnSec: number;
  psOffSec: number;
  stateSuoi1: boolean;
  statePhunSuongAny: boolean;
}

export const EnvironmentalPanel: React.FC<EnvironmentalPanelProps> = ({
  temperature,
  humidity,
  tempThreshold,
  humThreshold,
  autoTempMode,
  autoHumidityMode,
  psCycleState,
  psOnSec,
  psOffSec,
  stateSuoi1,
  statePhunSuongAny,
}) => {
  // Trạng thái chu kỳ phun
  const getCycleStateBadge = () => {
    if (!autoHumidityMode) {
      return (
        <span className="text-xs px-2.5 py-1 rounded-full bg-slate-100 text-slate-600 font-medium">
          Chế độ lịch RTC
        </span>
      );
    }
    if (psCycleState === 1) {
      return (
        <span className="text-xs px-2.5 py-1 rounded-full bg-cyan-100 text-cyan-800 font-semibold flex items-center gap-1 animate-pulse">
          <Waves className="w-3.5 h-3.5" />
          ĐANG PHUN ({psOnSec}s)
        </span>
      );
    }
    if (psCycleState === 2) {
      return (
        <span className="text-xs px-2.5 py-1 rounded-full bg-amber-100 text-amber-800 font-semibold flex items-center gap-1">
          <ShieldCheck className="w-3.5 h-3.5" />
          NGHỈ LAN TỎA ({psOffSec}s)
        </span>
      );
    }
    return (
      <span className="text-xs px-2.5 py-1 rounded-full bg-emerald-100 text-emerald-800 font-medium">
        ĐỦ ẨM (CHỜ)
      </span>
    );
  };

  return (
    <div className="grid grid-cols-1 md:grid-cols-2 gap-4 mb-6">
      {/* Thẻ Nhiệt Độ */}
      <div className="bg-white border border-slate-200 rounded-xl p-5 shadow-sm relative overflow-hidden">
        <div className="flex items-center justify-between mb-3">
          <div className="flex items-center gap-2">
            <div className="p-2 bg-amber-50 rounded-lg text-amber-600">
              <Thermometer className="w-5 h-5" />
            </div>
            <div>
              <h3 className="font-semibold text-slate-800 text-sm">Cảm Biến Nhiệt Độ</h3>
              <p className="text-xs text-slate-500">Thanh ghi @W_0#HDW10</p>
            </div>
          </div>
          <div className="text-right">
            <span
              className={`text-xs px-2.5 py-1 rounded-full font-medium ${
                autoTempMode
                  ? 'bg-amber-100 text-amber-800 font-semibold'
                  : 'bg-slate-100 text-slate-600'
              }`}
            >
              {autoTempMode ? 'TỰ ĐỘNG SƯỞI' : 'LỊCH RTC'}
            </span>
          </div>
        </div>

        <div className="flex items-baseline gap-2 my-2">
          <span className="text-4xl font-extrabold tracking-tight text-amber-600">
            {temperature.toFixed(1)}
          </span>
          <span className="text-xl font-bold text-slate-500">°C</span>
        </div>

        <div className="mt-4 pt-3 border-t border-slate-100 flex items-center justify-between text-xs">
          <span className="text-slate-600">
            Ngưỡng kích hoạt: <b>&lt; {tempThreshold.toFixed(1)}°C</b>
          </span>
          <div className="flex items-center gap-1.5">
            <span className="text-slate-500">Sưởi 1:</span>
            <span
              className={`px-2 py-0.5 rounded font-bold ${
                stateSuoi1 ? 'bg-orange-100 text-orange-700' : 'bg-slate-100 text-slate-600'
              }`}
            >
              {stateSuoi1 ? 'ĐANG BẬT' : 'TẮT'}
            </span>
          </div>
        </div>
      </div>

      {/* Thẻ Độ Ẩm */}
      <div className="bg-white border border-slate-200 rounded-xl p-5 shadow-sm relative overflow-hidden">
        <div className="flex items-center justify-between mb-3">
          <div className="flex items-center gap-2">
            <div className="p-2 bg-sky-50 rounded-lg text-sky-600">
              <Droplets className="w-5 h-5" />
            </div>
            <div>
              <h3 className="font-semibold text-slate-800 text-sm">Cảm Biến Độ Ẩm</h3>
              <p className="text-xs text-slate-500">Thanh ghi @W_0#HDW11</p>
            </div>
          </div>
          <div>{getCycleStateBadge()}</div>
        </div>

        <div className="flex items-baseline gap-2 my-2">
          <span className="text-4xl font-extrabold tracking-tight text-sky-600">
            {humidity.toFixed(1)}
          </span>
          <span className="text-xl font-bold text-slate-500">%</span>
        </div>

        <div className="mt-4 pt-3 border-t border-slate-100 flex items-center justify-between text-xs">
          <span className="text-slate-600">
            Ngưỡng kích hoạt: <b>&lt; {humThreshold.toFixed(1)}%</b>
          </span>
          <div className="flex items-center gap-1.5">
            <span className="text-slate-500">4 Bơm Phun Sương:</span>
            <span
              className={`px-2 py-0.5 rounded font-bold ${
                statePhunSuongAny ? 'bg-cyan-100 text-cyan-700' : 'bg-slate-100 text-slate-600'
              }`}
            >
              {statePhunSuongAny ? 'ĐANG PHUN' : 'TẮT'}
            </span>
          </div>
        </div>
      </div>
    </div>
  );
};
