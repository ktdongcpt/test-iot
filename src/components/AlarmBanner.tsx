'use client';

import React from 'react';
import { AlertTriangle, WifiOff, BellRing, VolumeX } from 'lucide-react';

interface AlarmBannerProps {
  plcCommOk: boolean;
  plcAlarmMsg: string;
  plcHeartbeat: number;
}

export const AlarmBanner: React.FC<AlarmBannerProps> = ({
  plcCommOk,
  plcAlarmMsg,
  plcHeartbeat,
}) => {
  if (plcCommOk) return null;

  return (
    <div className="alarm-alert-box w-full mb-6 p-4 bg-red-50 border-2 border-red-500 rounded-xl shadow-md flex items-start sm:items-center gap-4 text-red-900 transition-all">
      <div className="p-3 bg-red-100 rounded-full text-red-600 shrink-0">
        <AlertTriangle className="w-8 h-8 animate-bounce" />
      </div>

      <div className="flex-1">
        <div className="flex items-center gap-2">
          <span className="px-2 py-0.5 text-xs font-bold uppercase tracking-wider bg-red-600 text-white rounded">
            CẢNH BÁO NGUY HIỂM
          </span>
          <span className="text-sm font-semibold text-red-700">
            MẤT TRUYỀN THÔNG RS485 VỚI PLC
          </span>
        </div>
        <p className="text-sm text-red-800 mt-1 font-medium leading-relaxed">
          {plcAlarmMsg || 'Heartbeat PLC (@W_0#HDW13) đứng yên quá 15 giây! Hệ thống không nhận được tín hiệu phản hồi từ PLC.'}
        </p>
        <div className="text-xs text-red-600 mt-1 flex items-center gap-2">
          <span>• Giá trị Heartbeat PLC cuối cùng: <b>{plcHeartbeat}/60</b></span>
          <span>• Tình trạng: <b>Đã kích hoạt cờ báo động LB100</b></span>
        </div>
      </div>
    </div>
  );
};
