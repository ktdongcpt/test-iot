/*
 * ==============================================================================
 * DỰ ÁN: VBOX-ESP32 WEB SERVER & MQTT GATEWAY (vboxesp32.ino)
 * ==============================================================================
 * Mục đích kết nối đồng bộ hoàn toàn với hệ thống V-Box (code/vbox.txt):
 *  1. MQTT Client kết nối tới broker.emqx.io:1883:
 *     - SUBSCRIBE topic "3FAMIOT/HMI_PUB": 
 *         + Nhiệt độ ("Temperature" / "Tempeturate" từ @W_0#HDW10)
 *         + Độ ẩm ("Humidity" từ @W_0#HDW11)
 *         + Heartbeat V-Box -> PLC (@W_0#HDW12: đếm 0 -> 60)
 *         + Heartbeat PLC -> V-Box (@W_0#HDW13: đếm 0 -> 60)
 *         + Trạng thái truyền thông RS485 ("plc_comm", "plc_status", "alarm")
 *         + Trạng thái thực tế 10 thiết bị (Feedback đọc từ @B_0#HDX1.0 -> @B_0#HDX1.9)
 *     - SUBSCRIBE topic "3FAMIOT/TIME": Nhận thời gian thực & ngày tháng từ RTC V-Box.
 *     - SUBSCRIBE topic "3FAMIOT/ALARM": Nhận cảnh báo tức thời khi mất truyền thông RS485 với PLC.
 *     - PUBLISH topic "3FAMIOT/HMI_SUB": Gửi lệnh điều khiển (@B_0#HDX0.0 -> @B_0#HDX0.9)
 *       và cấu hình 10 khung giờ cho từng thiết bị.
 *  2. Quản lý 10 thiết bị theo mapping.txt:
 *     - Phun sương 1: Lệnh @B_0#HDX0.0 | Phản hồi @B_0#HDX1.0
 *     - Phun sương 2: Lệnh @B_0#HDX0.1 | Phản hồi @B_0#HDX1.1
 *     - Phun sương 3: Lệnh @B_0#HDX0.2 | Phản hồi @B_0#HDX1.2
 *     - Phun sương 4: Lệnh @B_0#HDX0.3 | Phản hồi @B_0#HDX1.3
 *     - Đèn 1:        Lệnh @B_0#HDX0.4 | Phản hồi @B_0#HDX1.4
 *     - Đèn 2:        Lệnh @B_0#HDX0.5 | Phản hồi @B_0#HDX1.5
 *     - Sưởi 1:       Lệnh @B_0#HDX0.6 | Phản hồi @B_0#HDX1.6
 *     - Loa 1:        Lệnh @B_0#HDX0.7 | Phản hồi @B_0#HDX1.7
 *     - Loa 2:        Lệnh @B_0#HDX0.8 | Phản hồi @B_0#HDX1.8
 *     - Loa 3:        Lệnh @B_0#HDX0.9 | Phản hồi @B_0#HDX1.9
 *  3. Giao diện Web hiện đại:
 *     - Banner cảnh báo khẩn cấp khi MẤT TRUYỀN THÔNG RS485 (Heartbeat PLC đứng yên > 15s).
 *     - Hiển thị song song 2 Heartbeat: V-Box (HDW12) và PLC (HDW13).
 *     - Hiển thị chính xác trạng thái phản hồi thực tế (Feedback) từng thiết bị.
 *     - Cài đặt 10 khung giờ hẹn trong ngày cho mỗi thiết bị, lưu vào localStorage.
 * ==============================================================================
 */

#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>

// ==============================================================================
// 1. CẤU HÌNH WI-FI
// ==============================================================================
const char* ssid_sta     = "Dong";                 // Tên Wi-Fi
const char* password_sta = "123456987b";           // Mật khẩu Wi-Fi

// Wi-Fi dự phòng (Access Point do ESP32 phát khi không có mạng)
const char* ssid_ap      = "VBOX-ESP32-AP";
const char* password_ap  = "12345678";

// ==============================================================================
// 2. CẤU HÌNH MQTT BROKER & TOPIC (KHỚP HOÀN TOÀN VỚI vbox.txt)
// ==============================================================================
const char* mqtt_server  = "broker.emqx.io";
const int   mqtt_port    = 1883;
const char* mqtt_user    = "";
const char* mqtt_pass    = "";

// Các Topic MQTT từ vbox.txt
const char* TOPIC_SUB_DATA  = "3FAMIOT/HMI_PUB";    // ESP32 nhận nhiệt độ, độ ẩm, 2 heartbeat & phản hồi từ V-Box
const char* TOPIC_SUB_TIME  = "3FAMIOT/TIME";       // ESP32 nhận thời gian thực từ V-Box
const char* TOPIC_SUB_ALARM = "3FAMIOT/ALARM";      // ESP32 nhận cảnh báo tức thời khi mất truyền thông RS485
const char* TOPIC_PUB_CMD   = "3FAMIOT/HMI_SUB";    // ESP32 gửi lệnh điều khiển xuống V-Box

// ==============================================================================
// 3. CẤU HÌNH CẢM BIẾN TÙY CHỌN (NẾU ESP32 GẮN THÊM CẢM BIẾN DHT TRỰC TIẾP)
// ==============================================================================
// #define USE_DHT_SENSOR
#ifdef USE_DHT_SENSOR
  #include <DHT.h>
  #define DHTPIN 4
  #define DHTTYPE DHT11
  DHT dht(DHTPIN, DHTTYPE);
#endif

// ==============================================================================
// 4. BIẾN TOÀN CỤC & DỮ LIỆU ĐỒNG BỘ
// ==============================================================================
WiFiClient espClient;
PubSubClient mqttClient(espClient);
WebServer server(80);

// Dữ liệu cảm biến & truyền thông nhận từ V-Box
float currentTemperature = 0.0;
float currentHumidity    = 0.0;
int vboxHeartbeat        = 0;      // Heartbeat V-Box -> PLC (@W_0#HDW12: 0 -> 60)
int plcHeartbeat         = 0;      // Heartbeat PLC -> V-Box (@W_0#HDW13: 0 -> 60)
unsigned long lastHeartbeatTime = 0;
String vboxTime          = "--:--:--";
String vboxDate          = "--/--/----";

// Trạng thái truyền thông RS485 giữa V-Box và PLC (Giám sát đứng yên > 15s)
bool plcCommOk           = true;   // true: Kết nối tốt, false: Mất truyền thông RS485
String plcAlarmMsg       = "";     // Nội dung cảnh báo lỗi truyền thông

// Cài đặt ngưỡng cảm biến & Chế độ tự động
float tempThreshold      = 25.0;   // Ngưỡng nhiệt độ (°C): Nhiệt độ < ngưỡng => Bật sưởi 1
float humThreshold       = 70.0;   // Ngưỡng độ ẩm (%): Độ ẩm < ngưỡng => Bật 4 phun sương
float psAutoOnTime       = 2.0;    // Thời gian phun mỗi lần (phút)
float psAutoOffTime      = 15.0;   // Khoảng thời gian nghỉ giữa các lần phun (phút)
int   psAutoOnSec        = 120;    // Thời gian phun mỗi lần (giây)
int   psAutoOffSec       = 900;    // Khoảng thời gian nghỉ giữa các lần phun (giây)
int   psCycleState       = 0;      // 0: Chờ/Đủ ẩm, 1: Đang phun, 2: Đang nghỉ lan tỏa (Khóa bơm)
bool autoTempMode        = false;  // true: Tự động theo cảm biến nhiệt độ, false: Lịch hẹn RTC
bool autoHumidityMode    = false;  // true: Tự động theo cảm biến độ ẩm, false: Lịch hẹn RTC
bool autoPumpMode        = false;  // true: Tự động theo cảm biến mực nước, false: Thủ công / Lịch hẹn RTC

String lastMqttReceived  = "Đang chờ dữ liệu từ V-Box...";
String lastMqttPublished = "Chưa gửi lệnh";
String lastVboxSchedulesResp = "{}"; // Lưu phản hồi lịch thực tế từ V-Box
unsigned long lastVboxSchedulesTime = 0;
unsigned long lastMqttMsgTime = 0;
unsigned long lastReconnectAttempt = 0;

// Trạng thái phản hồi thực tế của 12 thiết bị (Đọc từ @B_0#HDX1.0 -> @B_0#HDX1.11)
bool stateMisting1   = false; // Phản hồi @B_0#HDX1.0
bool stateMisting2   = false; // Phản hồi @B_0#HDX1.1
bool stateMisting3   = false; // Phản hồi @B_0#HDX1.2
bool stateMisting4   = false; // Phản hồi @B_0#HDX1.3
bool stateLight1     = false; // Phản hồi @B_0#HDX1.4
bool stateLight2     = false; // Phản hồi @B_0#HDX1.5
bool stateHeater1    = false; // Phản hồi @B_0#HDX1.6
bool stateSpeaker1   = false; // Phản hồi @B_0#HDX1.7
bool stateSpeaker2   = false; // Phản hồi @B_0#HDX1.8
bool stateSpeaker3   = false; // Phản hồi @B_0#HDX1.9
bool statePump       = false; // Phản hồi @B_0#HDX1.10
bool stateInsectTrap = false; // Phản hồi @B_0#HDX1.11

// Cảm biến mực nước (@B_0#HDX5.0: 0 = Đủ nước, 1 = Thiếu nước; @B_0#HDX5.1: 1 = Chưa tràn, 0 = Tràn)
int waterEmpty = 0;    // 1: Thiếu nước, 0: Đủ nước
int waterSafe = 1;     // 1: Chưa tràn, 0: Tràn
int waterOverflow = 0; // 1: Đang tràn, 0: Bình thường

// Cảm biến âm thanh (Độ ồn dB, hệ số 0.1)
float sound1 = 0.0;
float sound2 = 0.0;
float sound3 = 0.0;

// Cảm biến dòng điện (A, hệ số 0.01 theo mapping.txt)
float curLight1   = 0.0; // HDW20: Light 1 Current
float curLight2   = 0.0; // HDW21: Light 2 Current
float curMisting1 = 0.0; // HDW22: Misting 1 Current
float curMisting2 = 0.0; // HDW23: Misting 2 Current
float curMisting3 = 0.0; // HDW24: Misting 3 Current
float curSpeaker1 = 0.0; // HDW25: Speaker 1 Current
float curSpeaker2 = 0.0; // HDW26: Speaker 2 Current
float curPump     = 0.0; // HDW27: Pump Current

// ==============================================================================
// 5. GIAO DIỆN WEB (HTML, CSS, JS HIỆN ĐẠI CÓ BANNER CẢNH BÁO MẤT RS485 & 2 HEARTBEAT)
// ==============================================================================
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>V-BOX ESP32 • Giám Sát Cảm Biến & Điều Khiển 12 Thiết Bị</title>
  <style>
    :root {
      --bg: #f8fafc;
      --card-bg: #ffffff;
      --border: #e2e8f0;
      --border-focus: #0284c7;
      --primary: #0284c7;
      --primary-hover: #0369a1;
      --temp: #ea580c;
      --hum: #0284c7;
      --success: #10b981;
      --danger: #ef4444;
      --text: #0f172a;
      --text-secondary: #334155;
      --muted: #64748b;
      --shadow-sm: 0 1px 2px 0 rgba(0, 0, 0, 0.05);
      --shadow: 0 1px 3px 0 rgba(0, 0, 0, 0.06), 0 1px 2px -1px rgba(0, 0, 0, 0.04);
      --shadow-md: 0 4px 6px -1px rgba(0, 0, 0, 0.07), 0 2px 4px -2px rgba(0, 0, 0, 0.05);
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif; }
    body {
      background: var(--bg);
      color: var(--text);
      min-height: 100vh;
      display: flex;
      flex-direction: column;
      align-items: center;
      padding: 24px 16px;
      -webkit-font-smoothing: antialiased;
    }
    .header { text-align: center; margin-bottom: 20px; width: 100%; max-width: 960px; }
    .header h1 { font-size: 1.65rem; color: var(--text); font-weight: 700; letter-spacing: -0.02em; }
    .header p { font-size: 0.88rem; color: var(--muted); margin-top: 4px; }
    .badges { margin-top: 12px; display: flex; gap: 8px; justify-content: center; flex-wrap: wrap; }
    .badge {
      display: inline-flex; align-items: center; gap: 5px; padding: 4px 12px; border-radius: 9999px;
      font-size: 0.75rem; font-weight: 500;
    }
    .badge-ok { background: #ecfdf5; color: #047857; border: 1px solid #a7f3d0; }
    .badge-err { background: #fef2f2; color: #b91c1c; border: 1px solid #fecaca; }
    .badge-time { background: #f0fdf4; color: #166534; border: 1px solid #bbf7d0; }
    .badge-hb { background: #faf5ff; color: #7e22ce; border: 1px solid #e9d5ff; }
    .badge-plchb { background: #eff6ff; color: #1d4ed8; border: 1px solid #bfdbfe; }
    .badge-plc-ok { background: #ecfdf5; color: #047857; border: 1px solid #a7f3d0; }

    /* Banner cảnh báo khẩn cấp khi mất RS485 */
    .alarm-banner {
      width: 100%; max-width: 960px; margin-bottom: 18px; padding: 14px 18px;
      background: #fef2f2; border: 1px solid #fca5a5; border-left: 5px solid #ef4444; border-radius: 10px;
      display: flex; align-items: center; gap: 14px; color: #991b1b;
      box-shadow: var(--shadow); animation: pulseAlert 2s infinite ease-in-out;
    }
    @keyframes pulseAlert {
      0%, 100% { box-shadow: 0 0 0 0 rgba(239, 68, 68, 0.2); }
      50% { box-shadow: 0 0 0 6px rgba(239, 68, 68, 0.12); }
    }
    .alarm-icon { font-size: 1.8rem; flex-shrink: 0; }
    .alarm-content strong { font-size: 0.98rem; color: #991b1b; display: block; font-weight: 700; }
    .alarm-content p { font-size: 0.84rem; color: #7f1d1d; margin-top: 2px; line-height: 1.4; }

    .grid {
      display: grid; grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
      gap: 16px; width: 100%; max-width: 960px;
    }
    .card {
      background: var(--card-bg);
      border: 1px solid var(--border);
      border-radius: 12px;
      padding: 20px;
      box-shadow: var(--shadow);
    }
    .card-title {
      font-size: 0.82rem; font-weight: 700; color: var(--muted);
      text-transform: uppercase; letter-spacing: 0.05em;
      margin-bottom: 14px; display: flex; align-items: center; gap: 8px;
    }
    .sensor-display {
      display: flex; align-items: baseline; justify-content: center; padding: 8px 0;
    }
    .sensor-val { font-size: 3rem; font-weight: 700; font-variant-numeric: tabular-nums; }
    .sensor-unit { font-size: 1.25rem; margin-left: 6px; color: var(--muted); font-weight: 500; }
    .val-temp { color: var(--temp); }
    .val-hum { color: var(--hum); }

    /* Lưới 10 thiết bị */
    .device-grid {
      display: grid; grid-template-columns: repeat(auto-fit, minmax(260px, 1fr));
      gap: 10px;
    }
    .device-item {
      background: #ffffff;
      border: 1px solid var(--border);
      border-radius: 10px;
      padding: 12px 14px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      transition: border-color 0.15s ease, box-shadow 0.15s ease;
    }
    .device-item:hover { border-color: #cbd5e1; box-shadow: var(--shadow-sm); }
    .device-info { display: flex; flex-direction: column; gap: 3px; }
    .device-name-row { display: flex; align-items: center; gap: 8px; }
    .device-name { font-weight: 600; font-size: 0.92rem; color: var(--text); }
    .status-pill {
      font-size: 0.68rem; font-weight: 600; padding: 2px 7px; border-radius: 9999px;
    }
    .status-on { background: #ecfdf5; color: #059669; border: 1px solid #a7f3d0; }
    .status-off { background: #f1f5f9; color: #64748b; border: 1px solid #e2e8f0; }
    .device-tag { font-size: 0.72rem; color: var(--muted); font-family: ui-monospace, SFMono-Regular, Menlo, monospace; }
    
    .switch { position: relative; width: 44px; height: 24px; flex-shrink: 0; }
    .switch input { opacity: 0; width: 0; height: 0; }
    .slider {
      position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0;
      background-color: #cbd5e1; transition: .25s ease; border-radius: 34px;
    }
    .slider:before {
      position: absolute; content: ""; height: 18px; width: 18px; left: 3px; bottom: 3px;
      background-color: white; transition: .25s ease; border-radius: 50%;
      box-shadow: 0 1px 3px rgba(0, 0, 0, 0.15);
    }
    input:checked + .slider { background-color: var(--success); }
    input:checked + .slider:before { transform: translateX(20px); }

    /* Hiệu ứng khi công tắc bị khóa do đang chạy Tự Động theo cảm biến */
    .switch.disabled-switch {
      cursor: not-allowed;
    }
    .switch.disabled-switch .slider {
      cursor: not-allowed;
      opacity: 0.65;
    }
    .switch input:disabled + .slider {
      cursor: not-allowed;
      opacity: 0.65;
    }
    .device-item.item-auto-locked {
      background: #f8fafc;
      border-color: #cbd5e1;
    }
    .lock-badge {
      font-size: 0.68rem;
      font-weight: 600;
      padding: 2px 7px;
      border-radius: 9999px;
      background: #e0f2fe;
      color: #0284c7;
      border: 1px solid #bae6fd;
      display: inline-flex;
      align-items: center;
      gap: 3px;
    }

    /* Giao diện 10 khung giờ */
    .slots-container {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(290px, 1fr));
      gap: 10px;
      margin-top: 14px;
      max-height: 480px;
      overflow-y: auto;
      padding-right: 4px;
    }
    .slot-card {
      background: #ffffff;
      border: 1px solid var(--border);
      border-radius: 8px;
      padding: 10px 12px;
      transition: all 0.15s ease;
    }
    .slot-card.active {
      border-color: #93c5fd;
      background: #f0f9ff;
    }
    .slot-card.inactive { opacity: 0.65; background: #fafafa; }
    .slot-header {
      display: flex; justify-content: space-between; align-items: center; margin-bottom: 6px;
    }
    .slot-num-badge {
      font-size: 0.75rem; font-weight: 600; padding: 2px 8px; border-radius: 6px;
      background: #f1f5f9; color: #475569; border: 1px solid #e2e8f0;
    }
    .slot-card.active .slot-num-badge {
      background: #e0f2fe; color: #0369a1; border-color: #bae6fd;
    }
    .slot-toggle {
      display: flex; align-items: center; gap: 6px; font-size: 0.8rem;
      color: #334155; cursor: pointer; font-weight: 500;
    }
    .slot-toggle input {
      cursor: pointer; accent-color: var(--primary); width: 15px; height: 15px;
    }
    .slot-times {
      display: flex; align-items: center; gap: 8px;
    }
    .slot-time-col {
      flex: 1; display: flex; flex-direction: column; gap: 3px;
    }
    .slot-time-col label {
      font-size: 0.72rem; color: var(--muted); font-weight: 500;
    }
    .slot-time-col input[type="time"] {
      width: 100%; padding: 6px 8px; font-size: 0.88rem;
      background: #ffffff; border: 1px solid #cbd5e1; border-radius: 6px;
      color: var(--text); outline: none; transition: border-color 0.15s;
    }
    .slot-time-col input[type="time"]:focus { border-color: var(--border-focus); box-shadow: 0 0 0 2px rgba(2, 132, 199, 0.15); }
    .slot-sep { color: var(--muted); font-weight: 600; margin-top: 14px; font-size: 0.85rem; }

    .form-grid {
      display: grid; grid-template-columns: 1fr 1fr; gap: 12px; margin-top: 8px;
    }
    @media (max-width: 600px) {
      .form-grid { grid-template-columns: 1fr; }
    }
    .form-group { display: flex; flex-direction: column; gap: 6px; }
    .form-group label { font-size: 0.82rem; color: var(--muted); font-weight: 600; }
    select, input[type="date"] {
      width: 100%; padding: 8px 12px; background: #ffffff;
      border: 1px solid #cbd5e1; border-radius: 8px; color: var(--text);
      font-size: 0.88rem; outline: none; transition: border-color 0.15s;
    }
    select:focus, input[type="date"]:focus { border-color: var(--border-focus); box-shadow: 0 0 0 2px rgba(2, 132, 199, 0.15); }

    .btn-group { display: flex; gap: 10px; margin-top: 14px; flex-wrap: wrap; }
    .btn-sched {
      flex: 1; min-width: 200px; padding: 10px 14px; font-weight: 600; border-radius: 8px;
      border: none; cursor: pointer; transition: all 0.15s ease; display: flex;
      align-items: center; justify-content: center; gap: 6px; font-size: 0.85rem;
    }
    .btn-sched-apply { background: var(--primary); color: white; }
    .btn-sched-apply:hover { background: var(--primary-hover); }
    .btn-sched-cancel {
      background: #fef2f2; color: #dc2626;
      border: 1px solid #fecaca;
    }
    .btn-sched-cancel:hover { background: #fee2e2; }

    .sched-info-box {
      margin-top: 12px; padding: 10px 14px; background: #f0f9ff;
      border-radius: 6px; font-size: 0.84rem; color: #0369a1; line-height: 1.5;
      border: 1px solid #e0f2fe; border-left: 3px solid var(--primary);
    }

    .input-group { display: flex; gap: 8px; margin-top: 10px; }
    input[type="text"] {
      flex: 1; padding: 8px 12px; background: #ffffff;
      border: 1px solid #cbd5e1; border-radius: 8px; color: var(--text);
      font-size: 0.88rem; outline: none; transition: border-color 0.15s;
    }
    input[type="text"]:focus { border-color: var(--border-focus); box-shadow: 0 0 0 2px rgba(2, 132, 199, 0.15); }
    button.btn-common {
      padding: 8px 16px; background: #0f172a; color: #ffffff;
      border: none; border-radius: 8px; font-weight: 600; cursor: pointer;
      font-size: 0.85rem; transition: background 0.15s;
    }
    button.btn-common:hover { background: #334155; }
    .log-box {
      background: #f8fafc; border: 1px solid var(--border); border-radius: 8px; padding: 10px 14px;
      margin-top: 10px; font-family: ui-monospace, SFMono-Regular, Menlo, monospace; font-size: 0.78rem;
      color: #334155; word-break: break-all; line-height: 1.6;
    }
    .footer { margin-top: 25px; font-size: 0.78rem; color: var(--muted); text-align: center; }
  </style>
</head>
<body>

  <!-- Banner cảnh báo khẩn cấp khi mất RS485 (Heartbeat PLC đứng yên > 15s) -->
  <div id="alarm-banner" class="alarm-banner" style="display: none;">
    <span class="alarm-icon">⚠️</span>
    <div class="alarm-content">
      <strong id="alarm-title">CẢNH BÁO: MẤT TRUYỀN THÔNG RS485 VỚI PLC!</strong>
      <p id="alarm-desc">Giá trị Heartbeat từ PLC (@W_0#HDW13) đã đứng yên quá 15 giây. Vui lòng kiểm tra cáp truyền thông RS485, PLC hoặc cổng COM1.</p>
    </div>
  </div>

  <div class="header">
    <h1>V-BOX ESP32 GATEWAY</h1>
    <p>Thu Thập Cảm Biến • Giám Sát Heartbeat Kép • Điều Khiển & Phản Hồi 12 Thiết Bị</p>
    <div class="badges">
      <span class="badge badge-ok" id="badge-mqtt">MQTT: Đang kết nối...</span>
      <span class="badge badge-ok" id="badge-plc">PLC RS485: Đang kiểm tra...</span>
      <span class="badge badge-hb" id="badge-hb">V-Box HB: --/60</span>
      <span class="badge badge-plchb" id="badge-plc-hb">PLC HB: --/60</span>
      <span class="badge badge-time" id="badge-time">V-Box Time: --:--:--</span>
    </div>
  </div>

  <div class="grid">
    <!-- Thẻ Nhiệt Độ -->
    <div class="card">
      <div class="card-title">
        <span>🌡️</span> NHIỆT ĐỘ (Từ V-Box @W_0#HDW10)
      </div>
      <div class="sensor-display">
        <span class="sensor-val val-temp" id="temp-val">--</span>
        <span class="sensor-unit">°C</span>
      </div>
    </div>

    <!-- Thẻ Độ Ẩm -->
    <div class="card">
      <div class="card-title">
        <span>💧</span> ĐỘ ẨM (Từ V-Box @W_0#HDW11)
      </div>
      <div class="sensor-display">
        <span class="sensor-val val-hum" id="hum-val">--</span>
        <span class="sensor-unit">%</span>
      </div>
    </div>

    <!-- Thẻ Mực Nước Bể Chứa -->
    <div class="card">
      <div class="card-title">
        <span>🌊</span> MỰC NƯỚC (HDX5.0 • HDX5.1)
      </div>
      <div style="display: flex; flex-direction: column; gap: 8px; margin-top: 6px;">
        <div style="display: flex; justify-content: space-between; align-items: center; padding: 7px 10px; background: #f8fafc; border-radius: 8px; border: 1px solid var(--border);">
          <span style="font-size: 0.82rem; font-weight: 500; color: var(--text);">Mực nước (HDX5.0):</span>
          <span class="badge badge-ok" id="badge-water-empty" style="font-size: 0.78rem; font-weight: 700;">ĐỦ NƯỚC</span>
        </div>
        <div style="display: flex; justify-content: space-between; align-items: center; padding: 7px 10px; background: #f8fafc; border-radius: 8px; border: 1px solid var(--border);">
          <span style="font-size: 0.82rem; font-weight: 500; color: var(--text);">Tràn nước (HDX5.1):</span>
          <span class="badge badge-ok" id="badge-water-overflow" style="font-size: 0.78rem; font-weight: 700;">CHƯA TRÀN</span>
        </div>
      </div>
    </div>

    <!-- Thẻ Cảm Biến Âm Thanh (Độ Ồn dB) -->
    <div class="card">
      <div class="card-title">
        <span>📢</span> ĐỘ ỒN ÂM THANH (dB • &times;0.1)
      </div>
      <div style="display: grid; grid-template-columns: repeat(3, 1fr); gap: 6px; margin-top: 6px; text-align: center;">
        <div style="background: #f8fafc; border: 1px solid var(--border); border-radius: 8px; padding: 8px 4px;">
          <div style="font-size: 0.72rem; color: var(--muted); font-weight: 600;">Âm thanh 1</div>
          <div style="font-size: 1.12rem; font-weight: 700; color: #0284c7; margin-top: 2px;"><span id="val-sound-1">--</span> <span style="font-size: 0.7rem; font-weight: 500;">dB</span></div>
          <div style="font-size: 0.68rem; color: var(--muted);">@W_0#HDW30</div>
        </div>
        <div style="background: #f8fafc; border: 1px solid var(--border); border-radius: 8px; padding: 8px 4px;">
          <div style="font-size: 0.72rem; color: var(--muted); font-weight: 600;">Âm thanh 2</div>
          <div style="font-size: 1.12rem; font-weight: 700; color: #0284c7; margin-top: 2px;"><span id="val-sound-2">--</span> <span style="font-size: 0.7rem; font-weight: 500;">dB</span></div>
          <div style="font-size: 0.68rem; color: var(--muted);">@W_0#HDW31</div>
        </div>
        <div style="background: #f8fafc; border: 1px solid var(--border); border-radius: 8px; padding: 8px 4px;">
          <div style="font-size: 0.72rem; color: var(--muted); font-weight: 600;">Âm thanh 3</div>
          <div style="font-size: 1.12rem; font-weight: 700; color: #0284c7; margin-top: 2px;"><span id="val-sound-3">--</span> <span style="font-size: 0.7rem; font-weight: 500;">dB</span></div>
          <div style="font-size: 0.68rem; color: var(--muted);">@W_0#HDW32</div>
        </div>
      </div>
    </div>

    <!-- Thẻ Giám Sát Dòng Điện 8 Thiết Bị -->
    <div class="card" style="grid-column: 1 / -1;">
      <div class="card-title">
        <span>⚡</span> GIÁM SÁT DÒNG ĐIỆN 8 THIẾT BỊ (@W_0#HDW20 &rarr; HDW27 • ĐƠN VỊ AMPE &times;0.01)
      </div>
      <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(135px, 1fr)); gap: 10px; margin-top: 6px;">
        <div style="background: #f8fafc; border: 1px solid var(--border); border-radius: 8px; padding: 8px 10px;">
          <div style="font-size: 0.74rem; color: var(--muted); font-weight: 600;">Đèn 1 (HDW20)</div>
          <div style="font-size: 1.1rem; font-weight: 700; color: #0f172a; margin-top: 2px;"><span id="cur-light_1">0.00</span> <span style="font-size: 0.75rem; color: var(--muted);">A</span></div>
        </div>
        <div style="background: #f8fafc; border: 1px solid var(--border); border-radius: 8px; padding: 8px 10px;">
          <div style="font-size: 0.74rem; color: var(--muted); font-weight: 600;">Đèn 2 (HDW21)</div>
          <div style="font-size: 1.1rem; font-weight: 700; color: #0f172a; margin-top: 2px;"><span id="cur-light_2">0.00</span> <span style="font-size: 0.75rem; color: var(--muted);">A</span></div>
        </div>
        <div style="background: #f8fafc; border: 1px solid var(--border); border-radius: 8px; padding: 8px 10px;">
          <div style="font-size: 0.74rem; color: var(--muted); font-weight: 600;">Phun sương 1 (HDW22)</div>
          <div style="font-size: 1.1rem; font-weight: 700; color: #0f172a; margin-top: 2px;"><span id="cur-misting_1">0.00</span> <span style="font-size: 0.75rem; color: var(--muted);">A</span></div>
        </div>
        <div style="background: #f8fafc; border: 1px solid var(--border); border-radius: 8px; padding: 8px 10px;">
          <div style="font-size: 0.74rem; color: var(--muted); font-weight: 600;">Phun sương 2 (HDW23)</div>
          <div style="font-size: 1.1rem; font-weight: 700; color: #0f172a; margin-top: 2px;"><span id="cur-misting_2">0.00</span> <span style="font-size: 0.75rem; color: var(--muted);">A</span></div>
        </div>
        <div style="background: #f8fafc; border: 1px solid var(--border); border-radius: 8px; padding: 8px 10px;">
          <div style="font-size: 0.74rem; color: var(--muted); font-weight: 600;">Phun sương 3 (HDW24)</div>
          <div style="font-size: 1.1rem; font-weight: 700; color: #0f172a; margin-top: 2px;"><span id="cur-misting_3">0.00</span> <span style="font-size: 0.75rem; color: var(--muted);">A</span></div>
        </div>
        <div style="background: #f8fafc; border: 1px solid var(--border); border-radius: 8px; padding: 8px 10px;">
          <div style="font-size: 0.74rem; color: var(--muted); font-weight: 600;">Loa 1 (HDW25)</div>
          <div style="font-size: 1.1rem; font-weight: 700; color: #0f172a; margin-top: 2px;"><span id="cur-speaker_1">0.00</span> <span style="font-size: 0.75rem; color: var(--muted);">A</span></div>
        </div>
        <div style="background: #f8fafc; border: 1px solid var(--border); border-radius: 8px; padding: 8px 10px;">
          <div style="font-size: 0.74rem; color: var(--muted); font-weight: 600;">Loa 2 (HDW26)</div>
          <div style="font-size: 1.1rem; font-weight: 700; color: #0f172a; margin-top: 2px;"><span id="cur-speaker_2">0.00</span> <span style="font-size: 0.75rem; color: var(--muted);">A</span></div>
        </div>
        <div style="background: #f8fafc; border: 1px solid var(--border); border-radius: 8px; padding: 8px 10px;">
          <div style="font-size: 0.74rem; color: var(--muted); font-weight: 600;">Máy bơm (HDW27)</div>
          <div style="font-size: 1.1rem; font-weight: 700; color: #0f172a; margin-top: 2px;"><span id="cur-pump">0.00</span> <span style="font-size: 0.75rem; color: var(--muted);">A</span></div>
        </div>
      </div>
    </div>

    <!-- Thẻ Điều Khiển Tự Động Cảm Biến & Chuyển Chế Độ -->
    <div class="card" style="grid-column: 1 / -1;">
      <div class="card-title">
        <span>⚙️</span> ĐIỀU KHIỂN TỰ ĐỘNG CẢM BIẾN & CHUYỂN CHẾ ĐỘ
      </div>
      <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(280px, 1fr)); gap: 14px;">
        
        <!-- Cột 1: Tự động Phun sương theo Độ ẩm -->
        <div style="background: #ffffff; border: 1px solid var(--border); border-radius: 10px; padding: 14px; display: flex; flex-direction: column; justify-content: space-between;">
          <div>
            <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 8px;">
              <div style="display: flex; align-items: center; gap: 8px;">
                <span style="font-size: 1.25rem;">💧</span>
                <strong style="font-size: 0.95rem; color: var(--text);">Phun Sương (4 Bơm)</strong>
              </div>
              <label class="switch" title="Gạt BẬT: Tự động theo cảm biến | Gạt TẮT: Chạy theo lịch hẹn RTC">
                <input type="checkbox" id="sw-auto-humidity" onchange="toggleAutoMode('humidity', this.checked)">
                <span class="slider"></span>
              </label>
            </div>

            <div style="margin-bottom: 12px;">
              <span class="badge" id="badge-mode-humidity" style="font-size: 0.72rem;">Đang tải chế độ...</span>
            </div>
          </div>

          <div>
            <!-- Ngưỡng kích hoạt độ ẩm -->
            <div style="display: flex; align-items: center; justify-content: space-between; margin-bottom: 8px; flex-wrap: wrap; gap: 6px;">
              <label style="font-size: 0.82rem; color: var(--muted); font-weight: 500;">Bật khi Độ ẩm &lt;</label>
              <div style="display: inline-flex; align-items: center; gap: 4px;">
                <input type="number" id="input-hum-threshold" step="0.5" min="0" max="100" style="width: 72px; padding: 5px 8px; border: 1px solid #cbd5e1; border-radius: 6px; font-weight: 600; text-align: center; outline: none; font-size: 0.88rem; color: var(--text);" value="70.0">
                <span style="font-size: 0.82rem; color: var(--muted); font-weight: 600;">%</span>
              </div>
            </div>

            <!-- Thời gian phun mỗi lần -->
            <div style="display: flex; align-items: center; justify-content: space-between; margin-bottom: 8px; flex-wrap: wrap; gap: 6px;">
              <label style="font-size: 0.82rem; color: var(--muted); font-weight: 500;" title="Thời gian chạy mỗi lần phun">Thời gian phun mỗi lần:</label>
              <div style="display: inline-flex; align-items: center; gap: 4px;">
                <input type="number" id="input-ps-on-time" step="1" min="1" max="7200" style="width: 65px; padding: 5px 6px; border: 1px solid #cbd5e1; border-radius: 6px; font-weight: 600; text-align: center; outline: none; font-size: 0.88rem; color: var(--text);" value="120">
                <select id="unit-ps-on" style="width: 62px; padding: 5px 2px; border: 1px solid #cbd5e1; border-radius: 6px; font-size: 0.8rem; background: #fff; color: var(--text);">
                  <option value="s" selected>giây</option>
                  <option value="m">phút</option>
                </select>
              </div>
            </div>

            <!-- Khoảng thời gian nghỉ giữa các lần phun -->
            <div style="display: flex; align-items: center; justify-content: space-between; margin-bottom: 12px; flex-wrap: wrap; gap: 6px;">
              <label style="font-size: 0.82rem; color: var(--muted); font-weight: 500;" title="Nghỉ để độ ẩm lan tỏa đều, khóa bơm tuyệt đối tránh úng nước">Khoảng thời gian nghỉ:</label>
              <div style="display: inline-flex; align-items: center; gap: 4px;">
                <input type="number" id="input-ps-off-time" step="1" min="0" max="86400" style="width: 65px; padding: 5px 6px; border: 1px solid #cbd5e1; border-radius: 6px; font-weight: 600; text-align: center; outline: none; font-size: 0.88rem; color: var(--text);" value="15">
                <select id="unit-ps-off" style="width: 62px; padding: 5px 2px; border: 1px solid #cbd5e1; border-radius: 6px; font-size: 0.8rem; background: #fff; color: var(--text);">
                  <option value="m" selected>phút</option>
                  <option value="s">giây</option>
                </select>
              </div>
            </div>

            <button type="button" class="btn-common" style="width: 100%; padding: 7px 0; font-size: 0.82rem; margin-bottom: 8px;" onclick="saveThreshold('humidity')">Lưu Cài Đặt Phun Sương</button>

            <p style="font-size: 0.74rem; color: var(--muted); margin: 0; line-height: 1.4;">
              • <b>Chu kỳ 3 bước:</b> Phun &rarr; Nghỉ lan tỏa (khóa bơm) &rarr; Chờ đủ ẩm.<br>
              • <b>Gạt TẮT:</b> Chạy theo 10 khung giờ lịch hẹn RTC trong ngày.
            </p>
          </div>
        </div>

        <!-- Cột 2: Tự động Sưởi 1 theo Nhiệt độ -->
        <div style="background: #ffffff; border: 1px solid var(--border); border-radius: 10px; padding: 14px; display: flex; flex-direction: column; justify-content: space-between;">
          <div>
            <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 8px;">
              <div style="display: flex; align-items: center; gap: 8px;">
                <span style="font-size: 1.25rem;">🔥</span>
                <strong style="font-size: 0.95rem; color: var(--text);">Sưởi 1</strong>
              </div>
              <label class="switch" title="Gạt BẬT: Tự động theo cảm biến | Gạt TẮT: Chạy theo lịch hẹn RTC">
                <input type="checkbox" id="sw-auto-temp" onchange="toggleAutoMode('temp', this.checked)">
                <span class="slider"></span>
              </label>
            </div>

            <div style="margin-bottom: 12px;">
              <span class="badge" id="badge-mode-temp" style="font-size: 0.72rem;">Đang tải chế độ...</span>
            </div>
          </div>

          <div>
            <div style="display: flex; align-items: center; justify-content: space-between; margin-bottom: 12px; flex-wrap: wrap; gap: 6px;">
              <label style="font-size: 0.82rem; color: var(--muted); font-weight: 500;">Bật Sưởi 1 khi Nhiệt độ &lt;</label>
              <div style="display: inline-flex; align-items: center; gap: 4px;">
                <input type="number" id="input-temp-threshold" step="0.5" min="0" max="60" style="width: 72px; padding: 5px 8px; border: 1px solid #cbd5e1; border-radius: 6px; font-weight: 600; text-align: center; outline: none; font-size: 0.88rem; color: var(--text);" value="25.0">
                <span style="font-size: 0.82rem; color: var(--muted); font-weight: 600;">°C</span>
              </div>
            </div>

            <button type="button" class="btn-common" style="width: 100%; padding: 7px 0; font-size: 0.82rem; margin-bottom: 8px;" onclick="saveThreshold('temp')">Lưu Cài Đặt Sưởi 1</button>

            <p style="font-size: 0.74rem; color: var(--muted); margin: 0; line-height: 1.4;">
              • <b>Gạt BẬT:</b> Cảm biến kiểm soát (Nhiệt độ &lt; ngưỡng cài &rarr; Bật sưởi 1).<br>
              • <b>Gạt TẮT:</b> Chạy theo 10 khung giờ lịch hẹn RTC trong ngày.
            </p>
          </div>
        </div>

        <!-- Cột 3: Tự động Bơm Nước theo Mực Nước Bể Chứa -->
        <div style="background: #ffffff; border: 1px solid var(--border); border-radius: 10px; padding: 14px; display: flex; flex-direction: column; justify-content: space-between;">
          <div>
            <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 8px;">
              <div style="display: flex; align-items: center; gap: 8px;">
                <span style="font-size: 1.25rem;">🌊</span>
                <strong style="font-size: 0.95rem; color: var(--text);">Máy Bơm Nước</strong>
              </div>
              <label class="switch" title="Gạt BẬT: Tự động bơm theo mực nước bể | Gạt TẮT: Điều khiển thủ công / Lịch RTC">
                <input type="checkbox" id="sw-auto-pump" onchange="toggleAutoMode('pump', this.checked)">
                <span class="slider"></span>
              </label>
            </div>

            <div style="margin-bottom: 12px;">
              <span class="badge" id="badge-mode-pump" style="font-size: 0.72rem;">Đang tải chế độ...</span>
            </div>
          </div>

          <div>
            <div style="background: #f8fafc; border: 1px solid var(--border); border-radius: 8px; padding: 10px; margin-bottom: 12px; font-size: 0.8rem; color: var(--text);">
              <div style="display: flex; justify-content: space-between; margin-bottom: 6px;">
                <span style="color: var(--muted);">Thiếu nước đáy (HDX5.0 = 1):</span>
                <strong style="color: #10b981;">Kích hoạt BẬT bơm</strong>
              </div>
              <div style="display: flex; justify-content: space-between; margin-bottom: 6px;">
                <span style="color: var(--muted);">Nước dâng giữa bể (HDX5.0 = 0):</span>
                <strong style="color: #0284c7;">Tiếp tục BƠM tới đỉnh</strong>
              </div>
              <div style="display: flex; justify-content: space-between;">
                <span style="color: var(--muted);">Tràn đỉnh bể (HDX5.1 = 0):</span>
                <strong style="color: #ef4444;">NGỪNG BƠM HOÀN TOÀN</strong>
              </div>
            </div>

            <p style="font-size: 0.74rem; color: var(--muted); margin: 0; line-height: 1.4;">
              • <b>Chu trình tự động:</b> Thiếu nước đáy &rarr; Tự bật bơm &rarr; Bơm liên tục tới khi tràn đỉnh thì ngừng &rarr; Dùng nước cạn đáy lặp lại.<br>
              • <b>Gạt TẮT:</b> Điều khiển nút nhấn thủ công hoặc theo lịch hẹn RTC.
            </p>
          </div>
        </div>

      </div>
    </div>

    <!-- Thẻ Bảng Điều Khiển & Phản Hồi 12 Thiết Bị Thực Tế -->
    <div class="card" style="grid-column: 1 / -1;">
      <div class="card-title">
        <span>⚡</span> BẢNG ĐIỀU KHIỂN & PHẢN HỒI 12 THIẾT BỊ (LỆNH HDX0.x • PHẢN HỒI HDX1.x)
      </div>
      <div class="device-grid">
        <!-- 12 thiết bị -->
        <div class="device-item" id="item-misting_1">
          <div class="device-info">
            <div class="device-name-row">
              <span class="device-name">Phun sương 1</span>
              <span class="status-pill status-off" id="pill-misting_1">TẮT</span>
              <span class="lock-badge" id="lock-misting_1" style="display: none;">🔒 Tự động</span>
            </div>
            <span class="device-tag">Lệnh: HDX0.0 • Phản hồi: HDX1.0 • <b id="tag-cur-misting_1" style="color:#0284c7;">-- A</b></span>
          </div>
          <label class="switch" id="lbl-sw-misting_1" onclick="onSwitchLabelClick(event, 'misting_1')">
            <input type="checkbox" id="sw-misting_1" onchange="sendMqttControl('misting_1', this.checked ? 1 : 0)">
            <span class="slider"></span>
          </label>
        </div>

        <div class="device-item" id="item-misting_2">
          <div class="device-info">
            <div class="device-name-row">
              <span class="device-name">Phun sương 2</span>
              <span class="status-pill status-off" id="pill-misting_2">TẮT</span>
              <span class="lock-badge" id="lock-misting_2" style="display: none;">🔒 Tự động</span>
            </div>
            <span class="device-tag">Lệnh: HDX0.1 • Phản hồi: HDX1.1 • <b id="tag-cur-misting_2" style="color:#0284c7;">-- A</b></span>
          </div>
          <label class="switch" id="lbl-sw-misting_2" onclick="onSwitchLabelClick(event, 'misting_2')">
            <input type="checkbox" id="sw-misting_2" onchange="sendMqttControl('misting_2', this.checked ? 1 : 0)">
            <span class="slider"></span>
          </label>
        </div>

        <div class="device-item" id="item-misting_3">
          <div class="device-info">
            <div class="device-name-row">
              <span class="device-name">Phun sương 3</span>
              <span class="status-pill status-off" id="pill-misting_3">TẮT</span>
              <span class="lock-badge" id="lock-misting_3" style="display: none;">🔒 Tự động</span>
            </div>
            <span class="device-tag">Lệnh: HDX0.2 • Phản hồi: HDX1.2 • <b id="tag-cur-misting_3" style="color:#0284c7;">-- A</b></span>
          </div>
          <label class="switch" id="lbl-sw-misting_3" onclick="onSwitchLabelClick(event, 'misting_3')">
            <input type="checkbox" id="sw-misting_3" onchange="sendMqttControl('misting_3', this.checked ? 1 : 0)">
            <span class="slider"></span>
          </label>
        </div>

        <div class="device-item" id="item-misting_4">
          <div class="device-info">
            <div class="device-name-row">
              <span class="device-name">Phun sương 4</span>
              <span class="status-pill status-off" id="pill-misting_4">TẮT</span>
              <span class="lock-badge" id="lock-misting_4" style="display: none;">🔒 Tự động</span>
            </div>
            <span class="device-tag">Lệnh: HDX0.3 • Phản hồi: HDX1.3</span>
          </div>
          <label class="switch" id="lbl-sw-misting_4" onclick="onSwitchLabelClick(event, 'misting_4')">
            <input type="checkbox" id="sw-misting_4" onchange="sendMqttControl('misting_4', this.checked ? 1 : 0)">
            <span class="slider"></span>
          </label>
        </div>

        <div class="device-item" id="item-light_1">
          <div class="device-info">
            <div class="device-name-row">
              <span class="device-name">Đèn 1</span>
              <span class="status-pill status-off" id="pill-light_1">TẮT</span>
            </div>
            <span class="device-tag">Lệnh: HDX0.4 • Phản hồi: HDX1.4 • <b id="tag-cur-light_1" style="color:#0284c7;">-- A</b></span>
          </div>
          <label class="switch" id="lbl-sw-light_1">
            <input type="checkbox" id="sw-light_1" onchange="sendMqttControl('light_1', this.checked ? 1 : 0)">
            <span class="slider"></span>
          </label>
        </div>

        <div class="device-item" id="item-light_2">
          <div class="device-info">
            <div class="device-name-row">
              <span class="device-name">Đèn 2</span>
              <span class="status-pill status-off" id="pill-light_2">TẮT</span>
            </div>
            <span class="device-tag">Lệnh: HDX0.5 • Phản hồi: HDX1.5 • <b id="tag-cur-light_2" style="color:#0284c7;">-- A</b></span>
          </div>
          <label class="switch" id="lbl-sw-light_2">
            <input type="checkbox" id="sw-light_2" onchange="sendMqttControl('light_2', this.checked ? 1 : 0)">
            <span class="slider"></span>
          </label>
        </div>

        <div class="device-item" id="item-heater_1">
          <div class="device-info">
            <div class="device-name-row">
              <span class="device-name">Sưởi 1</span>
              <span class="status-pill status-off" id="pill-heater_1">TẮT</span>
              <span class="lock-badge" id="lock-heater_1" style="display: none;">🔒 Tự động</span>
            </div>
            <span class="device-tag">Lệnh: HDX0.6 • Phản hồi: HDX1.6</span>
          </div>
          <label class="switch" id="lbl-sw-heater_1" onclick="onSwitchLabelClick(event, 'heater_1')">
            <input type="checkbox" id="sw-heater_1" onchange="sendMqttControl('heater_1', this.checked ? 1 : 0)">
            <span class="slider"></span>
          </label>
        </div>

        <div class="device-item" id="item-speaker_1">
          <div class="device-info">
            <div class="device-name-row">
              <span class="device-name">Loa 1</span>
              <span class="status-pill status-off" id="pill-speaker_1">TẮT</span>
            </div>
            <span class="device-tag">Lệnh: HDX0.7 • Phản hồi: HDX1.7 • <b id="tag-cur-speaker_1" style="color:#0284c7;">-- A</b></span>
          </div>
          <label class="switch" id="lbl-sw-speaker_1">
            <input type="checkbox" id="sw-speaker_1" onchange="sendMqttControl('speaker_1', this.checked ? 1 : 0)">
            <span class="slider"></span>
          </label>
        </div>

        <div class="device-item" id="item-speaker_2">
          <div class="device-info">
            <div class="device-name-row">
              <span class="device-name">Loa 2</span>
              <span class="status-pill status-off" id="pill-speaker_2">TẮT</span>
            </div>
            <span class="device-tag">Lệnh: HDX0.8 • Phản hồi: HDX1.8 • <b id="tag-cur-speaker_2" style="color:#0284c7;">-- A</b></span>
          </div>
          <label class="switch" id="lbl-sw-speaker_2">
            <input type="checkbox" id="sw-speaker_2" onchange="sendMqttControl('speaker_2', this.checked ? 1 : 0)">
            <span class="slider"></span>
          </label>
        </div>

        <div class="device-item" id="item-speaker_3">
          <div class="device-info">
            <div class="device-name-row">
              <span class="device-name">Loa 3</span>
              <span class="status-pill status-off" id="pill-speaker_3">TẮT</span>
            </div>
            <span class="device-tag">Lệnh: HDX0.9 • Phản hồi: HDX1.9</span>
          </div>
          <label class="switch" id="lbl-sw-speaker_3">
            <input type="checkbox" id="sw-speaker_3" onchange="sendMqttControl('speaker_3', this.checked ? 1 : 0)">
            <span class="slider"></span>
          </label>
        </div>

        <div class="device-item" id="item-pump">
          <div class="device-info">
            <div class="device-name-row">
              <span class="device-name">Máy bơm</span>
              <span class="status-pill status-off" id="pill-pump">TẮT</span>
              <span class="lock-badge" id="lock-pump" style="display: none;">🔒 Tự động</span>
            </div>
            <span class="device-tag">Lệnh: HDX0.10 • Phản hồi: HDX1.10 • <b id="tag-cur-pump" style="color:#0284c7;">-- A</b></span>
          </div>
          <label class="switch" id="lbl-sw-pump" onclick="onSwitchLabelClick(event, 'pump')">
            <input type="checkbox" id="sw-pump" onchange="sendMqttControl('pump', this.checked ? 1 : 0)">
            <span class="slider"></span>
          </label>
        </div>

        <div class="device-item" id="item-insect_trap">
          <div class="device-info">
            <div class="device-name-row">
              <span class="device-name">Bẫy côn trùng</span>
              <span class="status-pill status-off" id="pill-insect_trap">TẮT</span>
            </div>
            <span class="device-tag">Lệnh: HDX0.11 • Phản hồi: HDX1.11</span>
          </div>
          <label class="switch" id="lbl-sw-insect_trap">
            <input type="checkbox" id="sw-insect_trap" onchange="sendMqttControl('insect_trap', this.checked ? 1 : 0)">
            <span class="slider"></span>
          </label>
        </div>
      </div>
    </div>

    <!-- Thẻ Cài Đặt Hẹn Giờ 10 Khung Giờ (Xổ ra khi chọn thiết bị) -->
    <div class="card" style="grid-column: 1 / -1;">
      <div class="card-title">
        <span>⏰</span> CÀI ĐẶT 10 KHUNG GIỜ HẸN TRONG NGÀY (V-BOX RTC)
      </div>
      <p style="font-size: 0.85rem; color: var(--muted); margin-bottom: 12px;">
        Chọn thiết bị bên dưới để tùy chỉnh khung giờ hoạt động tự động trong ngày (Phun sương: tối đa 24 khung giờ, thiết bị khác: 10 khung giờ):
      </p>

      <div class="form-grid">
        <div class="form-group">
          <label style="font-weight: 600; color: var(--primary);">👉 Chọn thiết bị cài lịch hẹn:</label>
          <select id="sched-device" onchange="onDeviceSelectChange()">
            <option value="misting_1">Phun sương 1 (@B_0#HDX0.0 / HDX1.0) - [24 Khung Giờ]</option>
            <option value="misting_2">Phun sương 2 (@B_0#HDX0.1 / HDX1.1) - [24 Khung Giờ]</option>
            <option value="misting_3">Phun sương 3 (@B_0#HDX0.2 / HDX1.2) - [24 Khung Giờ]</option>
            <option value="misting_4">Phun sương 4 (@B_0#HDX0.3 / HDX1.3) - [24 Khung Giờ]</option>
            <option value="light_1">Đèn 1 (@B_0#HDX0.4 / HDX1.4) - [10 Khung Giờ]</option>
            <option value="light_2">Đèn 2 (@B_0#HDX0.5 / HDX1.5) - [10 Khung Giờ]</option>
            <option value="heater_1">Sưởi 1 (@B_0#HDX0.6 / HDX1.6) - [10 Khung Giờ]</option>
            <option value="speaker_1">Loa 1 (@B_0#HDX0.7 / HDX1.7) - [10 Khung Giờ]</option>
            <option value="speaker_2">Loa 2 (@B_0#HDX0.8 / HDX1.8) - [10 Khung Giờ]</option>
            <option value="speaker_3">Loa 3 (@B_0#HDX0.9 / HDX1.9) - [10 Khung Giờ]</option>
            <option value="pump">Máy bơm (@B_0#HDX0.10 / HDX1.10) - [10 Khung Giờ]</option>
            <option value="insect_trap">Bẫy côn trùng (@B_0#HDX0.11 / HDX1.11) - [10 Khung Giờ]</option>
          </select>
        </div>

        <div class="form-group">
          <label>Tần suất áp dụng:</label>
          <div style="display: flex; gap: 8px; align-items: center;">
            <input type="date" id="sched-date" style="flex: 1; opacity: 0.5;" disabled>
            <label style="font-size: 0.8rem; color: var(--text-secondary); cursor: pointer; white-space: nowrap; display: flex; align-items: center; gap: 4px;">
              <input type="checkbox" id="sched-daily" checked onchange="toggleDaily(this.checked)"> Hàng ngày
            </label>
          </div>
        </div>
      </div>

      <!-- Danh sách khung giờ được render tự động -->
      <div style="margin-top: 14px; display: flex; justify-content: space-between; align-items: center;">
        <span style="font-size: 0.88rem; font-weight: 600; color: var(--text);" id="slots-header-title">
          24 Khung Giờ Của: Phun sương 1
        </span>
        <div style="display: flex; gap: 8px;">
          <button type="button" style="padding: 4px 10px; font-size: 0.75rem; background: #f1f5f9; color: #475569; border: 1px solid #e2e8f0; border-radius: 6px; cursor: pointer;" onclick="toggleAllSlots(true)">Bật tất cả</button>
          <button type="button" style="padding: 4px 10px; font-size: 0.75rem; background: #f1f5f9; color: #475569; border: 1px solid #e2e8f0; border-radius: 6px; cursor: pointer;" onclick="toggleAllSlots(false)">Tắt tất cả</button>
        </div>
      </div>

      <div class="slots-container" id="slots-container">
        <!-- Khung Giờ sẽ được JavaScript render tại đây -->
      </div>

      <div class="btn-group">
        <button type="button" class="btn-sched btn-sched-apply" onclick="saveCurrentDeviceSchedule()">
          💾 Lưu & Kích Hoạt Lịch Cho Thiết Bị Này
        </button>
        <button type="button" class="btn-sched" style="background: #0284c7; color: #ffffff;" onclick="openVboxSchedModal()">
          🔍 Xem Lịch Đã Nạp Thực Tế Trên V-Box
        </button>
        <button type="button" class="btn-sched btn-sched-cancel" onclick="clearCurrentDeviceSchedule()">
          ⏹️ Tắt / Hủy Lịch Hẹn Của Thiết Bị Này
        </button>
      </div>

      <div class="sched-info-box">
        <b>Trạng thái:</b> <span id="sched-status-text">Đang tải lịch hẹn...</span>
      </div>
    </div>

    <!-- Modal Popup Xem Lịch Nạp Thực Tế Trên V-Box -->
    <div id="vbox-sched-modal" style="display: none; position: fixed; z-index: 9999; top: 0; left: 0; width: 100%; height: 100%; background: rgba(15, 23, 42, 0.55); backdrop-filter: blur(2px); align-items: center; justify-content: center; padding: 16px;">
      <div style="background: #ffffff; border-radius: 12px; max-width: 680px; width: 100%; max-height: 88vh; display: flex; flex-direction: column; box-shadow: 0 20px 25px -5px rgba(0,0,0,0.15), 0 10px 10px -5px rgba(0,0,0,0.05); border: 1px solid var(--border);">
        <div style="padding: 16px 20px; border-bottom: 1px solid var(--border); display: flex; justify-content: space-between; align-items: center;">
          <div>
            <h3 style="font-size: 1.05rem; font-weight: 700; color: var(--text);" id="modal-sched-title">📋 Lịch Đã Nạp Thực Tế Trên V-Box</h3>
            <p style="font-size: 0.8rem; color: var(--muted); margin-top: 3px;" id="modal-sched-subtitle">Đang tải dữ liệu từ V-Box...</p>
          </div>
          <button type="button" onclick="closeVboxSchedModal()" style="border: none; background: #f1f5f9; color: #475569; width: 32px; height: 32px; border-radius: 50%; font-size: 1.1rem; cursor: pointer; display: flex; align-items: center; justify-content: center; font-weight: bold;">✕</button>
        </div>
        <div style="padding: 16px 20px; overflow-y: auto; flex: 1; max-height: 60vh;" id="modal-sched-body">
          <!-- Danh sách các khung giờ được nạp thực tế trên V-Box -->
        </div>
        <div style="padding: 12px 20px; border-top: 1px solid var(--border); display: flex; justify-content: space-between; align-items: center; background: #fafafa; border-radius: 0 0 12px 12px; flex-wrap: wrap; gap: 8px;">
          <span style="font-size: 0.78rem; color: var(--muted);" id="modal-sched-status">Chế độ xem đối soát dữ liệu thực tế từ bộ nhớ V-Box (không nạp, không can thiệp)</span>
          <button type="button" class="btn-common" style="background: #0284c7; padding: 7px 18px;" onclick="closeVboxSchedModal()">Đóng Cửa Sổ</button>
        </div>
      </div>
    </div>

    <!-- Thẻ Truyền & Nhận Dữ Liệu Tùy Biến Qua MQTT -->
    <div class="card" style="grid-column: 1 / -1;">
      <div class="card-title">
        <span>🔄</span> TRUYỀN & NHẬN LỆNH MQTT TÙY CHỌN
      </div>
      <p style="font-size: 0.85rem; color: var(--muted);">Gửi JSON bất kỳ tới V-Box (Topic: 3FAMIOT/HMI_SUB):</p>
      <div class="input-group">
        <input type="text" id="custom-msg" placeholder='VD: {"phun_suong_2": 1, "duration": 30}'>
        <button class="btn-common" onclick="sendCustomMqtt()">Gửi MQTT</button>
      </div>
      <div class="log-box" id="mqtt-log">
        <b>Dữ liệu MQTT nhận từ V-Box:</b> Đang chờ...<br>
        <b>Lệnh MQTT gửi gần nhất:</b> Chưa có
      </div>
    </div>
  </div>

  <div class="footer">
    Broker: broker.emqx.io:1883 • Subscribe: 3FAMIOT/HMI_PUB, TIME, ALARM • Publish: 3FAMIOT/HMI_SUB
  </div>

  <script>
    const devKeys = [
      'misting_1', 'misting_2', 'misting_3', 'misting_4',
      'light_1', 'light_2', 'heater_1', 'speaker_1', 'speaker_2', 'speaker_3',
      'pump', 'insect_trap'
    ];

    const devNames = {
      misting_1:   'Phun sương 1 (HDX0.0 / HDX1.0)',
      misting_2:   'Phun sương 2 (HDX0.1 / HDX1.1)',
      misting_3:   'Phun sương 3 (HDX0.2 / HDX1.2)',
      misting_4:   'Phun sương 4 (HDX0.3 / HDX1.3)',
      light_1:     'Đèn 1 (HDX0.4 / HDX1.4)',
      light_2:     'Đèn 2 (HDX0.5 / HDX1.5)',
      heater_1:    'Sưởi 1 (HDX0.6 / HDX1.6)',
      speaker_1:   'Loa 1 (HDX0.7 / HDX1.7)',
      speaker_2:   'Loa 2 (HDX0.8 / HDX1.8)',
      speaker_3:   'Loa 3 (HDX0.9 / HDX1.9)',
      pump:        'Máy bơm (HDX0.10 / HDX1.10)',
      insect_trap: 'Bẫy côn trùng (HDX0.11 / HDX1.11)'
    };

    const PS_DEVICES = ['misting_1', 'misting_2', 'misting_3', 'misting_4'];
    let currentAutoHumidity = false;
    let currentAutoTemp = false;
    let currentAutoPump = false;

    function isMistingDevice(k) {
      return PS_DEVICES.indexOf(k) !== -1;
    }

    let deviceSchedules = {};

    function getMaxSlots(dev) {
      return (dev && dev.startsWith('misting')) ? 24 : 10;
    }

    function initDefaultSchedules() {
      const stored = localStorage.getItem('vbox_device_schedules_v2');
      if (stored) {
        try {
          deviceSchedules = JSON.parse(stored);
        } catch (e) {
          deviceSchedules = {};
        }
      }

      devKeys.forEach(k => {
        const maxS = getMaxSlots(k);
        if (!deviceSchedules[k]) {
          let slots = [];
          for (let i = 0; i < maxS; i++) {
            slots.push({ en: false, start: '', stop: '' });
          }
          deviceSchedules[k] = { isDaily: true, date: '', slots: slots };
        } else {
          if (!deviceSchedules[k].slots) deviceSchedules[k].slots = [];
          while (deviceSchedules[k].slots.length < maxS) {
            deviceSchedules[k].slots.push({ en: false, start: '', stop: '' });
          }
        }
      });

      if (!stored && deviceSchedules['misting_1']) {
        const targetDev = deviceSchedules['misting_1'];
        targetDev.slots[0] = { en: true, start: '07:10', stop: '07:20' };
        targetDev.slots[1] = { en: true, start: '08:00', stop: '08:30' };
      }
    }

    function renderSlotsUI(dev) {
      const container = document.getElementById('slots-container');
      container.innerHTML = '';
      const maxSlots = getMaxSlots(dev);

      for (let i = 0; i < maxSlots; i++) {
        const card = document.createElement('div');
        card.className = 'slot-card inactive';
        card.id = `slot-card-${i}`;
        card.innerHTML = `
          <div class="slot-header">
            <span class="slot-num-badge">Khung Giờ #${i + 1}</span>
            <label class="slot-toggle">
              <input type="checkbox" id="slot-en-${i}" onchange="onSlotToggleChange(${i})">
              <span>Kích hoạt</span>
            </label>
          </div>
          <div class="slot-times">
            <div class="slot-time-col">
              <label>Bật lúc:</label>
              <input type="time" id="slot-start-${i}" onchange="onSlotTimeChange(${i})">
            </div>
            <span class="slot-sep">➔</span>
            <div class="slot-time-col">
              <label>Tắt lúc:</label>
              <input type="time" id="slot-stop-${i}" onchange="onSlotTimeChange(${i})">
            </div>
          </div>
        `;
        container.appendChild(card);
      }
    }

    function onDeviceSelectChange() {
      const dev = document.getElementById('sched-device').value;
      const maxSlots = getMaxSlots(dev);
      renderSlotsUI(dev);

      document.getElementById('slots-header-title').innerText = `${maxSlots} Khung Giờ Của: ${devNames[dev]}`;

      const cfg = deviceSchedules[dev] || { isDaily: true, date: '', slots: [] };
      document.getElementById('sched-daily').checked = (cfg.isDaily !== false);
      toggleDaily(cfg.isDaily !== false);
      if (cfg.date) document.getElementById('sched-date').value = cfg.date;

      for (let i = 0; i < maxSlots; i++) {
        const s = (cfg.slots && cfg.slots[i]) ? cfg.slots[i] : { en: false, start: '', stop: '' };
        const enInput = document.getElementById(`slot-en-${i}`);
        const stInput = document.getElementById(`slot-start-${i}`);
        const spInput = document.getElementById(`slot-stop-${i}`);

        if (enInput) enInput.checked = !!s.en;
        if (stInput) stInput.value = s.start || '';
        if (spInput) spInput.value = s.stop || '';

        updateSlotCardStyle(i);
      }
      updateStatusSummary(dev);
    }

    function updateSlotCardStyle(idx) {
      const card = document.getElementById(`slot-card-${idx}`);
      const chk = document.getElementById(`slot-en-${idx}`);
      if (card && chk) {
        card.className = chk.checked ? 'slot-card active' : 'slot-card inactive';
      }
    }

    function onSlotToggleChange(idx) {
      updateSlotCardStyle(idx);
    }

    function onSlotTimeChange(idx) {
      const st = document.getElementById(`slot-start-${idx}`).value;
      const sp = document.getElementById(`slot-stop-${idx}`).value;
      if (st && sp) {
        document.getElementById(`slot-en-${idx}`).checked = true;
        updateSlotCardStyle(idx);
      }
    }

    function toggleAllSlots(enable) {
      const dev = document.getElementById('sched-device').value;
      const maxSlots = getMaxSlots(dev);
      for (let i = 0; i < maxSlots; i++) {
        const chk = document.getElementById(`slot-en-${i}`);
        if (chk) chk.checked = enable;
        updateSlotCardStyle(i);
      }
    }

    function toggleDaily(isDaily) {
      const dInput = document.getElementById('sched-date');
      dInput.disabled = isDaily;
      dInput.style.opacity = isDaily ? '0.5' : '1';
      if (!isDaily && !dInput.value) {
        const today = new Date().toISOString().split('T')[0];
        dInput.value = today;
      }
    }

    function updateStatusSummary(dev) {
      const cfg = deviceSchedules[dev];
      if (!cfg || !cfg.slots) return;
      let activeSlots = [];
      cfg.slots.forEach((s, idx) => {
        if (s.en && s.start && s.stop) {
          activeSlots.push(`[#${idx + 1}: ${s.start} → ${s.stop}]`);
        }
      });

      const txt = document.getElementById('sched-status-text');
      if (activeSlots.length === 0) {
        txt.innerText = `${devNames[dev]}: Hiện chưa kích hoạt khung giờ nào trên giao diện Web.`;
      } else {
        const freq = (cfg.isDaily !== false) ? 'Hàng ngày' : (cfg.date || 'Ngày cụ thể');
        txt.innerHTML = `<b>${devNames[dev]}</b> (${freq}) đang chọn <b>${activeSlots.length}</b> khung giờ: <br>` + activeSlots.join(', ');
      }

      if (isMistingDevice(dev) && currentAutoHumidity) {
        txt.innerHTML += `<br><span style="color:#0284c7; font-weight:600; font-size:0.8rem; display:inline-block; margin-top:4px;">ℹ️ Chú ý: Thiết bị này đang chạy Tự Động theo cảm biến độ ẩm. Các khung giờ cài đặt sẽ có hiệu lực sau khi bạn tắt chế độ Tự Động.</span>`;
      } else if (dev === 'heater_1' && currentAutoTemp) {
        txt.innerHTML += `<br><span style="color:#ea580c; font-weight:600; font-size:0.8rem; display:inline-block; margin-top:4px;">ℹ️ Chú ý: Thiết bị này đang chạy Tự Động theo cảm biến nhiệt độ. Các khung giờ cài đặt sẽ có hiệu lực sau khi bạn tắt chế độ Tự Động.</span>`;
      } else if (dev === 'pump' && currentAutoPump) {
        txt.innerHTML += `<br><span style="color:#0284c7; font-weight:600; font-size:0.8rem; display:inline-block; margin-top:4px;">ℹ️ Chú ý: Thiết bị này đang chạy Tự Động theo cảm biến mực nước. Các khung giờ cài đặt sẽ có hiệu lực sau khi bạn tắt chế độ Tự Động.</span>`;
      }
    }

    async function saveCurrentDeviceSchedule() {
      const dev = document.getElementById('sched-device').value;
      const isDaily = document.getElementById('sched-daily').checked;
      const dateVal = document.getElementById('sched-date').value;
      const maxSlots = getMaxSlots(dev);

      let slots = [];
      let activePayloadSlots = [];

      for (let i = 0; i < maxSlots; i++) {
        const enInput = document.getElementById(`slot-en-${i}`);
        const stInput = document.getElementById(`slot-start-${i}`);
        const spInput = document.getElementById(`slot-stop-${i}`);
        const en = enInput ? enInput.checked : false;
        const st = stInput ? stInput.value : '';
        const sp = spInput ? spInput.value : '';

        slots.push({ en: en, start: st, stop: sp });

        if (en && st && sp) {
          let item = { start: st, stop: sp, enable: 1 };
          if (!isDaily && dateVal) {
            const parts = dateVal.split('-');
            item.year = parseInt(parts[0]);
            item.month = parseInt(parts[1]);
            item.day = parseInt(parts[2]);
          }
          activePayloadSlots.push(item);
        }
      }

      deviceSchedules[dev] = {
        isDaily: isDaily,
        date: dateVal,
        slots: slots
      };
      localStorage.setItem('vbox_device_schedules_v2', JSON.stringify(deviceSchedules));

      let payload = {
        device: dev,
        mode: "schedule",
        schedules: activePayloadSlots
      };
      payload[dev] = "schedule";

      const jsonStr = JSON.stringify(payload);
      await postMqttPayload(jsonStr);

      updateStatusSummary(dev);
      alert(`Đã lưu và gửi thành công ${activePayloadSlots.length} khung giờ cho ${devNames[dev]}!\nBấm "Xem Lịch Đã Nạp Thực Tế Trên V-Box" để kiểm tra V-Box đã nhận.`);
    }

    async function clearCurrentDeviceSchedule() {
      const dev = document.getElementById('sched-device').value;
      if (!confirm(`Bạn có chắc muốn tắt chế độ hẹn giờ của ${devNames[dev]}?`)) return;

      toggleAllSlots(false);
      if (deviceSchedules[dev] && deviceSchedules[dev].slots) {
        deviceSchedules[dev].slots.forEach(s => s.en = false);
        localStorage.setItem('vbox_device_schedules_v2', JSON.stringify(deviceSchedules));
      }

      let payload = { device: dev, mode: 0 };
      payload[dev] = 0;
      await postMqttPayload(JSON.stringify(payload));

      updateStatusSummary(dev);
      alert(`Đã tắt lịch hẹn cho ${devNames[dev]}`);
    }

    // Modal chỉ để xem và đối soát lịch thực tế nạp trong V-Box (không nạp, không can thiệp)
    async function openVboxSchedModal() {
      const dev = document.getElementById('sched-device').value;
      const devName = devNames[dev] || dev;
      document.getElementById('vbox-sched-modal').style.display = 'flex';
      document.getElementById('modal-sched-title').innerText = `📋 Danh Sách Khung Giờ Đã Lưu Trên V-Box`;
      document.getElementById('modal-sched-subtitle').innerText = `Thiết bị: ${devName} • Đang kết nối đọc dữ liệu từ V-Box...`;
      document.getElementById('modal-sched-body').innerHTML = `
        <div style="text-align: center; padding: 32px 16px; color: var(--muted);">
          <div style="font-size: 2rem; margin-bottom: 8px;">⏳</div>
          <b>Đang đọc dữ liệu lịch thực tế từ V-Box...</b><br>
          <span style="font-size: 0.8rem;">(Kiểm tra danh sách đang lưu trong bộ nhớ V-Box, hoàn toàn không tác động đến thiết bị)</span>
        </div>
      `;

      // 1. Gửi lệnh yêu cầu đọc lịch xuống V-Box qua MQTT (chỉ đọc)
      await postMqttPayload(JSON.stringify({ cmd: "get_schedules", target_device: dev, device: dev }));

      // 2. Poll API /api/vbox_schedules để nhận phản hồi từ V-Box
      let attempts = 0;
      const pollInterval = setInterval(async () => {
        attempts++;
        try {
          const res = await fetch('/api/vbox_schedules');
          if (res.ok) {
            const data = await res.json();
            if (data && data.type === 'VBOX_SCHEDULES_RESP' && (data.device === dev || data.device === 'all')) {
              clearInterval(pollInterval);
              renderVboxSchedContent(dev, data);
              return;
            }
          }
        } catch (e) {}

        if (attempts >= 10) { // Timeout 5 giây
          clearInterval(pollInterval);
          document.getElementById('modal-sched-body').innerHTML = `
            <div style="background: #fef2f2; border: 1px solid #fecaca; border-radius: 8px; padding: 16px; color: #991b1b; text-align: center;">
              <b>Chưa nhận được phản hồi từ V-Box!</b><br>
              <span style="font-size: 0.82rem; color: #7f1d1d;">Đường truyền MQTT đang trễ hoặc V-Box chưa kịp gửi lại. Vui lòng bấm thử lại.</span>
            </div>
          `;
        }
      }, 500);
    }

    function renderVboxSchedContent(dev, data) {
      let targetData = (data.device === 'all' && data.all) ? data.all[dev] : data;
      const scheds = (targetData && targetData.schedules) ? targetData.schedules : [];
      const devName = devNames[dev] || dev;
      const maxSlots = getMaxSlots(dev);

      document.getElementById('modal-sched-subtitle').innerText = 
        `Thiết bị: ${devName} • Giới hạn: ${maxSlots} khung giờ • Đang lưu: ${scheds.length} khung giờ`;

      if (scheds.length === 0) {
        document.getElementById('modal-sched-body').innerHTML = `
          <div style="background: #fffbeb; border: 1px solid #fef3c7; border-radius: 8px; padding: 24px 16px; text-align: center; color: #92400e;">
            <div style="font-size: 2rem; margin-bottom: 8px;">⚠️</div>
            <b style="font-size: 0.98rem; color: #b45309;">V-BOX HIỆN CHƯA CÓ KHUNG GIỜ HẸN NÀO ĐƯỢC LƯU!</b><br>
            <p style="font-size: 0.84rem; color: #78350f; margin-top: 8px; line-height: 1.6;">
              Nếu bạn vừa chọn giờ ở ngoài màn hình nhưng chưa bấm <b>"Lưu & Kích Hoạt Lịch"</b>, V-Box sẽ chưa nhận được dữ liệu.<br>
              👉 <b>Cách làm:</b> Bấm nút <b>"Đóng Cửa Sổ"</b> bên dưới, ra ngoài kiểm tra các khung giờ đã chọn và bấm nút <b>"💾 Lưu & Kích Hoạt Lịch Cho Thiết Bị Này"</b>. Sau khi lưu xong, bấm mở lại cửa sổ này để đối soát kiểm tra xem đã lưu đúng chưa nhé!
            </p>
          </div>
        `;
        return;
      }

      let html = `
        <div style="margin-bottom: 14px; font-size: 0.84rem; color: #047857; background: #ecfdf5; border: 1px solid #a7f3d0; padding: 10px 14px; border-radius: 8px;">
          ✅ <b>ĐÃ ĐỐI SOÁT VỚI BỘ NHỚ V-BOX:</b> Đã lưu thành công <b>${scheds.length}</b> khung giờ hẹn đang kích hoạt chạy thực tế:
        </div>
        <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 10px;">
      `;

      scheds.forEach((s, idx) => {
        const freq = (s.day && s.month && s.day > 0) ? `Ngày ${s.day}/${s.month}` : `Hàng ngày`;
        const startStr = s.start || `${String(s.start_h).padStart(2,'0')}:${String(s.start_m).padStart(2,'0')}`;
        const stopStr = s.stop || `${String(s.end_h).padStart(2,'0')}:${String(s.end_m).padStart(2,'0')}`;

        html += `
          <div style="background: #ffffff; border: 1px solid var(--border); border-left: 4px solid #0284c7; border-radius: 8px; padding: 10px 14px; display: flex; justify-content: space-between; align-items: center; box-shadow: var(--shadow-sm);">
            <div>
              <span style="font-size: 0.72rem; font-weight: 700; color: #0284c7; background: #e0f2fe; padding: 2px 7px; border-radius: 4px;">#${idx + 1}</span>
              <span style="font-size: 0.92rem; font-weight: 700; color: var(--text); margin-left: 8px;">${startStr} ➔ ${stopStr}</span>
            </div>
            <span style="font-size: 0.78rem; color: #475569; font-weight: 500; background: #f1f5f9; padding: 2px 8px; border-radius: 9999px;">${freq}</span>
          </div>
        `;
      });

      html += `</div>`;
      document.getElementById('modal-sched-body').innerHTML = html;
    }

    function closeVboxSchedModal() {
      document.getElementById('vbox-sched-modal').style.display = 'none';
    }

    function onSwitchLabelClick(event, devKey) {
      if (isMistingDevice(devKey) && currentAutoHumidity) {
        event.preventDefault();
        event.stopPropagation();
        alert('Chế độ Tự Động Phun Sương đang BẬT!\n\nCác bơm phun sương đang tự động vận hành theo cảm biến độ ẩm, không thể bật/tắt thủ công (chỉ để xem trạng thái).\n\nNếu muốn điều khiển bằng tay, vui lòng gạt TẮT công tắc "Phun Sương (4 Bơm)" ở mục Điều Khiển Tự Động phía trên.');
      } else if (devKey === 'heater_1' && currentAutoTemp) {
        event.preventDefault();
        event.stopPropagation();
        alert('Chế độ Tự Động Sưởi đang BẬT!\n\nSưởi 1 đang tự động vận hành theo cảm biến nhiệt độ, không thể bật/tắt thủ công (chỉ để xem trạng thái).\n\nNếu muốn điều khiển bằng tay, vui lòng gạt TẮT công tắc "Sưởi 1" ở mục Điều Khiển Tự Động phía trên.');
      } else if (devKey === 'pump' && currentAutoPump) {
        event.preventDefault();
        event.stopPropagation();
        alert('Chế độ Tự Động Bơm Nước đang BẬT!\n\nMáy bơm đang tự động vận hành theo cảm biến mực nước bể chứa (khi thiếu nước tự bật, đủ nước tự dừng), không thể bật/tắt thủ công (chỉ để xem trạng thái).\n\nNếu muốn điều khiển bằng tay, vui lòng gạt TẮT công tắc "Máy Bơm Nước" ở mục Điều Khiển Tự Động phía trên.');
      }
    }

    function updateMistingControlLock(isAuto) {
      currentAutoHumidity = !!isAuto;
      PS_DEVICES.forEach(k => {
        const sw = document.getElementById('sw-' + k);
        const lbl = document.getElementById('lbl-sw-' + k);
        const lock = document.getElementById('lock-' + k);
        const item = document.getElementById('item-' + k);

        if (sw) sw.disabled = currentAutoHumidity;
        if (lbl) {
          if (currentAutoHumidity) {
            lbl.classList.add('disabled-switch');
            lbl.title = 'Đang chạy tự động theo cảm biến độ ẩm (Khóa điều khiển thủ công - Chỉ xem trạng thái)';
          } else {
            lbl.classList.remove('disabled-switch');
            lbl.title = '';
          }
        }
        if (lock) {
          lock.style.display = currentAutoHumidity ? 'inline-flex' : 'none';
        }
        if (item) {
          if (currentAutoHumidity) item.classList.add('item-auto-locked');
          else item.classList.remove('item-auto-locked');
        }
      });
      const curSchedDevH = document.getElementById('sched-device');
      if (curSchedDevH) updateStatusSummary(curSchedDevH.value);
    }

    function updateHeatingControlLock(isAuto) {
      currentAutoTemp = !!isAuto;
      const k = 'heater_1';
      const sw = document.getElementById('sw-' + k);
      const lbl = document.getElementById('lbl-sw-' + k);
      const lock = document.getElementById('lock-' + k);
      const item = document.getElementById('item-' + k);

      if (sw) sw.disabled = currentAutoTemp;
      if (lbl) {
        if (currentAutoTemp) {
          lbl.classList.add('disabled-switch');
          lbl.title = 'Đang chạy tự động theo cảm biến nhiệt độ (Khóa điều khiển thủ công - Chỉ xem trạng thái)';
        } else {
          lbl.classList.remove('disabled-switch');
          lbl.title = '';
        }
      }
      if (lock) {
        lock.style.display = currentAutoTemp ? 'inline-flex' : 'none';
      }
      if (item) {
        if (currentAutoTemp) item.classList.add('item-auto-locked');
        else item.classList.remove('item-auto-locked');
      }
      const curSchedDevT = document.getElementById('sched-device');
      if (curSchedDevT) updateStatusSummary(curSchedDevT.value);
    }

    function updatePumpControlLock(isAuto) {
      currentAutoPump = !!isAuto;
      const k = 'pump';
      const sw = document.getElementById('sw-' + k);
      const lbl = document.getElementById('lbl-sw-' + k);
      const lock = document.getElementById('lock-' + k);
      const item = document.getElementById('item-' + k);

      if (sw) sw.disabled = currentAutoPump;
      if (lbl) {
        if (currentAutoPump) {
          lbl.classList.add('disabled-switch');
          lbl.title = 'Đang chạy tự động theo cảm biến mực nước (Khóa điều khiển thủ công - Chỉ xem trạng thái)';
        } else {
          lbl.classList.remove('disabled-switch');
          lbl.title = '';
        }
      }
      if (lock) {
        lock.style.display = currentAutoPump ? 'inline-flex' : 'none';
      }
      if (item) {
        if (currentAutoPump) item.classList.add('item-auto-locked');
        else item.classList.remove('item-auto-locked');
      }
      const curSchedDevP = document.getElementById('sched-device');
      if (curSchedDevP) updateStatusSummary(curSchedDevP.value);
    }

    async function sendMqttControl(deviceKey, val) {
      if (isMistingDevice(deviceKey) && currentAutoHumidity) {
        alert('Chế độ Tự Động Phun Sương đang BẬT! Không thể bật/tắt thủ công các bơm phun sương.');
        refreshData();
        return;
      }
      if (deviceKey === 'heater_1' && currentAutoTemp) {
        alert('Chế độ Tự Động Sưởi đang BẬT! Không thể bật/tắt thủ công Sưởi 1.');
        refreshData();
        return;
      }
      if (deviceKey === 'pump' && currentAutoPump) {
        alert('Chế độ Tự Động Bơm Nước đang BẬT! Không thể bật/tắt thủ công Máy bơm.');
        refreshData();
        return;
      }
      const payload = `{"${deviceKey}": ${val}}`;
      await postMqttPayload(payload);
    }

    async function toggleAutoMode(type, isChecked) {
      let payload = {};
      if (type === 'humidity' || type === 'hum') {
        payload['auto_humidity'] = isChecked ? 1 : 0;
        updateMistingControlLock(isChecked);
        if (!isChecked) {
          PS_DEVICES.forEach(k => {
            const swEl = document.getElementById('sw-' + k);
            const pillEl = document.getElementById('pill-' + k);
            if (swEl) swEl.checked = false;
            if (pillEl) {
              pillEl.innerText = 'TẮT';
              pillEl.className = 'status-pill status-off';
            }
          });
        }
      } else if (type === 'temp') {
        payload['auto_temp'] = isChecked ? 1 : 0;
        updateHeatingControlLock(isChecked);
        if (!isChecked) {
          const swEl = document.getElementById('sw-heater_1');
          const pillEl = document.getElementById('pill-heater_1');
          if (swEl) swEl.checked = false;
          if (pillEl) {
            pillEl.innerText = 'TẮT';
            pillEl.className = 'status-pill status-off';
          }
        }
      } else if (type === 'pump') {
        payload['auto_pump'] = isChecked ? 1 : 0;
        updatePumpControlLock(isChecked);
        if (!isChecked) {
          const swEl = document.getElementById('sw-pump');
          const pillEl = document.getElementById('pill-pump');
          if (swEl) swEl.checked = false;
          if (pillEl) {
            pillEl.innerText = 'TẮT';
            pillEl.className = 'status-pill status-off';
          }
        }
      }
      await postMqttPayload(JSON.stringify(payload));
    }

    async function saveThreshold(type) {
      let payload = {};
      if (type === 'humidity' || type === 'hum') {
        const val = parseFloat(document.getElementById('input-hum-threshold').value);
        if (isNaN(val) || val < 0 || val > 100) {
          alert('Vui lòng nhập ngưỡng độ ẩm hợp lệ (0 - 100%)!');
          return;
        }
        const onVal = parseFloat(document.getElementById('input-ps-on-time').value);
        if (isNaN(onVal) || onVal <= 0) {
          alert('Vui lòng nhập thời gian phun hợp lệ (> 0)!');
          return;
        }
        const offVal = parseFloat(document.getElementById('input-ps-off-time').value);
        if (isNaN(offVal) || offVal < 0) {
          alert('Vui lòng nhập khoảng thời gian nghỉ hợp lệ (>= 0)!');
          return;
        }
        const onUnit = document.getElementById('unit-ps-on').value;
        const offUnit = document.getElementById('unit-ps-off').value;
        const onSec = Math.round(onUnit === 'm' ? onVal * 60 : onVal);
        const offSec = Math.round(offUnit === 'm' ? offVal * 60 : offVal);

        payload['hum_threshold'] = val;
        payload['ps_on_sec'] = onSec;
        payload['ps_off_sec'] = offSec;
        payload['ps_on_time'] = +(onSec / 60).toFixed(2);
        payload['ps_off_time'] = +(offSec / 60).toFixed(2);
      } else if (type === 'temp') {
        const val = parseFloat(document.getElementById('input-temp-threshold').value);
        if (isNaN(val) || val < -20 || val > 100) {
          alert('Vui lòng nhập ngưỡng nhiệt độ hợp lệ (-20 đến 100°C)!');
          return;
        }
        payload['temp_threshold'] = val;
      }
      await postMqttPayload(JSON.stringify(payload));
      alert('Đã lưu cấu hình và gửi xuống V-Box thành công!');
    }

    async function sendCustomMqtt() {
      const input = document.getElementById('custom-msg');
      const val = input.value.trim();
      if (!val) return;
      await postMqttPayload(val);
      input.value = '';
    }

    async function postMqttPayload(payload) {
      try {
        await fetch('/api/mqtt_publish', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: `payload=${encodeURIComponent(payload)}`
        });
        refreshData();
      } catch (err) {
        alert('Lỗi khi gửi lệnh qua MQTT!');
      }
    }

    async function refreshData() {
      try {
        const res = await fetch('/api/data');
        if (!res.ok) throw new Error('ERR');
        const data = await res.json();

        // 1. Cập nhật Banner cảnh báo & Badge RS485 PLC (Dựa trên Heartbeat PLC đứng yên > 15s)
        const alarmBanner = document.getElementById('alarm-banner');
        const badgePlc = document.getElementById('badge-plc');
        if (!data.plcCommOk) {
          alarmBanner.style.display = 'flex';
          if (data.plcAlarmMsg && data.plcAlarmMsg.length > 0) {
            document.getElementById('alarm-title').innerText = data.plcAlarmMsg;
          }
          badgePlc.className = 'badge badge-err';
          badgePlc.innerText = 'PLC RS485: MẤT KẾT NỐI!';
        } else {
          alarmBanner.style.display = 'none';
          badgePlc.className = 'badge badge-ok';
          badgePlc.innerText = 'PLC RS485: Kết nối tốt';
        }

        // 2. Cập nhật giá trị cảm biến
        document.getElementById('temp-val').innerText = data.temperature.toFixed(1);
        document.getElementById('hum-val').innerText = data.humidity.toFixed(1);

        // 2.1 Cập nhật Chế độ Tự động & Ngưỡng cài đặt Cảm biến (3 trạng thái)
        const swHum = document.getElementById('sw-auto-humidity');
        const bHum = document.getElementById('badge-mode-humidity');
        if (swHum && bHum) {
          swHum.checked = !!data.autoHumidityMode;
          updateMistingControlLock(!!data.autoHumidityMode);
          if (data.autoHumidityMode) {
            if (data.psCycleState == 1) {
              bHum.className = 'badge badge-ok';
              bHum.removeAttribute('style');
              let tDesc = data.psOnSec ? (data.psOnSec >= 60 ? `${(data.psOnSec/60).toFixed(1)}p` : `${data.psOnSec}s`) : `${data.psOnTime || 2}p`;
              bHum.innerText = `TỰ ĐỘNG: ĐANG PHUN (${tDesc})`;
            } else if (data.psCycleState == 2) {
              bHum.className = 'badge badge-hb';
              bHum.style.background = '#fef3c7';
              bHum.style.color = '#b45309';
              bHum.style.border = '1px solid #fde68a';
              let tDesc = data.psOffSec ? (data.psOffSec >= 60 ? `${(data.psOffSec/60).toFixed(1)}p` : `${data.psOffSec}s`) : `${data.psOffTime || 15}p`;
              bHum.innerText = `TỰ ĐỘNG: ĐANG NGHỈ LAN TỎA (${tDesc}) [KHÓA BƠM]`;
            } else {
              bHum.className = 'badge badge-ok';
              bHum.removeAttribute('style');
              bHum.innerText = 'TỰ ĐỘNG ĐỘ ẨM: ĐỦ ẨM (ĐANG CHỜ)';
            }
          } else {
            bHum.className = 'badge badge-hb';
            bHum.removeAttribute('style');
            bHum.innerText = 'ĐANG TẮT: THEO LỊCH HẸN RTC';
          }
        }

        const swTemp = document.getElementById('sw-auto-temp');
        const bTemp = document.getElementById('badge-mode-temp');
        if (swTemp && bTemp) {
          swTemp.checked = !!data.autoTempMode;
          updateHeatingControlLock(!!data.autoTempMode);
          if (data.autoTempMode) {
            bTemp.className = 'badge badge-ok';
            bTemp.innerText = 'ĐANG BẬT: TỰ ĐỘNG THEO NHIỆT ĐỘ';
          } else {
            bTemp.className = 'badge badge-hb';
            bTemp.innerText = 'ĐANG TẮT: THEO LỊCH HẸN RTC';
          }
        }

        const swPump = document.getElementById('sw-auto-pump');
        const bPump = document.getElementById('badge-mode-pump');
        if (swPump && bPump) {
          swPump.checked = !!data.autoPumpMode;
          updatePumpControlLock(!!data.autoPumpMode);
          if (data.autoPumpMode) {
            const isPumping = (data.devices && data.devices.pump == 1);
            if (data.waterOverflow == 1 || data.waterSafe == 0) {
              bPump.className = 'badge badge-err';
              bPump.innerText = 'TỰ ĐỘNG: ĐẦY/TRÀN NƯỚC (ĐÃ NGẮT BƠM)';
            } else if (isPumping) {
              bPump.className = 'badge badge-ok';
              bPump.innerText = 'TỰ ĐỘNG: ĐANG BƠM NƯỚC (CHỜ ĐẦY ĐỈNH)';
            } else {
              bPump.className = 'badge badge-ok';
              bPump.innerText = 'TỰ ĐỘNG: ĐỦ NƯỚC (CHỜ DÙNG CẠN ĐÁY)';
            }
          } else {
            bPump.className = 'badge badge-hb';
            bPump.innerText = 'ĐANG TẮT: THỦ CÔNG / LỊCH HẸN RTC';
          }
        }

        // Cập nhật giá trị ngưỡng & chu kỳ vào ô nhập liệu nếu người dùng không focus
        const inHum = document.getElementById('input-hum-threshold');
        if (inHum && document.activeElement !== inHum && data.humThreshold !== undefined) {
          inHum.value = Number(data.humThreshold).toFixed(1);
        }
        const inTemp = document.getElementById('input-temp-threshold');
        if (inTemp && document.activeElement !== inTemp && data.tempThreshold !== undefined) {
          inTemp.value = Number(data.tempThreshold).toFixed(1);
        }

        if (!window.psTimeInitialized && data.psOnSec !== undefined && data.psOnSec > 0) {
          window.psTimeInitialized = true;
          const inPsOn = document.getElementById('input-ps-on-time');
          const unitPsOn = document.getElementById('unit-ps-on');
          const inPsOff = document.getElementById('input-ps-off-time');
          const unitPsOff = document.getElementById('unit-ps-off');

          if (inPsOn && unitPsOn) {
            if (data.psOnSec >= 60 && data.psOnSec % 60 === 0) {
              inPsOn.value = data.psOnSec / 60;
              unitPsOn.value = 'm';
            } else {
              inPsOn.value = data.psOnSec;
              unitPsOn.value = 's';
            }
          }

          if (inPsOff && unitPsOff) {
            if (data.psOffSec >= 60 && data.psOffSec % 60 === 0) {
              inPsOff.value = data.psOffSec / 60;
              unitPsOff.value = 'm';
            } else {
              inPsOff.value = data.psOffSec;
              unitPsOff.value = 's';
            }
          }
        }

        // 3. Cập nhật trạng thái phản hồi thực tế (Feedback) & Switch của 12 thiết bị
        if (data.devices) {
          devKeys.forEach(k => {
            const isRunning = (data.devices[k] == 1);
            const swEl = document.getElementById('sw-' + k);
            const pillEl = document.getElementById('pill-' + k);

            if (swEl) swEl.checked = isRunning;
            if (pillEl) {
              pillEl.innerText = isRunning ? 'ĐANG BẬT' : 'TẮT';
              pillEl.className = isRunning ? 'status-pill status-on' : 'status-pill status-off';
            }
          });
        }

        // 3.1 Cập nhật Cảm biến Mực nước (@B_0#HDX5.0 & @B_0#HDX5.1)
        const bEmpty = document.getElementById('badge-water-empty');
        if (bEmpty) {
          if (data.waterEmpty == 1) {
            bEmpty.className = 'badge badge-err';
            bEmpty.innerText = '⚠️ THIẾU NƯỚC!';
          } else {
            bEmpty.className = 'badge badge-ok';
            bEmpty.innerText = 'ĐỦ NƯỚC';
          }
        }
        const bOver = document.getElementById('badge-water-overflow');
        if (bOver) {
          if (data.waterOverflow == 1 || data.waterSafe == 0) {
            bOver.className = 'badge badge-err';
            bOver.innerText = '🚨 TRÀN NƯỚC!';
          } else {
            bOver.className = 'badge badge-ok';
            bOver.innerText = 'CHƯA TRÀN';
          }
        }

        // 3.2 Cập nhật Cảm biến Âm thanh (dB)
        if (data.sound) {
          const s1El = document.getElementById('val-sound-1');
          const s2El = document.getElementById('val-sound-2');
          const s3El = document.getElementById('val-sound-3');
          if (s1El) s1El.innerText = Number(data.sound.s1).toFixed(1);
          if (s2El) s2El.innerText = Number(data.sound.s2).toFixed(1);
          if (s3El) s3El.innerText = Number(data.sound.s3).toFixed(1);
        }

        // 3.3 Cập nhật Cảm biến Dòng điện (A)
        if (data.current) {
          const curKeys = ['light_1', 'light_2', 'misting_1', 'misting_2', 'misting_3', 'speaker_1', 'speaker_2', 'pump'];
          curKeys.forEach(ck => {
            const val = data.current[ck];
            const curVal = Number(val || 0).toFixed(2);
            const el = document.getElementById('cur-' + ck);
            if (el) el.innerText = curVal;
            const tagEl = document.getElementById('tag-cur-' + ck);
            if (tagEl) tagEl.innerText = curVal + ' A';
          });
        }

        // 4. Badge MQTT
        const bMqtt = document.getElementById('badge-mqtt');
        if (data.mqttConnected) {
          bMqtt.innerText = 'MQTT: Đã kết nối broker.emqx.io';
          bMqtt.className = 'badge badge-ok';
        } else {
          bMqtt.innerText = 'MQTT: Mất kết nối';
          bMqtt.className = 'badge badge-err';
        }

        // 5. Badge Heartbeat Kép: V-Box (@W_0#HDW12) và PLC (@W_0#HDW13)
        const bHb = document.getElementById('badge-hb');
        if (data.heartbeatAlive) {
          bHb.innerText = `V-Box HB: ${data.heartbeat}/60`;
          bHb.className = 'badge badge-hb';
        } else {
          bHb.innerText = `V-Box HB: Mất tín hiệu`;
          bHb.className = 'badge badge-err';
        }

        const bPlcHb = document.getElementById('badge-plc-hb');
        bPlcHb.innerText = `PLC HB: ${data.plcHeartbeat}/60`;
        bPlcHb.className = data.plcCommOk ? 'badge badge-plchb' : 'badge badge-err';

        // 6. Badge thời gian & ngày V-Box
        let timeStr = data.vboxTime || '--:--:--';
        if (data.vboxDate && data.vboxDate !== '--/--/----') {
          timeStr += ' • ' + data.vboxDate;
        }
        document.getElementById('badge-time').innerText = 'V-Box: ' + timeStr;

        // 7. Log box
        let commStatusStr = data.plcCommOk ? '<span style="color:#059669;font-weight:600">Kết nối tốt</span>' : '<span style="color:#dc2626;font-weight:bold">MẤT TRUYỀN THÔNG (>15s)!</span>';
        document.getElementById('mqtt-log').innerHTML = 
          `<b>PLC RS485:</b> ${commStatusStr} | <b>V-Box HB:</b> ${data.heartbeat}/60 | <b>PLC HB:</b> ${data.plcHeartbeat}/60<br>` +
          `<b>Dữ liệu MQTT nhận:</b> ${data.lastReceived}<br>` +
          `<b>Lệnh MQTT gửi:</b> ${data.lastPublished}<br>` +
          `<b>ESP32 IP:</b> ${data.ip} | <b>RSSI:</b> ${data.rssi} dBm`;

      } catch (e) {
        document.getElementById('badge-mqtt').innerText = 'Mất kết nối ESP32';
        document.getElementById('badge-mqtt').className = 'badge badge-err';
      }
    }

    document.getElementById('custom-msg').addEventListener('keypress', function(e) {
      if (e.key === 'Enter') sendCustomMqtt();
    });

    window.onload = function() {
      initDefaultSchedules();
      onDeviceSelectChange();
      refreshData();
      setInterval(refreshData, 2000);
    };
  </script>
</body>
</html>
)rawliteral";

// ==============================================================================
// 6. XỬ LÝ NHẬN TIN NHẮN TỪ MQTT BROKER
// ==============================================================================
// Hàm thoát ký tự đặc biệt để tạo chuỗi JSON an toàn cho Web API
String escapeJsonString(const String& input) {
  String output = "";
  output.reserve(input.length() + 16);
  for (size_t i = 0; i < input.length(); i++) {
    char c = input[i];
    if (c == '\"') output += "\\\"";
    else if (c == '\\') output += "\\\\";
    else if (c == '\n') output += "\\n";
    else if (c == '\r') output += "\\r";
    else if (c == '\t') output += "\\t";
    else output += c;
  }
  return output;
}

// Hàm trích xuất giá trị số từ chuỗi JSON (không phân biệt hoa/thường)
float extractJsonNumber(const String& json, const String& key) {
  String lowerJson = json;
  lowerJson.toLowerCase();
  String lowerKey = key;
  lowerKey.toLowerCase();

  int keyIndex = lowerJson.indexOf("\"" + lowerKey + "\"");
  if (keyIndex == -1) keyIndex = lowerJson.indexOf(lowerKey);
  if (keyIndex == -1) return -999.0;

  int colonIndex = json.indexOf(':', keyIndex);
  if (colonIndex == -1) return -999.0;

  int startIndex = colonIndex + 1;
  while (startIndex < json.length() && (json[startIndex] == ' ' || json[startIndex] == '\"')) {
    startIndex++;
  }

  int endIndex = startIndex;
  while (endIndex < json.length() && (isDigit(json[endIndex]) || json[endIndex] == '.' || json[endIndex] == '-')) {
    endIndex++;
  }

  if (startIndex < endIndex) {
    return json.substring(startIndex, endIndex).toFloat();
  }
  return -999.0;
}

// Hàm trích xuất chuỗi từ JSON (dùng lấy thời gian hoặc thông điệp cảnh báo)
String extractJsonString(const String& json, const String& key) {
  String lowerJson = json;
  lowerJson.toLowerCase();
  String lowerKey = key;
  lowerKey.toLowerCase();

  int keyIndex = lowerJson.indexOf("\"" + lowerKey + "\"");
  if (keyIndex == -1) keyIndex = lowerJson.indexOf(lowerKey);
  if (keyIndex == -1) return "";

  int colonIndex = json.indexOf(':', keyIndex);
  if (colonIndex == -1) return "";

  int firstQuote = json.indexOf('\"', colonIndex);
  if (firstQuote == -1) return "";

  int secondQuote = json.indexOf('\"', firstQuote + 1);
  if (secondQuote == -1) return "";

  return json.substring(firstQuote + 1, secondQuote);
}

// Hàm cập nhật trạng thái phản hồi thực tế của 12 thiết bị từ chuỗi JSON
void updateDeviceStatesFromMsg(const String& msg) {
  const char* devKeysEng[] = {
    "misting_1", "misting_2", "misting_3", "misting_4",
    "light_1", "light_2", "heater_1", "speaker_1", "speaker_2", "speaker_3",
    "pump", "insect_trap"
  };
  bool* devPtrs[] = {
    &stateMisting1, &stateMisting2, &stateMisting3, &stateMisting4,
    &stateLight1, &stateLight2, &stateHeater1, &stateSpeaker1, &stateSpeaker2, &stateSpeaker3,
    &statePump, &stateInsectTrap
  };

  for (int i = 0; i < 12; i++) {
    float val = extractJsonNumber(msg, devKeysEng[i]);
    if (val != -999.0) {
      *(devPtrs[i]) = (val > 0.5);
    }
  }

  // Cập nhật chế độ tự động & ngưỡng cài đặt cảm biến nếu có trong payload
  float at = extractJsonNumber(msg, "auto_temp");
  if (at != -999.0) autoTempMode = (at > 0.5);

  float ah = extractJsonNumber(msg, "auto_humidity");
  if (ah != -999.0) autoHumidityMode = (ah > 0.5);

  float ap = extractJsonNumber(msg, "auto_pump");
  if (ap != -999.0) autoPumpMode = (ap > 0.5);

  float tt = extractJsonNumber(msg, "temp_threshold");
  if (tt != -999.0) tempThreshold = tt;

  float ht = extractJsonNumber(msg, "hum_threshold");
  if (ht != -999.0) humThreshold = ht;

  float pOnSec = extractJsonNumber(msg, "ps_on_sec");
  if (pOnSec != -999.0) {
    psAutoOnSec = (int)pOnSec;
    psAutoOnTime = psAutoOnSec / 60.0;
  } else {
    float pOn = extractJsonNumber(msg, "ps_on_time");
    if (pOn != -999.0) {
      psAutoOnTime = pOn;
      psAutoOnSec = (int)(pOn * 60);
    }
  }

  float pOffSec = extractJsonNumber(msg, "ps_off_sec");
  if (pOffSec != -999.0) {
    psAutoOffSec = (int)pOffSec;
    psAutoOffTime = psAutoOffSec / 60.0;
  } else {
    float pOff = extractJsonNumber(msg, "ps_off_time");
    if (pOff != -999.0) {
      psAutoOffTime = pOff;
      psAutoOffSec = (int)(pOff * 60);
    }
  }

  float pState = extractJsonNumber(msg, "ps_cycle_state");
  if (pState != -999.0) psCycleState = (int)pState;
}

// Callback khi nhận tin nhắn MQTT từ Broker
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print(F("[MQTT NHẬN] Topic: "));
  Serial.print(topic);
  Serial.print(F(" | Nội dung: "));
  Serial.println(message);

  lastMqttReceived = message;
  lastMqttMsgTime  = millis();

  // 1. Nhận cảnh báo tức thời từ V-Box (Topic: 3FAMIOT/ALARM)
  if (String(topic) == TOPIC_SUB_ALARM) {
    float comm = extractJsonNumber(message, "plc_comm");
    if (comm != -999.0) {
      plcCommOk = (comm > 0.5);
    }
    String msg = extractJsonString(message, "message");
    if (msg.length() > 0) {
      plcAlarmMsg = msg;
    }
    Serial.printf("[CẢNH BÁO RS485 PLC] -> %s (Trạng thái: %s)\n", 
      plcAlarmMsg.c_str(), plcCommOk ? "KẾT NỐI LẠI" : "MẤT KẾT NỐI");
  }

  // 2. Nhận dữ liệu Nhiệt độ, Độ ẩm, 2 Heartbeat & Trạng thái từ V-Box (Topic: 3FAMIOT/HMI_PUB)
  else if (String(topic) == TOPIC_SUB_DATA) {
    // 2.0 Kiểm tra phản hồi danh sách lịch thực tế từ V-Box
    if (message.indexOf("\"VBOX_SCHEDULES_RESP\"") != -1) {
      lastVboxSchedulesResp = message;
      lastVboxSchedulesTime = millis();
      Serial.println(F("[MQTT NHẬN LỊCH V-BOX] Đã nhận danh sách lịch thực tế từ V-Box!"));
      return;
    }

    float temp = extractJsonNumber(message, "Tempeturate");
    if (temp == -999.0) temp = extractJsonNumber(message, "Temperature");
    if (temp == -999.0) temp = extractJsonNumber(message, "temp");

    float hum = extractJsonNumber(message, "Humidity");
    if (hum == -999.0) hum = extractJsonNumber(message, "hum");

    if (temp != -999.0) currentTemperature = temp;
    if (hum  != -999.0) currentHumidity    = hum;

    // Trích xuất Heartbeat V-Box -> PLC (@W_0#HDW12)
    float hb = extractJsonNumber(message, "heartbeat");
    if (hb == -999.0) hb = extractJsonNumber(message, "kt_ket_noi");
    if (hb != -999.0) {
      vboxHeartbeat = (int)hb;
      lastHeartbeatTime = millis();
    }

    // Trích xuất Heartbeat PLC -> V-Box (@W_0#HDW13)
    float plcHb = extractJsonNumber(message, "plc_heartbeat");
    if (plcHb != -999.0) {
      plcHeartbeat = (int)plcHb;
    }

    // Trích xuất trạng thái truyền thông RS485 với PLC
    float comm = extractJsonNumber(message, "plc_comm");
    if (comm != -999.0) {
      plcCommOk = (comm > 0.5);
      if (!plcCommOk) {
        String al = extractJsonString(message, "alarm");
        plcAlarmMsg = (al.length() > 0) ? al : "CẢNH BÁO: MẤT TRUYỀN THÔNG RS485 VỚI PLC (Heartbeat đứng yên > 15s)!";
      } else {
        plcAlarmMsg = "";
      }
    }

    // Cập nhật trạng thái phản hồi thực tế của 10 thiết bị (@B_0#HDX1.0 -> @B_0#HDX1.9)
    updateDeviceStatesFromMsg(message);

    // Trích xuất cảm biến mực nước (@B_0#HDX5.0 & @B_0#HDX5.1)
    float wEmpty = extractJsonNumber(message, "water_empty");
    if (wEmpty != -999.0) waterEmpty = (int)wEmpty;

    float wSafe = extractJsonNumber(message, "water_safe");
    if (wSafe != -999.0) {
      waterSafe = (int)wSafe;
      waterOverflow = (waterSafe == 0) ? 1 : 0;
    }
    float wOver = extractJsonNumber(message, "water_overflow");
    if (wOver != -999.0) waterOverflow = (int)wOver;

    // Trích xuất cảm biến âm thanh (dB)
    float s1 = extractJsonNumber(message, "sound_1");
    if (s1 != -999.0) sound1 = s1;
    float s2 = extractJsonNumber(message, "sound_2");
    if (s2 != -999.0) sound2 = s2;
    float s3 = extractJsonNumber(message, "sound_3");
    if (s3 != -999.0) sound3 = s3;

    // Trích xuất 8 cảm biến dòng điện (A) theo mapping.txt
    float cLight1 = extractJsonNumber(message, "cur_light_1");
    if (cLight1 != -999.0) curLight1 = cLight1;

    float cLight2 = extractJsonNumber(message, "cur_light_2");
    if (cLight2 != -999.0) curLight2 = cLight2;

    float cPs1 = extractJsonNumber(message, "cur_misting_1");
    if (cPs1 != -999.0) curMisting1 = cPs1;

    float cPs2 = extractJsonNumber(message, "cur_misting_2");
    if (cPs2 != -999.0) curMisting2 = cPs2;

    float cPs3 = extractJsonNumber(message, "cur_misting_3");
    if (cPs3 != -999.0) curMisting3 = cPs3;

    float cLoa1 = extractJsonNumber(message, "cur_speaker_1");
    if (cLoa1 != -999.0) curSpeaker1 = cLoa1;

    float cLoa2 = extractJsonNumber(message, "cur_speaker_2");
    if (cLoa2 != -999.0) curSpeaker2 = cLoa2;

    float cPump = extractJsonNumber(message, "cur_pump");
    if (cPump != -999.0) curPump = cPump;

    Serial.printf("[V-BOX DATA] -> Temp: %.1f °C, Hum: %.1f %%, VBox_HB: %d/60, PLC_HB: %d/60, Water: [Thiếu:%d, Tràn:%d], Sound1: %.1fdB\n", 
      currentTemperature, currentHumidity, vboxHeartbeat, plcHeartbeat, waterEmpty, waterOverflow, sound1);
  }

  // 3. Nhận thời gian thực từ V-Box (Topic: 3FAMIOT/TIME)
  else if (String(topic) == TOPIC_SUB_TIME) {
    String t = extractJsonString(message, "time");
    String d = extractJsonString(message, "date");
    if (t.length() > 0) vboxTime = t;
    if (d.length() > 0) vboxDate = d;
    Serial.printf("[V-BOX TIME & DATE] -> Giờ: %s | Ngày: %s\n", vboxTime.c_str(), vboxDate.c_str());
  }
}

// ==============================================================================
// 7. KẾT NỐI & TỰ ĐỘNG KẾT NỐI LẠI MQTT
// ==============================================================================
void reconnectMqtt() {
  if (WiFi.status() != WL_CONNECTED) return;

  Serial.print(F("Đang kết nối MQTT Broker (broker.emqx.io)... "));
  String clientId = "ESP32_VBoxClient_" + String(random(0xffff), HEX);

  if (mqttClient.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
    Serial.println(F("THÀNH CÔNG!"));
    
    // Subscribe các topic đồng bộ với V-Box
    mqttClient.subscribe(TOPIC_SUB_DATA);
    mqttClient.subscribe(TOPIC_SUB_TIME);
    mqttClient.subscribe(TOPIC_SUB_ALARM);

    Serial.print(F("-> Đã Subscribe: "));
    Serial.println(TOPIC_SUB_DATA);
    Serial.print(F("-> Đã Subscribe: "));
    Serial.println(TOPIC_SUB_TIME);
    Serial.print(F("-> Đã Subscribe: "));
    Serial.println(TOPIC_SUB_ALARM);
  } else {
    Serial.print(F("Thất bại, mã lỗi rc="));
    Serial.println(mqttClient.state());
  }
}

// ==============================================================================
// 8. CÁC API ENDPOINTS CỦA WEB SERVER
// ==============================================================================

// GET / : Trả về trang Web Dashboard (Dùng send_P đọc trực tiếp từ Flash ROM theo từng gói nhỏ, không tốn RAM Heap)
void handleRoot() {
  server.send_P(200, "text/html; charset=utf-8", INDEX_HTML, sizeof(INDEX_HTML) - 1);
}

// GET /api/data : Trả về dữ liệu JSON cho giao diện Web tự động cập nhật
void handleGetData() {
  bool isHeartbeatAlive = (lastHeartbeatTime > 0) && ((millis() - lastHeartbeatTime) < 15000);

  String json = "{";
  json.reserve(3072);
  json += "\"temperature\":" + String(currentTemperature, 1) + ",";
  json += "\"humidity\":" + String(currentHumidity, 1) + ",";
  json += "\"heartbeat\":" + String(vboxHeartbeat) + ",";
  json += "\"plcHeartbeat\":" + String(plcHeartbeat) + ",";
  json += "\"heartbeatAlive\":" + String(isHeartbeatAlive ? "true" : "false") + ",";
  json += "\"plcCommOk\":" + String(plcCommOk ? "true" : "false") + ",";
  json += "\"plcAlarmMsg\":\"" + escapeJsonString(plcAlarmMsg) + "\",";
  json += "\"tempThreshold\":" + String(tempThreshold, 1) + ",";
  json += "\"humThreshold\":" + String(humThreshold, 1) + ",";
  json += "\"psOnTime\":" + String(psAutoOnTime, 1) + ",";
  json += "\"psOffTime\":" + String(psAutoOffTime, 1) + ",";
  json += "\"psOnSec\":" + String(psAutoOnSec) + ",";
  json += "\"psOffSec\":" + String(psAutoOffSec) + ",";
  json += "\"psCycleState\":" + String(psCycleState) + ",";
  json += "\"autoTempMode\":" + String(autoTempMode ? "true" : "false") + ",";
  json += "\"autoHumidityMode\":" + String(autoHumidityMode ? "true" : "false") + ",";
  json += "\"autoPumpMode\":" + String(autoPumpMode ? "true" : "false") + ",";
  json += "\"vboxTime\":\"" + vboxTime + "\",";
  json += "\"vboxDate\":\"" + vboxDate + "\",";
  json += "\"mqttConnected\":" + String(mqttClient.connected() ? "true" : "false") + ",";
  json += "\"waterEmpty\":" + String(waterEmpty) + ",";
  json += "\"waterSafe\":" + String(waterSafe) + ",";
  json += "\"waterOverflow\":" + String(waterOverflow) + ",";
  json += "\"sound\":{";
  json += "\"s1\":" + String(sound1, 1) + ",";
  json += "\"s2\":" + String(sound2, 1) + ",";
  json += "\"s3\":" + String(sound3, 1);
  json += "},";
  json += "\"current\":{";
  json += "\"light_1\":" + String(curLight1, 2) + ",";
  json += "\"light_2\":" + String(curLight2, 2) + ",";
  json += "\"misting_1\":" + String(curMisting1, 2) + ",";
  json += "\"misting_2\":" + String(curMisting2, 2) + ",";
  json += "\"misting_3\":" + String(curMisting3, 2) + ",";
  json += "\"speaker_1\":" + String(curSpeaker1, 2) + ",";
  json += "\"speaker_2\":" + String(curSpeaker2, 2) + ",";
  json += "\"pump\":" + String(curPump, 2);
  json += "},";
  json += "\"devices\":{";
  json += "\"misting_1\":" + String(stateMisting1 ? 1 : 0) + ",";
  json += "\"misting_2\":" + String(stateMisting2 ? 1 : 0) + ",";
  json += "\"misting_3\":" + String(stateMisting3 ? 1 : 0) + ",";
  json += "\"misting_4\":" + String(stateMisting4 ? 1 : 0) + ",";
  json += "\"light_1\":" + String(stateLight1 ? 1 : 0) + ",";
  json += "\"light_2\":" + String(stateLight2 ? 1 : 0) + ",";
  json += "\"heater_1\":" + String(stateHeater1 ? 1 : 0) + ",";
  json += "\"speaker_1\":" + String(stateSpeaker1 ? 1 : 0) + ",";
  json += "\"speaker_2\":" + String(stateSpeaker2 ? 1 : 0) + ",";
  json += "\"speaker_3\":" + String(stateSpeaker3 ? 1 : 0) + ",";
  json += "\"pump\":" + String(statePump ? 1 : 0) + ",";
  json += "\"insect_trap\":" + String(stateInsectTrap ? 1 : 0);
  json += "},";
  json += "\"lastReceived\":\"" + escapeJsonString(lastMqttReceived) + "\",";
  json += "\"lastPublished\":\"" + escapeJsonString(lastMqttPublished) + "\",";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"rssi\":" + String(WiFi.RSSI());
  json += "}";

  server.send(200, "application/json; charset=utf-8", json);
}

// GET /api/vbox_schedules : Trả về dữ liệu JSON lịch thực tế nhận được từ V-Box
void handleGetVboxSchedules() {
  server.send(200, "application/json; charset=utf-8", lastVboxSchedulesResp);
}

// POST /api/mqtt_publish : Nhận payload từ Web và Publish lên MQTT (Topic: 3FAMIOT/HMI_SUB)
void handleMqttPublish() {
  if (server.hasArg("payload")) {
    String payload = server.arg("payload");

    // Nếu payload có thay đổi chế độ tự động, đồng bộ biến trạng thái trước
    float ah = extractJsonNumber(payload, "auto_humidity");
    if (ah != -999.0) {
      autoHumidityMode = (ah > 0.5);
      if (!autoHumidityMode) {
        stateMisting1 = false;
        stateMisting2 = false;
        stateMisting3 = false;
        stateMisting4 = false;
      }
    }
    float at = extractJsonNumber(payload, "auto_temp");
    if (at != -999.0) {
      autoTempMode = (at > 0.5);
      if (!autoTempMode) {
        stateHeater1 = false;
      }
    }
    float ap = extractJsonNumber(payload, "auto_pump");
    if (ap != -999.0) {
      autoPumpMode = (ap > 0.5);
      if (!autoPumpMode) {
        statePump = false;
      }
    }

    // Nếu đang ở chế độ tự động độ ẩm, từ chối lệnh bật/tắt thủ công cho 4 bơm phun sương (chỉ cho phép xem trạng thái)
    if (autoHumidityMode && payload.indexOf("\"cmd\"") == -1 && payload.indexOf("schedules") == -1) {
      const char* psKeys[] = {
        "\"misting_1\"", "\"misting_2\"", "\"misting_3\"", "\"misting_4\""
      };
      for (int i = 0; i < 4; i++) {
        if (payload.indexOf(psKeys[i]) != -1) {
          Serial.printf("[CẢNH BÁO] Chế độ Tự động độ ẩm đang BẬT -> Khóa lệnh thủ công cho %s!\n", psKeys[i]);
          server.send(403, "text/plain; charset=utf-8", "Chế độ Tự động đang BẬT! Không thể điều khiển thủ công phun sương.");
          return;
        }
      }
    }

    // Nếu đang ở chế độ tự động nhiệt độ, từ chối lệnh bật/tắt thủ công cho Sưởi 1
    if (autoTempMode && payload.indexOf("\"cmd\"") == -1 && payload.indexOf("schedules") == -1) {
      if (payload.indexOf("\"heater_1\"") != -1) {
        Serial.println(F("[CẢNH BÁO] Chế độ Tự động nhiệt độ đang BẬT -> Khóa lệnh thủ công cho Sưởi 1!"));
        server.send(403, "text/plain; charset=utf-8", "Chế độ Tự động đang BẬT! Không thể điều khiển thủ công Sưởi 1.");
        return;
      }
    }

    // Nếu đang ở chế độ tự động bơm nước, từ chối lệnh bật/tắt thủ công cho Máy bơm
    if (autoPumpMode && payload.indexOf("\"cmd\"") == -1 && payload.indexOf("schedules") == -1) {
      if (payload.indexOf("\"pump\"") != -1) {
        Serial.println(F("[CẢNH BÁO] Chế độ Tự động bơm nước đang BẬT -> Khóa lệnh thủ công cho Máy bơm!"));
        server.send(403, "text/plain; charset=utf-8", "Chế độ Tự động đang BẬT! Không thể điều khiển thủ công Máy bơm.");
        return;
      }
    }

    if (mqttClient.connected()) {
      mqttClient.publish(TOPIC_PUB_CMD, payload.c_str());
      lastMqttPublished = payload;
      Serial.print(F("[MQTT GỬI ĐẾN V-BOX] Topic: "));
      Serial.print(TOPIC_PUB_CMD);
      Serial.print(F(" | Payload: "));
      Serial.println(payload);

      // Cập nhật tạm thời switch trên web khi vừa bấm gạt
      updateDeviceStatesFromMsg(payload);

      server.send(200, "text/plain; charset=utf-8", "Đã gửi qua MQTT");
    } else {
      server.send(503, "text/plain; charset=utf-8", "Lỗi: Mất kết nối MQTT Broker!");
    }
  } else {
    server.send(400, "text/plain; charset=utf-8", "Thiếu tham số payload");
  }
}

// ==============================================================================
// 9. SETUP & VÒNG LẶP CHÍNH (LOOP)
// ==============================================================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println(F("\n=========================================="));
  Serial.println(F(" KHỞI ĐỘNG V-BOX ESP32 MQTT WEB SERVER "));
  Serial.println(F("=========================================="));

#ifdef USE_DHT_SENSOR
  dht.begin();
  Serial.println(F("Đã khởi động cảm biến DHT."));
#endif

  // Kết nối Wi-Fi Station
  WiFi.mode(WIFI_AP_STA);
  Serial.print(F("Đang kết nối Wi-Fi: "));
  Serial.println(ssid_sta);
  WiFi.begin(ssid_sta, password_sta);

  unsigned long startWifi = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startWifi < 10000) {
    delay(500);
    Serial.print(F("."));
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("-> ĐÃ KẾT NỐI WI-FI THÀNH CÔNG!"));
    Serial.print(F("-> ĐỊA CHỈ TRUY CẬP WEB (STA): http://"));
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(F("-> Không kết nối được Wi-Fi chính, bật Access Point dự phòng."));
  }

  // Phát mạng AP dự phòng
  WiFi.softAP(ssid_ap, password_ap);
  Serial.print(F("-> ĐIỂM PHÁT AP DỰ PHÒNG: "));
  Serial.println(ssid_ap);
  Serial.print(F("-> TRUY CẬP QUA AP: http://"));
  Serial.println(WiFi.softAPIP());

  // Cấu hình MQTT với buffer 4096 bytes (đáp ứng 24 khung giờ)
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(4096);

  // Cấu hình Route cho Web Server
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/data", HTTP_GET, handleGetData);
  server.on("/api/vbox_schedules", HTTP_GET, handleGetVboxSchedules);
  server.on("/api/mqtt_publish", HTTP_POST, handleMqttPublish);

  server.begin();
  Serial.println(F("-> Web Server đã sẵn sàng trên cổng 80."));
  Serial.println(F("==========================================\n"));
}

void loop() {
  // Xử lý các request từ trình duyệt Web
  server.handleClient();

  // Duy trì kết nối MQTT
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected()) {
      unsigned long now = millis();
      if (now - lastReconnectAttempt > 5000) { // Thử kết nối lại mỗi 5 giây
        lastReconnectAttempt = now;
        reconnectMqtt();
      }
    } else {
      mqttClient.loop();
    }
  }

  // Nếu có bật cảm biến DHT riêng
#ifdef USE_DHT_SENSOR
  static unsigned long lastDhtRead = 0;
  if (millis() - lastDhtRead > 5000) {
    lastDhtRead = millis();
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
      currentTemperature = t;
      currentHumidity = h;
    }
  }
#endif
}
