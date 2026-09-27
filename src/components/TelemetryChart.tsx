'use client';

import React, { useState, useEffect } from 'react';
import { LineChart, Thermometer, Droplets, RefreshCw } from 'lucide-react';
import { supabase } from '@/lib/supabase';

interface TelemetryPoint {
  time: string;
  temp: number;
  hum: number;
}

interface TelemetryChartProps {
  currentTemp: number;
  currentHum: number;
}

export const TelemetryChart: React.FC<TelemetryChartProps> = ({ currentTemp, currentHum }) => {
  const [dataPoints, setDataPoints] = useState<TelemetryPoint[]>([]);

  // Thu thập điểm dữ liệu mới mỗi khi nhiệt/ẩm thay đổi
  useEffect(() => {
    if (currentTemp === 0 && currentHum === 0) return;

    const timeStr = new Date().toLocaleTimeString('vi-VN', { hour: '2-digit', minute: '2-digit', second: '2-digit' });
    setDataPoints((prev) => {
      const next = [...prev, { time: timeStr, temp: currentTemp, hum: currentHum }];
      if (next.length > 20) next.shift(); // Giữ tối đa 20 điểm gần nhất
      return next;
    });
  }, [currentTemp, currentHum]);

  // Tải dữ liệu ban đầu từ Supabase nếu có
  const fetchSupabaseHistory = async () => {
    if (!supabase) return;
    try {
      const { data, error } = await supabase
        .from('telemetry_logs')
        .select('temperature, humidity, recorded_at')
        .order('recorded_at', { ascending: false })
        .limit(20);

      if (!error && data && data.length > 0) {
        const loaded: TelemetryPoint[] = data.reverse().map((item: any) => ({
          time: new Date(item.recorded_at).toLocaleTimeString('vi-VN', { hour: '2-digit', minute: '2-digit' }),
          temp: Number(item.temperature),
          hum: Number(item.humidity),
        }));
        setDataPoints(loaded);
      }
    } catch (e) {
      console.error(e);
    }
  };

  useEffect(() => {
    fetchSupabaseHistory();
  }, []);

  // Tính tọa độ SVG đơn giản cho biểu đồ
  const width = 600;
  const height = 180;
  const padding = 30;

  const minTemp = 15;
  const maxTemp = 45;
  const minHum = 20;
  const maxHum = 100;

  const getYTemp = (val: number) => {
    const clamped = Math.max(minTemp, Math.min(maxTemp, val));
    return height - padding - ((clamped - minTemp) / (maxTemp - minTemp)) * (height - 2 * padding);
  };

  const getYHum = (val: number) => {
    const clamped = Math.max(minHum, Math.min(maxHum, val));
    return height - padding - ((clamped - minHum) / (maxHum - minHum)) * (height - 2 * padding);
  };

  const getX = (index: number, total: number) => {
    if (total <= 1) return padding;
    return padding + (index / (total - 1)) * (width - 2 * padding);
  };

  const tempPath = dataPoints
    .map((p, i) => `${i === 0 ? 'M' : 'L'} ${getX(i, dataPoints.length)} ${getYTemp(p.temp)}`)
    .join(' ');

  const humPath = dataPoints
    .map((p, i) => `${i === 0 ? 'M' : 'L'} ${getX(i, dataPoints.length)} ${getYHum(p.hum)}`)
    .join(' ');

  return (
    <div className="bg-white border border-slate-200 rounded-xl p-5 shadow-sm mb-6">
      <div className="flex items-center justify-between mb-4 pb-3 border-b border-slate-100">
        <div className="flex items-center gap-2">
          <LineChart className="w-5 h-5 text-indigo-600" />
          <h2 className="text-base font-bold text-slate-800">
            DIỄN BIẾN NHIỆT ĐỘ & ĐỘ ẨM THỜI GIAN THỰC
          </h2>
        </div>

        <div className="flex items-center gap-4 text-xs font-semibold">
          <div className="flex items-center gap-1.5 text-amber-600">
            <span className="w-3 h-3 rounded-full bg-amber-500 inline-block"></span>
            <span>Nhiệt độ (°C)</span>
          </div>
          <div className="flex items-center gap-1.5 text-sky-600">
            <span className="w-3 h-3 rounded-full bg-sky-500 inline-block"></span>
            <span>Độ ẩm (%)</span>
          </div>
          <button
            onClick={fetchSupabaseHistory}
            className="p-1 hover:bg-slate-100 rounded text-slate-400 hover:text-slate-600"
            title="Tải lại từ Supabase"
          >
            <RefreshCw className="w-3.5 h-3.5" />
          </button>
        </div>
      </div>

      {dataPoints.length < 2 ? (
        <div className="h-40 flex items-center justify-center text-xs text-slate-400">
          Đang thu thập dữ liệu từ V-Box để vẽ biểu đồ...
        </div>
      ) : (
        <div className="w-full overflow-x-auto">
          <svg viewBox={`0 0 ${width} ${height}`} className="w-full h-44 select-none">
            {/* Grid lines */}
            <line x1={padding} y1={padding} x2={width - padding} y2={padding} stroke="#f1f5f9" strokeDasharray="3 3" />
            <line x1={padding} y1={height / 2} x2={width - padding} y2={height / 2} stroke="#f1f5f9" strokeDasharray="3 3" />
            <line x1={padding} y1={height - padding} x2={width - padding} y2={height - padding} stroke="#e2e8f0" />

            {/* Lines */}
            <path d={tempPath} fill="none" stroke="#f59e0b" strokeWidth="2.5" strokeLinecap="round" />
            <path d={humPath} fill="none" stroke="#0284c7" strokeWidth="2.5" strokeLinecap="round" />

            {/* Dots */}
            {dataPoints.map((p, i) => (
              <g key={i}>
                <circle cx={getX(i, dataPoints.length)} cy={getYTemp(p.temp)} r="3.5" fill="#f59e0b" />
                <circle cx={getX(i, dataPoints.length)} cy={getYHum(p.hum)} r="3.5" fill="#0284c7" />
              </g>
            ))}
          </svg>

          {/* Time axis labels */}
          <div className="flex justify-between text-[10px] text-slate-400 px-6 mt-1 font-mono">
            <span>{dataPoints[0]?.time}</span>
            <span>{dataPoints[Math.floor(dataPoints.length / 2)]?.time}</span>
            <span>{dataPoints[dataPoints.length - 1]?.time}</span>
          </div>
        </div>
      )}
    </div>
  );
};
