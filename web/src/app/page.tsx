'use client';

import React, { useState, useEffect } from 'react';
import { mqttService } from '@/lib/mqtt-client';
import { TelemetryState, DeviceConfig } from '@/lib/types';
import { ConnectionBar } from '@/components/ConnectionBar';
import { AlarmBanner } from '@/components/AlarmBanner';
import { EnvironmentalPanel } from '@/components/EnvironmentalPanel';
import { AutoControlPanel } from '@/components/AutoControlPanel';
import { DeviceGrid } from '@/components/DeviceGrid';
import { ScheduleModal } from '@/components/ScheduleModal';
import { TelemetryChart } from '@/components/TelemetryChart';
import { MqttConsole } from '@/components/MqttConsole';
import { Activity, ShieldAlert, Cpu } from 'lucide-react';

export default function DashboardPage() {
  const [telemetry, setTelemetry] = useState<TelemetryState>(mqttService.state);
  const [selectedDeviceForSchedule, setSelectedDeviceForSchedule] = useState<DeviceConfig | null>(null);

  useEffect(() => {
    // Khởi động kết nối MQTT tới EMQX WebSocket
    mqttService.connect();

    // Lắng nghe cập nhật telemetry
    const unsubTelemetry = mqttService.onTelemetry((data) => {
      setTelemetry({ ...data });
    });

    // Lắng nghe kết nối
    const unsubConn = mqttService.onConnection((conn) => {
      setTelemetry((prev) => ({ ...prev, mqttConnected: conn }));
    });

    return () => {
      unsubTelemetry();
      unsubConn();
    };
  }, []);

  const anyMistActive = Boolean(
    telemetry.devices.phun_suong_1 ||
    telemetry.devices.phun_suong_2 ||
    telemetry.devices.phun_suong_3 ||
    telemetry.devices.phun_suong_4
  );

  return (
    <main className="min-h-screen p-4 sm:p-6 lg:p-8 max-w-7xl mx-auto">
      {/* Header trang */}
      <header className="mb-6 flex flex-col sm:flex-row sm:items-center sm:justify-between gap-4">
        <div>
          <div className="flex items-center gap-2">
            <div className="p-2 bg-sky-600 text-white rounded-xl shadow-sm">
              <Cpu className="w-6 h-6" />
            </div>
            <div>
              <h1 className="text-xl sm:text-2xl font-black tracking-tight text-slate-900">
                V-BOX & PLC SMART CONTROLLER
              </h1>
              <p className="text-xs text-slate-500 font-medium">
                Hệ thống Web SCADA giám sát truyền thông RS485 & điều khiển 10 thiết bị qua Internet
              </p>
            </div>
          </div>
        </div>

        <div className="flex items-center gap-3">
          <span className="text-xs px-3 py-1.5 bg-slate-100 border border-slate-200 text-slate-600 rounded-full font-mono">
            V-Box: <b className="text-slate-800">wecon-gateway</b>
          </span>
          <span className="text-xs px-3 py-1.5 bg-sky-50 border border-sky-200 text-sky-700 rounded-full font-semibold">
            Next.js Cloud Web
          </span>
        </div>
      </header>

      {/* 1. Banner cảnh báo khẩn cấp khi mất RS485 */}
      <AlarmBanner
        plcCommOk={telemetry.plcCommOk}
        plcAlarmMsg={telemetry.plcAlarmMsg}
        plcHeartbeat={telemetry.plcHeartbeat}
      />

      {/* 2. Thanh kết nối và 2 Heartbeat */}
      <ConnectionBar
        mqttConnected={telemetry.mqttConnected}
        vboxHeartbeat={telemetry.vboxHeartbeat}
        plcHeartbeat={telemetry.plcHeartbeat}
        plcCommOk={telemetry.plcCommOk}
        vboxTime={telemetry.vboxTime}
        vboxDate={telemetry.vboxDate}
      />

      {/* 3. Panel Cảm biến Nhiệt độ / Độ ẩm */}
      <EnvironmentalPanel
        temperature={telemetry.temperature}
        humidity={telemetry.humidity}
        tempThreshold={telemetry.tempThreshold}
        humThreshold={telemetry.humThreshold}
        autoTempMode={telemetry.autoTempMode}
        autoHumidityMode={telemetry.autoHumidityMode}
        psCycleState={telemetry.psCycleState}
        psOnSec={telemetry.psOnSec}
        psOffSec={telemetry.psOffSec}
        stateSuoi1={Boolean(telemetry.devices.suoi_1)}
        statePhunSuongAny={anyMistActive}
      />

      {/* 4. Panel Cài đặt tự động theo cảm biến */}
      <AutoControlPanel
        autoTempMode={telemetry.autoTempMode}
        autoHumidityMode={telemetry.autoHumidityMode}
        tempThreshold={telemetry.tempThreshold}
        humThreshold={telemetry.humThreshold}
        psOnSec={telemetry.psOnSec}
        psOffSec={telemetry.psOffSec}
      />

      {/* 5. Bảng điều khiển và phản hồi 10 thiết bị */}
      <DeviceGrid
        devicesState={telemetry.devices}
        autoTempMode={telemetry.autoTempMode}
        autoHumidityMode={telemetry.autoHumidityMode}
        onOpenSchedule={(dev) => setSelectedDeviceForSchedule(dev)}
      />

      {/* 6. Biểu đồ lịch sử nhiệt độ / độ ẩm */}
      <TelemetryChart
        currentTemp={telemetry.temperature}
        currentHum={telemetry.humidity}
      />

      {/* 7. Terminal Console MQTT */}
      <MqttConsole />

      {/* 8. Modal cài đặt lịch 24 khung giờ */}
      <ScheduleModal
        device={selectedDeviceForSchedule}
        onClose={() => setSelectedDeviceForSchedule(null)}
      />
    </main>
  );
}
