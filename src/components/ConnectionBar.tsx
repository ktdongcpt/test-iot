'use client';

import React from 'react';
import { Wifi, Activity, Cpu, Clock, RefreshCw, AlertCircle, CheckCircle2 } from 'lucide-react';
import { mqttService } from '@/lib/mqtt-client';

interface ConnectionBarProps {
  mqttConnected: boolean;
  vboxHeartbeat: number;
  plcHeartbeat: number;
  plcCommOk: boolean;
  vboxTime: string;
  vboxDate: string;
}

export const ConnectionBar: React.FC<ConnectionBarProps> = ({
  mqttConnected,
  vboxHeartbeat,
  plcHeartbeat,
  plcCommOk,
  vboxTime,
  vboxDate,
}) => {
  return (
    <div className="w-full bg-white border border-slate-200 rounded-xl p-3 shadow-sm mb-6 flex flex-wrap items-center justify-between gap-3">
      {/* Danh sách các Badge giám sát */}
      <div className="flex flex-wrap items-center gap-2 text-xs font-medium">
        {/* Badge MQTT */}
        <div
          className={`flex items-center gap-1.5 px-3 py-1.5 rounded-full border ${
            mqttConnected
              ? 'bg-emerald-50 text-emerald-700 border-emerald-200'
              : 'bg-rose-50 text-rose-700 border-rose-200 animate-pulse'
          }`}
        >
          <Wifi className="w-3.5 h-3.5" />
          <span>MQTT: {mqttConnected ? 'broker.emqx.io' : 'Mất kết nối'}</span>
        </div>

        {/* Badge V-Box Heartbeat */}
        <div className="flex items-center gap-1.5 px-3 py-1.5 rounded-full bg-purple-50 text-purple-700 border border-purple-200">
          <Activity className="w-3.5 h-3.5" />
          <span>V-Box HB: <b className="font-semibold">{vboxHeartbeat}/60</b> (@HDW12)</span>
        </div>

        {/* Badge PLC Heartbeat */}
        <div
          className={`flex items-center gap-1.5 px-3 py-1.5 rounded-full border ${
            plcCommOk
              ? 'bg-blue-50 text-blue-700 border-blue-200'
              : 'bg-rose-50 text-rose-700 border-rose-300 animate-pulse'
          }`}
        >
          <Cpu className="w-3.5 h-3.5" />
          <span>PLC HB: <b className="font-semibold">{plcHeartbeat}/60</b> (@HDW13)</span>
        </div>

        {/* Badge Truyền thông RS485 */}
        <div
          className={`flex items-center gap-1.5 px-3 py-1.5 rounded-full border ${
            plcCommOk
              ? 'bg-emerald-50 text-emerald-700 border-emerald-200'
              : 'bg-red-50 text-red-700 border-red-300 font-bold animate-pulse'
          }`}
        >
          {plcCommOk ? (
            <CheckCircle2 className="w-3.5 h-3.5 text-emerald-600" />
          ) : (
            <AlertCircle className="w-3.5 h-3.5 text-red-600" />
          )}
          <span>RS485: {plcCommOk ? 'Kết nối tốt' : 'MẤT TRUYỀN THÔNG!'}</span>
        </div>

        {/* Đồng hồ RTC V-Box */}
        <div className="flex items-center gap-1.5 px-3 py-1.5 rounded-full bg-slate-100 text-slate-700 border border-slate-200">
          <Clock className="w-3.5 h-3.5 text-slate-500" />
          <span>V-Box RTC: <b>{vboxTime}</b> {vboxDate !== '--/--/----' && `• ${vboxDate}`}</span>
        </div>
      </div>

      {/* Nút bấm làm mới / thử lại kết nối */}
      <div className="flex items-center gap-2">
        <button
          onClick={() => {
            mqttService.connect();
            mqttService.queryDeviceSchedules('all');
          }}
          className="flex items-center gap-1 text-xs px-3 py-1.5 bg-slate-100 hover:bg-slate-200 text-slate-700 rounded-lg transition-colors font-medium border border-slate-200"
          title="Thử kết nối lại hoặc đồng bộ lịch"
        >
          <RefreshCw className="w-3.5 h-3.5" />
          <span>Đồng bộ</span>
        </button>
      </div>
    </div>
  );
};
