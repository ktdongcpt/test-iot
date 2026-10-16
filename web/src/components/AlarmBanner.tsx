'use client';

import React from 'react';
import { AlertTriangle, WifiOff, RadioTower } from 'lucide-react';

interface AlarmBannerProps {
  vboxCommOk?: boolean;
  vboxAlarmMsg?: string;
  vboxHeartbeat?: number;
  plcCommOk: boolean;
  plcAlarmMsg: string;
  plcHeartbeat: number;
}

export const AlarmBanner: React.FC<AlarmBannerProps> = ({
  vboxCommOk = true,
  vboxAlarmMsg = '',
  vboxHeartbeat = 0,
  plcCommOk,
  plcAlarmMsg,
  plcHeartbeat,
}) => {
  if (vboxCommOk && plcCommOk) return null;

  return (
    <div className="w-full mb-6 space-y-3 transition-all">
      {/* 1. CẢNH BÁO MẤT KẾT NỐI V-BOX GATEWAY (MẤT MẠNG HOẶC MẤT NGUỒN) */}
      {!vboxCommOk && (
        <div className="alarm-alert-box w-full p-4 bg-amber-50 border-2 border-amber-500 rounded-xl shadow-md flex items-start sm:items-center gap-4 text-amber-950 animate-pulse">
          <div className="p-3 bg-amber-100 rounded-full text-amber-700 shrink-0">
            <WifiOff className="w-8 h-8" />
          </div>

          <div className="flex-1">
            <div className="flex flex-wrap items-center gap-2">
              <span className="px-2 py-0.5 text-xs font-bold uppercase tracking-wider bg-amber-600 text-white rounded">
                CẢNH BÁO MẤT TÍN HIỆU
              </span>
              <span className="text-sm font-bold text-amber-900">
                MẤT KẾT NỐI VỚI V-BOX GATEWAY (OFFLINE / MẤT MẠNG)
              </span>
            </div>
            <p className="text-sm text-amber-900 mt-1 font-medium leading-relaxed">
              {vboxAlarmMsg || 'Hơn 15 giây không nhận được tín hiệu Heartbeat từ V-Box Gateway (@W_0#HDW12). V-Box có thể bị mất điện, mất sóng Wi-Fi hoặc mất mạng 4G.'}
            </p>
            <div className="text-xs text-amber-800 mt-1.5 flex flex-wrap items-center gap-3 font-semibold">
              <span>• Heartbeat V-Box gần nhất: <b>{vboxHeartbeat}/60</b></span>
              <span>• Biện pháp: <b>Kiểm tra đèn nguồn Power và đèn mạng NET trên V-BOX</b></span>
            </div>
          </div>
        </div>
      )}

      {/* 2. CẢNH BÁO MẤT TRUYỀN THÔNG RS485 VỚI PLC */}
      {!plcCommOk && (
        <div className="alarm-alert-box w-full p-4 bg-red-50 border-2 border-red-500 rounded-xl shadow-md flex items-start sm:items-center gap-4 text-red-900 animate-pulse">
          <div className="p-3 bg-red-100 rounded-full text-red-600 shrink-0">
            <AlertTriangle className="w-8 h-8" />
          </div>

          <div className="flex-1">
            <div className="flex flex-wrap items-center gap-2">
              <span className="px-2 py-0.5 text-xs font-bold uppercase tracking-wider bg-red-600 text-white rounded">
                CẢNH BÁO NGUY HIỂM
              </span>
              <span className="text-sm font-bold text-red-800">
                MẤT TRUYỀN THÔNG RS485 VỚI PLC
              </span>
            </div>
            <p className="text-sm text-red-800 mt-1 font-medium leading-relaxed">
              {plcAlarmMsg || 'Heartbeat PLC (@W_0#HDW13) đứng yên quá 15 giây! Hệ thống không nhận được tín hiệu phản hồi từ PLC.'}
            </p>
            <div className="text-xs text-red-600 mt-1.5 flex flex-wrap items-center gap-3 font-semibold">
              <span>• Heartbeat PLC cuối cùng: <b>{plcHeartbeat}/60</b></span>
              <span>• Tình trạng: <b>Đã kích hoạt cờ báo động LB100</b></span>
            </div>
          </div>
        </div>
      )}
    </div>
  );
};
