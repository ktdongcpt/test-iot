export type DeviceKey =
  | 'phun_suong_1'
  | 'phun_suong_2'
  | 'phun_suong_3'
  | 'phun_suong_4'
  | 'den_1'
  | 'den_2'
  | 'suoi_1'
  | 'loa_1'
  | 'loa_2'
  | 'loa_3';

export interface DeviceConfig {
  key: DeviceKey;
  name: string;
  category: 'mist' | 'light' | 'heater' | 'speaker';
  cmdAddr: string;
  statusAddr: string;
  maxSlots: number; // 24 for mist, 10 for others
  iconName: string;
}

export const DEVICE_DEFINITIONS: DeviceConfig[] = [
  { key: 'phun_suong_1', name: 'Phun sương 1', category: 'mist', cmdAddr: '@B_0#HDX0.0', statusAddr: '@B_0#HDX1.0', maxSlots: 24, iconName: 'CloudRain' },
  { key: 'phun_suong_2', name: 'Phun sương 2', category: 'mist', cmdAddr: '@B_0#HDX0.1', statusAddr: '@B_0#HDX1.1', maxSlots: 24, iconName: 'CloudRain' },
  { key: 'phun_suong_3', name: 'Phun sương 3', category: 'mist', cmdAddr: '@B_0#HDX0.2', statusAddr: '@B_0#HDX1.2', maxSlots: 24, iconName: 'CloudRain' },
  { key: 'phun_suong_4', name: 'Phun sương 4', category: 'mist', cmdAddr: '@B_0#HDX0.3', statusAddr: '@B_0#HDX1.3', maxSlots: 24, iconName: 'CloudRain' },
  { key: 'den_1',        name: 'Đèn 1',        category: 'light', cmdAddr: '@B_0#HDX0.4', statusAddr: '@B_0#HDX1.4', maxSlots: 10, iconName: 'Lightbulb' },
  { key: 'den_2',        name: 'Đèn 2',        category: 'light', cmdAddr: '@B_0#HDX0.5', statusAddr: '@B_0#HDX1.5', maxSlots: 10, iconName: 'Lightbulb' },
  { key: 'suoi_1',       name: 'Sưởi 1',       category: 'heater', cmdAddr: '@B_0#HDX0.6', statusAddr: '@B_0#HDX1.6', maxSlots: 10, iconName: 'Flame' },
  { key: 'loa_1',        name: 'Loa 1',        category: 'speaker', cmdAddr: '@B_0#HDX0.7', statusAddr: '@B_0#HDX1.7', maxSlots: 10, iconName: 'Volume2' },
  { key: 'loa_2',        name: 'Loa 2',        category: 'speaker', cmdAddr: '@B_0#HDX0.8', statusAddr: '@B_0#HDX1.8', maxSlots: 10, iconName: 'Volume2' },
  { key: 'loa_3',        name: 'Loa 3',        category: 'speaker', cmdAddr: '@B_0#HDX0.9', statusAddr: '@B_0#HDX1.9', maxSlots: 10, iconName: 'Volume2' },
];

export interface ScheduleSlot {
  start: string; // "07:10"
  stop: string;  // "07:20"
  start_h?: number;
  start_m?: number;
  end_h?: number;
  end_m?: number;
  day?: number;
  month?: number;
  year?: number;
  enable?: boolean;
}

export interface TelemetryState {
  temperature: number;
  humidity: number;
  vboxHeartbeat: number;
  plcHeartbeat: number;
  heartbeatAlive: boolean;
  plcCommOk: boolean;
  plcAlarmMsg: string;
  tempThreshold: number;
  humThreshold: number;
  psOnSec: number;
  psOffSec: number;
  psOnTime: number;
  psOffTime: number;
  psCycleState: number; // 0: IDLE, 1: SPRAYING, 2: RESTING
  autoTempMode: boolean;
  autoHumidityMode: boolean;
  vboxTime: string;
  vboxDate: string;
  devices: Record<DeviceKey, boolean>;
  mqttConnected: boolean;
  lastReceived: string;
  lastPublished: string;
  lastReceivedAt: number;
}

export interface AlarmItem {
  id: string;
  time: string;
  message: string;
  type: string;
  status: 'CONNECTED' | 'DISCONNECTED';
  plc_heartbeat?: number;
}
