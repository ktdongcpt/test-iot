-- ==============================================================================
-- DỰ ÁN IOT V-BOX & PLC SMART CONTROLLER (SUPABASE DATABASE SCHEMA)
-- ==============================================================================

-- Bật UUID extension
create extension if not exists "uuid-ossp";

-- 1. BẢNG CẤU HÌNH THÔNG SỐ HỆ THỐNG & NGƯỠNG CẢM BIẾN (system_config)
create table if not exists public.system_config (
    id text primary key default 'default_config',
    temp_threshold numeric(4, 1) default 25.0 not null,  -- Ngưỡng nhiệt độ (°C)
    hum_threshold numeric(4, 1) default 70.0 not null,   -- Ngưỡng độ ẩm (%)
    ps_on_sec integer default 120 not null,              -- Thời gian phun mỗi lần (giây)
    ps_off_sec integer default 900 not null,             -- Thời gian nghỉ giữa các lần phun (giây)
    ps_on_time numeric(4, 2) default 2.0,                -- Tương thích đơn vị phút
    ps_off_time numeric(4, 2) default 15.0,              -- Tương thích đơn vị phút
    auto_temp boolean default false not null,            -- Chế độ tự động Sưởi 1 theo nhiệt độ
    auto_humidity boolean default false not null,        -- Chế độ tự động 4 bơm theo độ ẩm
    updated_at timestamp with time zone default now()
);

-- Khởi tạo bản ghi cấu hình mặc định nếu chưa có
insert into public.system_config (id, temp_threshold, hum_threshold, ps_on_sec, ps_off_sec, auto_temp, auto_humidity)
values ('default_config', 25.0, 70.0, 120, 900, false, false)
on conflict (id) do nothing;

-- 2. BẢNG LƯU TRỮ LỊCH HẸN GIỜ CỦA 10 THIẾT BỊ (device_schedules)
-- Hỗ trợ tối đa 24 khung giờ cho Phun sương 1..4 và 10 khung cho các thiết bị khác
create table if not exists public.device_schedules (
    id uuid primary key default uuid_generate_v4(),
    device_key varchar(50) not null,                     -- phun_suong_1..4, den_1..2, suoi_1, loa_1..3
    slot_index integer not null,                         -- Thứ tự khung giờ (0..23)
    enabled boolean default true not null,
    start_time varchar(5) not null,                      -- "07:10" (HH:MM)
    stop_time varchar(5) not null,                       -- "07:20" (HH:MM)
    start_h integer not null,
    start_m integer not null,
    end_h integer not null,
    end_m integer not null,
    day integer default 0,                               -- 0 = hàng ngày, hoặc 1..31
    month integer default 0,                             -- 0 = hàng tháng, hoặc 1..12
    created_at timestamp with time zone default now(),
    updated_at timestamp with time zone default now(),
    constraint unique_device_slot unique (device_key, slot_index)
);

-- Index tra cứu nhanh lịch theo thiết bị
create index if not exists idx_device_schedules_key on public.device_schedules (device_key);

-- 3. BẢNG NHẬT KÝ THÔNG SỐ CẢM BIẾN (telemetry_logs)
-- Lưu lịch sử nhiệt độ, độ ẩm để vẽ biểu đồ 24h, 7 ngày
create table if not exists public.telemetry_logs (
    id bigserial primary key,
    temperature numeric(4, 1) not null,
    humidity numeric(4, 1) not null,
    vbox_heartbeat integer,                              -- @W_0#HDW12: 0..60
    plc_heartbeat integer,                               -- @W_0#HDW13: 0..60
    plc_comm_ok boolean default true not null,           -- 1 = OK, 0 = Mất RS485
    ps_cycle_state integer default 0,                    -- 0: IDLE, 1: SPRAYING, 2: RESTING
    device_states jsonb,                                 -- Snapshot trạng thái 10 thiết bị
    recorded_at timestamp with time zone default now() not null
);

-- Index tối ưu truy vấn khoảng thời gian (vẽ biểu đồ)
create index if not exists idx_telemetry_recorded_at on public.telemetry_logs (recorded_at desc);

-- 4. BẢNG NHẬT KÝ CẢNH BÁO SỰ CỐ (alarm_events)
-- Lưu sự kiện mất kết nối RS485 hoặc nhiệt/độ ẩm vượt ngưỡng
create table if not exists public.alarm_events (
    id uuid primary key default uuid_generate_v4(),
    alarm_type varchar(50) not null,                     -- COMM_ALARM, HIGH_TEMP, LOW_HUMIDITY,...
    severity varchar(20) default 'critical' not null,    -- info, warning, critical
    source varchar(50) default 'V-BOX' not null,
    message text not null,
    plc_heartbeat integer,
    plc_comm integer,
    resolved boolean default false,
    resolved_at timestamp with time zone,
    created_at timestamp with time zone default now() not null
);

create index if not exists idx_alarm_events_created on public.alarm_events (created_at desc);

-- 5. BẬT ROW LEVEL SECURITY (RLS) & CẤP QUYỀN TRUY CẬP CƠ BẢN
alter table public.system_config enable row level security;
alter table public.device_schedules enable row level security;
alter table public.telemetry_logs enable row level security;
alter table public.alarm_events enable row level security;

-- Policy cho phép đọc công khai (để Web & Mobile có thể hiển thị Dashboard)
create policy "Allow public read access to system_config" on public.system_config for select using (true);
create policy "Allow public update to system_config" on public.system_config for all using (true);

create policy "Allow public read to device_schedules" on public.device_schedules for select using (true);
create policy "Allow public all to device_schedules" on public.device_schedules for all using (true);

create policy "Allow public read to telemetry_logs" on public.telemetry_logs for select using (true);
create policy "Allow public insert to telemetry_logs" on public.telemetry_logs for insert with check (true);

create policy "Allow public read to alarm_events" on public.alarm_events for select using (true);
create policy "Allow public insert to alarm_events" on public.alarm_events for insert with check (true);

-- 6. TỰ ĐỘNG DỌN DẸP DỮ LIỆU CŨ TRÁNH ĐẦY DATABASE (Retention Policy Function)
create or replace function cleanup_old_telemetry(days_to_keep integer default 30)
returns void as $$
begin
    delete from public.telemetry_logs
    where recorded_at < now() - (days_to_keep || ' days')::interval;
end;
$$ language plpgsql;
