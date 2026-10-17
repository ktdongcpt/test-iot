'use client';

import React from 'react';
import { 
  Thermometer, 
  Droplets, 
  Flame, 
  Waves, 
  AlertTriangle, 
  ShieldCheck, 
  Volume2, 
  Zap,
  CheckCircle2,
  AlertCircle
} from 'lucide-react';

interface EnvironmentalPanelProps {
  temperature: number;
  humidity: number;
  tempThreshold: number;
  humThreshold: number;
  autoTempMode: boolean;
  autoHumidityMode: boolean;
  autoPumpMode: boolean;
  waterEmpty: number;    // 1: Thiếu nước, 0: Đủ nước
  waterSafe: number;     // 1: Chưa tràn, 0: Tràn
  waterOverflow: number; // 1: Đang tràn, 0: Chưa tràn
  psCycleState: number;  // 0: IDLE, 1: SPRAYING, 2: RESTING
  psOnSec: number;
  psOffSec: number;
  stateSuoi1: boolean;
  statePhunSuongAny: boolean;
  stateMayBom: boolean;
  sound: {
    s1: number;
    s2: number;
    s3: number;
  };
  current: {
    light_1: number;
    light_2: number;
    misting_1: number;
    misting_2: number;
    misting_3: number;
    speaker_1: number;
    speaker_2: number;
    pump: number;
  };
}

export const EnvironmentalPanel: React.FC<EnvironmentalPanelProps> = ({
  temperature,
  humidity,
  tempThreshold,
  humThreshold,
  autoTempMode,
  autoHumidityMode,
  autoPumpMode,
  waterEmpty,
  waterSafe,
  waterOverflow,
  psCycleState,
  psOnSec,
  psOffSec,
  stateSuoi1,
  statePhunSuongAny,
  stateMayBom,
  sound,
  current,
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

  const isOverflow = waterOverflow === 1 || waterSafe === 0;
  const isLackWater = waterEmpty === 1;

  // Danh sách dòng điện 8 thiết bị (giữ nguyên nhãn tiếng Việt)
  const currentDevices = [
    { key: 'light_1', label: 'Đèn 1', addr: 'HDW20', val: current?.light_1 ?? 0 },
    { key: 'light_2', label: 'Đèn 2', addr: 'HDW21', val: current?.light_2 ?? 0 },
    { key: 'misting_1', label: 'Phun sương 1', addr: 'HDW22', val: current?.misting_1 ?? 0 },
    { key: 'misting_2', label: 'Phun sương 2', addr: 'HDW23', val: current?.misting_2 ?? 0 },
    { key: 'misting_3', label: 'Phun sương 3', addr: 'HDW24', val: current?.misting_3 ?? 0 },
    { key: 'speaker_1', label: 'Loa 1', addr: 'HDW25', val: current?.speaker_1 ?? 0 },
    { key: 'speaker_2', label: 'Loa 2', addr: 'HDW26', val: current?.speaker_2 ?? 0 },
    { key: 'pump', label: 'Máy bơm', addr: 'HDW27', val: current?.pump ?? 0 },
  ];

  return (
    <div className="space-y-4 mb-6">
      {/* Hàng 1: Nhiệt Độ, Độ Ẩm, Mực Nước Bể Chứa */}
      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
        {/* Thẻ Nhiệt Độ */}
        <div className="bg-white border border-slate-200 rounded-xl p-5 shadow-sm relative overflow-hidden flex flex-col justify-between">
          <div>
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
                {Number(temperature || 0).toFixed(1)}
              </span>
              <span className="text-xl font-bold text-slate-500">°C</span>
            </div>
          </div>

          <div className="mt-4 pt-3 border-t border-slate-100 flex items-center justify-between text-xs">
            <span className="text-slate-600">
              Ngưỡng kích hoạt: <b>&lt; {Number(tempThreshold || 0).toFixed(1)}°C</b>
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
        <div className="bg-white border border-slate-200 rounded-xl p-5 shadow-sm relative overflow-hidden flex flex-col justify-between">
          <div>
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
                {Number(humidity || 0).toFixed(1)}
              </span>
              <span className="text-xl font-bold text-slate-500">%</span>
            </div>
          </div>

          <div className="mt-4 pt-3 border-t border-slate-100 flex items-center justify-between text-xs">
            <span className="text-slate-600">
              Ngưỡng kích hoạt: <b>&lt; {Number(humThreshold || 0).toFixed(1)}%</b>
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

        {/* Thẻ Mực Nước Bể Chứa */}
        <div className="bg-white border border-slate-200 rounded-xl p-5 shadow-sm relative overflow-hidden flex flex-col justify-between">
          <div>
            <div className="flex items-center justify-between mb-3">
              <div className="flex items-center gap-2">
                <div className="p-2 bg-blue-50 rounded-lg text-blue-600">
                  <Waves className="w-5 h-5" />
                </div>
                <div>
                  <h3 className="font-semibold text-slate-800 text-sm">Mực Nước Bể Chứa</h3>
                  <p className="text-xs text-slate-500">Phao @B_0#HDX5.0 & HDX5.1</p>
                </div>
              </div>
              <div className="text-right">
                <span
                  className={`text-xs px-2.5 py-1 rounded-full font-medium ${
                    autoPumpMode
                      ? 'bg-blue-100 text-blue-800 font-semibold'
                      : 'bg-slate-100 text-slate-600'
                  }`}
                >
                  {autoPumpMode ? 'TỰ ĐỘNG BƠM' : 'THỦ CÔNG / RTC'}
                </span>
              </div>
            </div>

            {/* Trạng thái 2 phao: Thiếu nước đáy & Tràn đỉnh */}
            <div className="grid grid-cols-2 gap-2 my-2">
              <div className={`p-2.5 rounded-lg border flex flex-col items-center justify-center text-center ${
                isLackWater 
                  ? 'bg-amber-50 border-amber-200 text-amber-900' 
                  : 'bg-emerald-50 border-emerald-200 text-emerald-900'
              }`}>
                <span className="text-[11px] text-slate-500 font-medium">Phao đáy bể</span>
                <span className={`text-xs font-bold mt-0.5 flex items-center gap-1 ${
                  isLackWater ? 'text-amber-700 animate-pulse' : 'text-emerald-700'
                }`}>
                  {isLackWater ? (
                    <>
                      <AlertTriangle className="w-3.5 h-3.5 text-amber-600" />
                      CẠN ĐÁY (BẬT)
                    </>
                  ) : (
                    <>
                      <CheckCircle2 className="w-3.5 h-3.5 text-emerald-600" />
                      TRÊN ĐÁY (ĐỦ)
                    </>
                  )}
                </span>
                <span className="text-[10px] text-slate-400 font-mono mt-0.5">HDX5.0: {waterEmpty}</span>
              </div>

              <div className={`p-2.5 rounded-lg border flex flex-col items-center justify-center text-center ${
                isOverflow 
                  ? 'bg-rose-50 border-rose-300 text-rose-900 animate-pulse' 
                  : 'bg-emerald-50 border-emerald-200 text-emerald-900'
              }`}>
                <span className="text-[11px] text-slate-500 font-medium">Phao đỉnh bể</span>
                <span className={`text-xs font-bold mt-0.5 flex items-center gap-1 ${
                  isOverflow ? 'text-rose-700 font-black' : 'text-emerald-700'
                }`}>
                  {isOverflow ? (
                    <>
                      <AlertCircle className="w-3.5 h-3.5 text-rose-600" />
                      🚨 ĐẦY / TRÀN ĐỈNH
                    </>
                  ) : (
                    <>
                      <ShieldCheck className="w-3.5 h-3.5 text-emerald-600" />
                      CHƯA ĐẦY ĐỈNH
                    </>
                  )}
                </span>
                <span className="text-[10px] text-slate-400 font-mono mt-0.5">HDX5.1: {waterSafe}</span>
              </div>
            </div>
          </div>

          <div className="mt-4 pt-3 border-t border-slate-100 flex items-center justify-between text-xs">
            <span className="text-slate-600">
              Chế độ bơm: <b>{autoPumpMode ? (stateMayBom ? 'Bơm dâng tới đỉnh' : 'Chờ dùng cạn đáy') : 'Thủ công / RTC'}</b>
            </span>
            <div className="flex items-center gap-1.5">
              <span className="text-slate-500">Máy bơm:</span>
              <span
                className={`px-2 py-0.5 rounded font-bold ${
                  stateMayBom ? 'bg-blue-100 text-blue-700 animate-pulse' : 'bg-slate-100 text-slate-600'
                }`}
              >
                {stateMayBom ? 'ĐANG BƠM' : 'TẮT'}
              </span>
            </div>
          </div>
        </div>
      </div>

      {/* Hàng 2: Cảm Biến Độ Ồn Âm Thanh & Giám Sát Dòng Điện 8 Thiết Bị */}
      <div className="grid grid-cols-1 lg:grid-cols-3 gap-4">
        {/* Cảm Biến Âm Thanh / Độ Ồn (1 cột trên desktop) */}
        <div className="bg-white border border-slate-200 rounded-xl p-5 shadow-sm flex flex-col justify-between">
          <div>
            <div className="flex items-center justify-between mb-3">
              <div className="flex items-center gap-2">
                <div className="p-2 bg-purple-50 rounded-lg text-purple-600">
                  <Volume2 className="w-5 h-5" />
                </div>
                <div>
                  <h3 className="font-semibold text-slate-800 text-sm">Cảm Biến Âm Thanh (Độ Ồn)</h3>
                  <p className="text-xs text-slate-500">Thanh ghi @W_0#HDW30 &rarr; HDW32 (0.1 dB)</p>
                </div>
              </div>
              <span className="text-xs px-2 py-0.5 bg-purple-50 text-purple-700 font-semibold rounded-md border border-purple-200">
                3 Vị trí
              </span>
            </div>

            <div className="space-y-2.5 my-2">
              <div className="p-2.5 bg-slate-50 border border-slate-200 rounded-lg flex items-center justify-between">
                <div>
                  <span className="text-xs font-bold text-slate-800">Âm thanh 1</span>
                  <p className="text-[11px] text-slate-400 font-mono">HDW30</p>
                </div>
                <div className="flex items-baseline gap-1">
                  <span className="text-2xl font-black text-purple-700 tracking-tight">
                    {Number(sound?.s1 || 0).toFixed(1)}
                  </span>
                  <span className="text-xs font-bold text-slate-500">dB</span>
                </div>
              </div>

              <div className="p-2.5 bg-slate-50 border border-slate-200 rounded-lg flex items-center justify-between">
                <div>
                  <span className="text-xs font-bold text-slate-800">Âm thanh 2</span>
                  <p className="text-[11px] text-slate-400 font-mono">HDW31</p>
                </div>
                <div className="flex items-baseline gap-1">
                  <span className="text-2xl font-black text-purple-700 tracking-tight">
                    {Number(sound?.s2 || 0).toFixed(1)}
                  </span>
                  <span className="text-xs font-bold text-slate-500">dB</span>
                </div>
              </div>

              <div className="p-2.5 bg-slate-50 border border-slate-200 rounded-lg flex items-center justify-between">
                <div>
                  <span className="text-xs font-bold text-slate-800">Âm thanh 3</span>
                  <p className="text-[11px] text-slate-400 font-mono">HDW32</p>
                </div>
                <div className="flex items-baseline gap-1">
                  <span className="text-2xl font-black text-purple-700 tracking-tight">
                    {Number(sound?.s3 || 0).toFixed(1)}
                  </span>
                  <span className="text-xs font-bold text-slate-500">dB</span>
                </div>
              </div>
            </div>
          </div>

          <p className="text-[11px] text-slate-400 mt-2">
            Đo độ ồn môi trường thực tế tại nhà yến theo thời gian thực.
          </p>
        </div>

        {/* Giám Sát Dòng Điện 8 Thiết Bị (2 cột trên desktop) */}
        <div className="lg:col-span-2 bg-white border border-slate-200 rounded-xl p-5 shadow-sm flex flex-col justify-between">
          <div>
            <div className="flex items-center justify-between mb-3">
              <div className="flex items-center gap-2">
                <div className="p-2 bg-amber-50 rounded-lg text-amber-600">
                  <Zap className="w-5 h-5" />
                </div>
                <div>
                  <h3 className="font-semibold text-slate-800 text-sm">Giám Sát Dòng Điện 8 Thiết Bị (Ampe)</h3>
                  <p className="text-xs text-slate-500">Thanh ghi @W_0#HDW20 &rarr; HDW27 (Hệ số 0.01 A)</p>
                </div>
              </div>
              <span className="text-xs px-2.5 py-1 bg-amber-50 text-amber-700 font-semibold rounded-full border border-amber-200 flex items-center gap-1">
                <Zap className="w-3.5 h-3.5" />
                Dòng tải thực tế
              </span>
            </div>

            <div className="grid grid-cols-2 sm:grid-cols-4 gap-2.5 my-2">
              {currentDevices.map((dev) => {
                const valNum = Number(dev.val || 0);
                const isActive = valNum > 0.05;

                return (
                  <div
                    key={dev.key}
                    className={`p-3 rounded-xl border transition-all ${
                      isActive 
                        ? 'bg-amber-50/60 border-amber-300 shadow-xs' 
                        : 'bg-slate-50 border-slate-200'
                    }`}
                  >
                    <div className="flex items-center justify-between mb-1">
                      <span className="text-xs font-bold text-slate-800 truncate" title={dev.label}>
                        {dev.label}
                      </span>
                      <span className="text-[10px] text-slate-400 font-mono">{dev.addr}</span>
                    </div>

                    <div className="flex items-baseline justify-between mt-1">
                      <span className={`text-xl font-extrabold ${isActive ? 'text-amber-600' : 'text-slate-500'}`}>
                        {valNum.toFixed(2)}
                      </span>
                      <span className="text-xs font-bold text-slate-400">A</span>
                    </div>

                    <div className="mt-1 flex items-center gap-1">
                      <span className={`w-1.5 h-1.5 rounded-full ${isActive ? 'bg-amber-500 animate-pulse' : 'bg-slate-300'}`}></span>
                      <span className="text-[10px] text-slate-500">
                        {isActive ? 'Đang tiêu thụ' : 'Tắt / Chờ'}
                      </span>
                    </div>
                  </div>
                );
              })}
            </div>
          </div>

          <p className="text-[11px] text-slate-400 mt-2">
            Giúp phát hiện sớm hiện tượng đứt dây, cháy tải hoặc kẹt bơm khi thiết bị bật nhưng không có dòng điện.
          </p>
        </div>
      </div>
    </div>
  );
};
