import { createClient } from '@supabase/supabase-js';

const supabaseUrl = process.env.NEXT_PUBLIC_SUPABASE_URL || '';
const supabaseAnonKey = process.env.NEXT_PUBLIC_SUPABASE_ANON_KEY || '';

export const isSupabaseConfigured = Boolean(
  supabaseUrl && 
  supabaseAnonKey && 
  supabaseUrl.startsWith('https://') &&
  !supabaseUrl.includes('your-project')
);

export const supabase = isSupabaseConfigured
  ? createClient(supabaseUrl, supabaseAnonKey)
  : null;

// Hàm lưu log telemetry vào Supabase
export async function logTelemetryToSupabase(data: {
  temperature: number;
  humidity: number;
  vbox_heartbeat: number;
  plc_heartbeat: number;
  plc_comm_ok: boolean;
  ps_cycle_state: number;
  device_states: Record<string, number>;
}) {
  if (!supabase) return;
  try {
    await supabase.from('telemetry_logs').insert([
      {
        temperature: data.temperature,
        humidity: data.humidity,
        vbox_heartbeat: data.vbox_heartbeat,
        plc_heartbeat: data.plc_heartbeat,
        plc_comm_ok: data.plc_comm_ok,
        ps_cycle_state: data.ps_cycle_state,
        device_states: data.device_states,
        recorded_at: new Date().toISOString(),
      },
    ]);
  } catch (err) {
    console.error('Supabase telemetry logging error:', err);
  }
}

// Hàm lưu cảnh báo RS485 vào Supabase
export async function logAlarmToSupabase(alarm: {
  alarm_type: string;
  severity: string;
  source: string;
  message: string;
  plc_heartbeat?: number;
  plc_comm?: number;
}) {
  if (!supabase) return;
  try {
    await supabase.from('alarm_events').insert([
      {
        alarm_type: alarm.alarm_type,
        severity: alarm.severity,
        source: alarm.source,
        message: alarm.message,
        plc_heartbeat: alarm.plc_heartbeat,
        plc_comm: alarm.plc_comm,
        created_at: new Date().toISOString(),
      },
    ]);
  } catch (err) {
    console.error('Supabase alarm logging error:', err);
  }
}
