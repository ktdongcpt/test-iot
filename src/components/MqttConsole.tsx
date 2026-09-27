'use client';

import React, { useState, useEffect, useRef } from 'react';
import { Terminal, Send, Trash2, ArrowDownLeft, ArrowUpRight, Info } from 'lucide-react';
import { mqttService } from '@/lib/mqtt-client';

interface LogEntry {
  direction: 'IN' | 'OUT' | 'SYS';
  topic?: string;
  payload: string;
  time: string;
}

export const MqttConsole: React.FC = () => {
  const [logs, setLogs] = useState<LogEntry[]>([]);
  const [customMsg, setCustomMsg] = useState<string>('{"phun_suong_1": 1}');
  const [isOpen, setIsOpen] = useState<boolean>(true);
  const logContainerRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    const unsub = mqttService.onLog((entry) => {
      setLogs((prev) => {
        const next = [...prev, entry];
        if (next.length > 50) next.shift();
        return next;
      });
    });
    return () => unsub();
  }, []);

  useEffect(() => {
    if (logContainerRef.current) {
      logContainerRef.current.scrollTop = logContainerRef.current.scrollHeight;
    }
  }, [logs]);

  const handleSend = () => {
    if (!customMsg.trim()) return;
    try {
      const parsed = JSON.parse(customMsg);
      mqttService.publish(parsed);
    } catch (e) {
      mqttService.publish(customMsg);
    }
  };

  const handleTemplate = (tpl: string) => {
    setCustomMsg(tpl);
  };

  return (
    <div className="bg-white border border-slate-200 rounded-xl p-5 shadow-sm mb-6">
      <div className="flex items-center justify-between mb-4 pb-3 border-b border-slate-100">
        <div className="flex items-center gap-2">
          <Terminal className="w-5 h-5 text-slate-700" />
          <h2 className="text-base font-bold text-slate-800">
            NHẬT KÝ TRUYỀN THÔNG & TERMINAL LỆNH MQTT
          </h2>
        </div>

        <div className="flex items-center gap-2">
          <button
            onClick={() => setLogs([])}
            className="p-1.5 text-slate-400 hover:text-slate-700 hover:bg-slate-100 rounded-lg text-xs flex items-center gap-1"
            title="Xóa nhật ký"
          >
            <Trash2 className="w-3.5 h-3.5" />
            <span>Xóa log</span>
          </button>
        </div>
      </div>

      {/* Templates */}
      <div className="flex flex-wrap items-center gap-2 mb-3 text-xs">
        <span className="text-slate-500 font-medium">Lệnh mẫu nhanh:</span>
        <button
          onClick={() => handleTemplate('{"cmd": "get_schedules", "device": "all"}')}
          className="px-2.5 py-1 bg-slate-100 hover:bg-slate-200 rounded text-slate-700 font-mono"
        >
          get_schedules
        </button>
        <button
          onClick={() => handleTemplate('{"phun_suong_1": 1}')}
          className="px-2.5 py-1 bg-slate-100 hover:bg-slate-200 rounded text-slate-700 font-mono"
        >
          phun_suong_1 = 1
        </button>
        <button
          onClick={() => handleTemplate('{"suoi_1": 0}')}
          className="px-2.5 py-1 bg-slate-100 hover:bg-slate-200 rounded text-slate-700 font-mono"
        >
          suoi_1 = 0
        </button>
        <button
          onClick={() => handleTemplate('{"auto_humidity": 1, "hum_threshold": 75}')}
          className="px-2.5 py-1 bg-slate-100 hover:bg-slate-200 rounded text-slate-700 font-mono"
        >
          auto_humidity
        </button>
      </div>

      {/* Input gửi */}
      <div className="flex items-center gap-2 mb-4">
        <input
          type="text"
          value={customMsg}
          onChange={(e) => setCustomMsg(e.target.value)}
          onKeyDown={(e) => e.key === 'Enter' && handleSend()}
          placeholder='Nhập lệnh JSON gửi đến topic 3FAMIOT/HMI_SUB...'
          className="flex-1 px-3 py-2 bg-slate-50 border border-slate-300 rounded-lg text-xs font-mono text-slate-900 focus:outline-sky-500 focus:bg-white"
        />
        <button
          onClick={handleSend}
          className="px-4 py-2 bg-slate-800 hover:bg-slate-900 text-white text-xs font-semibold rounded-lg flex items-center gap-1.5 transition-colors"
        >
          <Send className="w-3.5 h-3.5" />
          <span>Gửi MQTT</span>
        </button>
      </div>

      {/* Terminal log window */}
      <div
        ref={logContainerRef}
        className="bg-slate-900 text-slate-200 font-mono text-xs p-4 rounded-xl h-56 overflow-y-auto space-y-1.5 leading-relaxed"
      >
        {logs.length === 0 ? (
          <div className="text-slate-500 italic">Đang chờ gói tin MQTT đến từ V-Box...</div>
        ) : (
          logs.map((entry, idx) => (
            <div key={idx} className="flex items-start gap-2">
              <span className="text-slate-500 text-[10px] shrink-0">{entry.time}</span>
              {entry.direction === 'IN' && (
                <span className="px-1 py-0.2 rounded text-[10px] bg-emerald-950 text-emerald-400 font-bold shrink-0 flex items-center gap-0.5">
                  <ArrowDownLeft className="w-3 h-3" /> NHẬN
                </span>
              )}
              {entry.direction === 'OUT' && (
                <span className="px-1 py-0.2 rounded text-[10px] bg-sky-950 text-sky-400 font-bold shrink-0 flex items-center gap-0.5">
                  <ArrowUpRight className="w-3 h-3" /> GỬI
                </span>
              )}
              {entry.direction === 'SYS' && (
                <span className="px-1 py-0.2 rounded text-[10px] bg-purple-950 text-purple-400 font-bold shrink-0 flex items-center gap-0.5">
                  <Info className="w-3 h-3" /> HỆ THỐNG
                </span>
              )}
              {entry.topic && (
                <span className="text-amber-400 text-[11px] font-semibold shrink-0">
                  [{entry.topic}]
                </span>
              )}
              <span className="text-slate-300 break-all">{entry.payload}</span>
            </div>
          ))
        )}
      </div>
    </div>
  );
};
