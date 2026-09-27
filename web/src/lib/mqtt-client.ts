'use client';

import mqtt, { MqttClient } from 'mqtt';
import { TelemetryState, DeviceKey, ScheduleSlot, AlarmItem } from './types';
import { logTelemetryToSupabase, logAlarmToSupabase } from './supabase';

const BROKER_URL = process.env.NEXT_PUBLIC_MQTT_BROKER_URL || 'wss://broker.emqx.io:8084/mqtt';
const TOPIC_PUB_DATA = process.env.NEXT_PUBLIC_MQTT_TOPIC_DATA || '3FAMIOT/HMI_PUB';
const TOPIC_SUB_CMD = process.env.NEXT_PUBLIC_MQTT_TOPIC_CMD || '3FAMIOT/HMI_SUB';
const TOPIC_TIME = process.env.NEXT_PUBLIC_MQTT_TOPIC_TIME || '3FAMIOT/TIME';
const TOPIC_ALARM = process.env.NEXT_PUBLIC_MQTT_TOPIC_ALARM || '3FAMIOT/ALARM';

class MqttService {
  private client: MqttClient | null = null;
  private isConnecting: boolean = false;
  private lastLogSaveTime: number = 0;

  // Listeners
  private telemetryCallbacks: ((data: TelemetryState) => void)[] = [];
  private alarmCallbacks: ((alarm: AlarmItem) => void)[] = [];
  private scheduleCallbacks: ((device: string, schedules: ScheduleSlot[]) => void)[] = [];
  private connectionCallbacks: ((connected: boolean) => void)[] = [];
  private logCallbacks: ((log: { direction: 'IN' | 'OUT' | 'SYS'; topic?: string; payload: string; time: string }) => void)[] = [];

  // State cache
  public state: TelemetryState = {
    temperature: 0,
    humidity: 0,
    vboxHeartbeat: 0,
    plcHeartbeat: 0,
    heartbeatAlive: false,
    plcCommOk: true,
    plcAlarmMsg: '',
    tempThreshold: 25.0,
    humThreshold: 70.0,
    psOnSec: 120,
    psOffSec: 900,
    psOnTime: 2.0,
    psOffTime: 15.0,
    psCycleState: 0,
    autoTempMode: false,
    autoHumidityMode: false,
    vboxTime: '--:--:--',
    vboxDate: '--/--/----',
    devices: {
      phun_suong_1: false,
      phun_suong_2: false,
      phun_suong_3: false,
      phun_suong_4: false,
      den_1: false,
      den_2: false,
      suoi_1: false,
      loa_1: false,
      loa_2: false,
      loa_3: false,
    },
    mqttConnected: false,
    lastReceived: 'Chưa có dữ liệu từ V-Box...',
    lastPublished: 'Chưa có lệnh gửi đi',
    lastReceivedAt: 0,
  };

  public connect() {
    if (typeof window === 'undefined') return;
    if (this.client && this.client.connected) return;
    if (this.isConnecting) return;

    this.isConnecting = true;
    const clientId = 'NextJS_VBoxClient_' + Math.random().toString(16).substring(2, 10);

    this.emitLog('SYS', `Đang kết nối MQTT Broker (${BROKER_URL})...`);

    try {
      this.client = mqtt.connect(BROKER_URL, {
        clientId,
        clean: true,
        connectTimeout: 8000,
        reconnectPeriod: 4000,
        keepalive: 60,
      });

      this.client.on('connect', () => {
        this.isConnecting = false;
        this.state.mqttConnected = true;
        this.emitConnection(true);
        this.emitLog('SYS', `Đã kết nối MQTT Broker thành công! ClientID: ${clientId}`);

        // Subscribe topics
        this.client?.subscribe([TOPIC_PUB_DATA, TOPIC_TIME, TOPIC_ALARM], (err) => {
          if (!err) {
            this.emitLog('SYS', `Đã đăng ký (Subscribe): ${TOPIC_PUB_DATA}, ${TOPIC_TIME}, ${TOPIC_ALARM}`);
          }
        });
      });

      this.client.on('message', (topic, payloadBuffer) => {
        const payloadStr = payloadBuffer.toString();
        this.handleIncomingMessage(topic, payloadStr);
      });

      this.client.on('error', (err) => {
        this.emitLog('SYS', `Lỗi kết nối MQTT: ${err.message}`);
      });

      this.client.on('close', () => {
        this.isConnecting = false;
        if (this.state.mqttConnected) {
          this.state.mqttConnected = false;
          this.emitConnection(false);
          this.emitLog('SYS', 'Mất kết nối MQTT Broker, đang thử lại...');
        }
      });
    } catch (err: any) {
      this.isConnecting = false;
      this.emitLog('SYS', `Không thể khởi tạo MQTT: ${err?.message}`);
    }
  }

  private handleIncomingMessage(topic: string, msg: string) {
    this.emitLog('IN', msg, topic);
    this.state.lastReceived = msg;
    this.state.lastReceivedAt = Date.now();

    try {
      const data = JSON.parse(msg);

      // 1. Nhận thời gian thực
      if (topic === TOPIC_TIME) {
        if (data.time) this.state.vboxTime = data.time;
        if (data.date) this.state.vboxDate = data.date;
        this.emitTelemetry();
        return;
      }

      // 2. Nhận cảnh báo tức thời RS485 (3FAMIOT/ALARM)
      if (topic === TOPIC_ALARM) {
        const commOk = data.plc_comm === 1 || data.status === 'CONNECTED';
        this.state.plcCommOk = commOk;
        this.state.plcAlarmMsg = data.message || (commOk ? '' : 'MẤT TRUYỀN THÔNG RS485 VỚI PLC!');
        if (data.plc_heartbeat !== undefined) this.state.plcHeartbeat = Number(data.plc_heartbeat);

        const alarmItem: AlarmItem = {
          id: Math.random().toString(36).substring(2),
          time: data.time || new Date().toLocaleTimeString(),
          message: this.state.plcAlarmMsg,
          type: data.type || 'COMM_ALARM',
          status: commOk ? 'CONNECTED' : 'DISCONNECTED',
          plc_heartbeat: data.plc_heartbeat,
        };
        this.emitAlarm(alarmItem);
        logAlarmToSupabase({
          alarm_type: alarmItem.type,
          severity: commOk ? 'info' : 'critical',
          source: data.source || 'V-BOX',
          message: alarmItem.message,
          plc_heartbeat: alarmItem.plc_heartbeat,
          plc_comm: commOk ? 1 : 0,
        });

        this.emitTelemetry();
        return;
      }

      // 3. Nhận dữ liệu trạng thái (3FAMIOT/HMI_PUB)
      if (topic === TOPIC_PUB_DATA) {
        // Phản hồi danh sách khung giờ thực tế từ V-Box
        if (data.type === 'VBOX_SCHEDULES_RESP') {
          if (data.device && data.schedules) {
            this.emitSchedules(data.device, data.schedules);
          } else if (data.all) {
            for (const [devKey, devInfo] of Object.entries<any>(data.all)) {
              if (devInfo.schedules) {
                this.emitSchedules(devKey, devInfo.schedules);
              }
            }
          }
          return;
        }

        // Cảm biến nhiệt độ & độ ẩm
        const temp = data.Tempeturate ?? data.Temperature ?? data.temp;
        const hum = data.Humidity ?? data.hum;
        if (temp !== undefined) this.state.temperature = Number(temp);
        if (hum !== undefined) this.state.humidity = Number(hum);

        // Heartbeat V-Box
        const hb = data.heartbeat ?? data.kt_ket_noi;
        if (hb !== undefined) {
          this.state.vboxHeartbeat = Number(hb);
          this.state.heartbeatAlive = true;
        }

        // Heartbeat PLC
        if (data.plc_heartbeat !== undefined) {
          this.state.plcHeartbeat = Number(data.plc_heartbeat);
        }

        // Trạng thái truyền thông RS485
        if (data.plc_comm !== undefined) {
          this.state.plcCommOk = Number(data.plc_comm) === 1;
        } else if (data.plc_status !== undefined) {
          this.state.plcCommOk = data.plc_status === 'OK';
        }
        if (!this.state.plcCommOk) {
          this.state.plcAlarmMsg = data.alarm || 'MẤT TRUYỀN THÔNG RS485 VỚI PLC (>15s)';
        } else {
          this.state.plcAlarmMsg = '';
        }

        // Cài đặt ngưỡng & chế độ tự động
        if (data.temp_threshold !== undefined) this.state.tempThreshold = Number(data.temp_threshold);
        if (data.hum_threshold !== undefined) this.state.humThreshold = Number(data.hum_threshold);
        if (data.ps_on_sec !== undefined) this.state.psOnSec = Number(data.ps_on_sec);
        if (data.ps_off_sec !== undefined) this.state.psOffSec = Number(data.ps_off_sec);
        if (data.ps_on_time !== undefined) this.state.psOnTime = Number(data.ps_on_time);
        if (data.ps_off_time !== undefined) this.state.psOffTime = Number(data.ps_off_time);
        if (data.ps_cycle_state !== undefined) this.state.psCycleState = Number(data.ps_cycle_state);
        if (data.auto_temp !== undefined) this.state.autoTempMode = Number(data.auto_temp) === 1;
        if (data.auto_humidity !== undefined) this.state.autoHumidityMode = Number(data.auto_humidity) === 1;

        // Trạng thái 10 thiết bị phản hồi
        const devKeys: DeviceKey[] = [
          'phun_suong_1', 'phun_suong_2', 'phun_suong_3', 'phun_suong_4',
          'den_1', 'den_2', 'suoi_1', 'loa_1', 'loa_2', 'loa_3',
        ];
        devKeys.forEach((k) => {
          if (data[k] !== undefined) {
            this.state.devices[k] = Number(data[k]) === 1;
          }
        });

        // Hỗ trợ alias cũ
        if (data.fan !== undefined && data.phun_suong_2 === undefined) this.state.devices.phun_suong_2 = Number(data.fan) === 1;
        if (data.pump !== undefined && data.phun_suong_3 === undefined) this.state.devices.phun_suong_3 = Number(data.pump) === 1;
        if (data.light !== undefined && data.den_1 === undefined) this.state.devices.den_1 = Number(data.light) === 1;

        this.emitTelemetry();

        // Định kỳ lưu log vào Supabase (mỗi 10 giây 1 lần để tiết kiệm quota)
        const now = Date.now();
        if (now - this.lastLogSaveTime > 10000) {
          this.lastLogSaveTime = now;
          const deviceStatesNumeric: Record<string, number> = {};
          devKeys.forEach(k => { deviceStatesNumeric[k] = this.state.devices[k] ? 1 : 0; });
          logTelemetryToSupabase({
            temperature: this.state.temperature,
            humidity: this.state.humidity,
            vbox_heartbeat: this.state.vboxHeartbeat,
            plc_heartbeat: this.state.plcHeartbeat,
            plc_comm_ok: this.state.plcCommOk,
            ps_cycle_state: this.state.psCycleState,
            device_states: deviceStatesNumeric,
          });
        }
      }
    } catch (e: any) {
      console.warn('Lỗi phân tích JSON MQTT:', e);
    }
  }

  // Gửi lệnh xuống V-Box
  public publish(payload: object | string): boolean {
    if (!this.client || !this.client.connected) {
      this.emitLog('SYS', 'Không thể gửi: Mất kết nối MQTT Broker!');
      return false;
    }
    const message = typeof payload === 'string' ? payload : JSON.stringify(payload);
    this.client.publish(TOPIC_SUB_CMD, message, { qos: 0 });
    this.state.lastPublished = message;
    this.emitLog('OUT', message, TOPIC_SUB_CMD);
    return true;
  }

  // Điều khiển thiết bị bật/tắt thủ công
  public setDeviceState(key: DeviceKey, state: boolean): boolean {
    return this.publish({ [key]: state ? 1 : 0 });
  }

  // Điều khiển thiết bị chạy hẹn giờ (duration tính theo giây)
  public setDeviceTimer(key: DeviceKey, durationSec: number): boolean {
    return this.publish({ device: key, duration: durationSec, [key]: 1 });
  }

  // Điều khiển thiết bị chạy chu kỳ tuần hoàn (onSec / offSec)
  public setDeviceCycle(key: DeviceKey, onSec: number, offSec: number): boolean {
    return this.publish({ device: key, mode: 'cycle', on_time: onSec, off_time: offSec, [key]: 'cycle' });
  }

  // Nạp danh sách khung giờ hẹn cho thiết bị
  public setDeviceSchedules(key: DeviceKey, schedules: ScheduleSlot[]): boolean {
    return this.publish({
      device: key,
      mode: 'schedule',
      schedules: schedules.map((s) => ({
        start: s.start,
        stop: s.stop,
        day: s.day || 0,
        month: s.month || 0,
        year: s.year || 0,
        enable: s.enable !== false ? 1 : 0,
      })),
    });
  }

  // Yêu cầu V-Box gửi danh sách khung giờ thực tế đang chạy
  public queryDeviceSchedules(deviceKey: string = 'all'): boolean {
    return this.publish({ cmd: 'get_schedules', device: deviceKey });
  }

  // Cài đặt ngưỡng và chế độ tự động
  public updateAutoSettings(settings: {
    auto_temp?: boolean;
    auto_humidity?: boolean;
    temp_threshold?: number;
    hum_threshold?: number;
    ps_on_sec?: number;
    ps_off_sec?: number;
  }): boolean {
    const payload: any = {};
    if (settings.auto_temp !== undefined) payload.auto_temp = settings.auto_temp ? 1 : 0;
    if (settings.auto_humidity !== undefined) payload.auto_humidity = settings.auto_humidity ? 1 : 0;
    if (settings.temp_threshold !== undefined) payload.temp_threshold = settings.temp_threshold;
    if (settings.hum_threshold !== undefined) payload.hum_threshold = settings.hum_threshold;
    if (settings.ps_on_sec !== undefined) payload.ps_on_sec = settings.ps_on_sec;
    if (settings.ps_off_sec !== undefined) payload.ps_off_sec = settings.ps_off_sec;
    return this.publish(payload);
  }

  // Event handlers
  public onTelemetry(cb: (data: TelemetryState) => void) {
    this.telemetryCallbacks.push(cb);
    cb(this.state);
    return () => {
      this.telemetryCallbacks = this.telemetryCallbacks.filter((c) => c !== cb);
    };
  }

  public onAlarm(cb: (alarm: AlarmItem) => void) {
    this.alarmCallbacks.push(cb);
    return () => {
      this.alarmCallbacks = this.alarmCallbacks.filter((c) => c !== cb);
    };
  }

  public onSchedules(cb: (device: string, schedules: ScheduleSlot[]) => void) {
    this.scheduleCallbacks.push(cb);
    return () => {
      this.scheduleCallbacks = this.scheduleCallbacks.filter((c) => c !== cb);
    };
  }

  public onConnection(cb: (connected: boolean) => void) {
    this.connectionCallbacks.push(cb);
    cb(this.state.mqttConnected);
    return () => {
      this.connectionCallbacks = this.connectionCallbacks.filter((c) => c !== cb);
    };
  }

  public onLog(cb: (log: { direction: 'IN' | 'OUT' | 'SYS'; topic?: string; payload: string; time: string }) => void) {
    this.logCallbacks.push(cb);
    return () => {
      this.logCallbacks = this.logCallbacks.filter((c) => c !== cb);
    };
  }

  private emitTelemetry() {
    this.telemetryCallbacks.forEach((cb) => cb({ ...this.state }));
  }

  private emitAlarm(alarm: AlarmItem) {
    this.alarmCallbacks.forEach((cb) => cb(alarm));
  }

  private emitSchedules(device: string, schedules: ScheduleSlot[]) {
    this.scheduleCallbacks.forEach((cb) => cb(device, schedules));
  }

  private emitConnection(connected: boolean) {
    this.connectionCallbacks.forEach((cb) => cb(connected));
  }

  private emitLog(direction: 'IN' | 'OUT' | 'SYS', payload: string, topic?: string) {
    const entry = {
      direction,
      topic,
      payload,
      time: new Date().toLocaleTimeString(),
    };
    this.logCallbacks.forEach((cb) => cb(entry));
  }
}

export const mqttService = new MqttService();
