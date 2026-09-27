# HỆ THỐNG IOT GIÁM SÁT & ĐIỀU KHIỂN V-BOX PLC (WEB NEXT.JS & MOBILE FLUTTER)

Dự án hiện đại hóa hệ thống điều khiển công nghiệp Wecon V-BOX và PLC, thay thế vai trò máy chủ cục bộ của ESP32 bằng:
1. **Web App (Next.js 13/14 + Tailwind CSS):** Triển khai toàn cầu trên Vercel, kết nối trực tiếp MQTT WebSocket (WSS), độ trễ phản hồi < 50ms.
2. **Mobile App (Flutter Multiplatform):** Ứng dụng di động mượt mà cho Android & iOS, tích hợp Push Notifications cảnh báo sự cố RS485 tức thời.
3. **Cơ sở dữ liệu Supabase (PostgreSQL Cloud):** Lưu trữ lịch sử thông số môi trường (nhiệt độ, độ ẩm), lịch hẹn 24 khung giờ và nhật ký sự cố.

---

## 1. Cấu Trúc Thư Mục

```
iot_web_app/
├── web/                       # Ứng dụng Web Next.js (Deploy Vercel)
│   ├── src/
│   │   ├── app/               # Next.js App Router (page.tsx, layout.tsx, globals.css)
│   │   ├── components/        # ConnectionBar, AlarmBanner, EnvironmentalPanel, DeviceGrid, ScheduleModal,...
│   │   └── lib/               # mqtt-client.ts, supabase.ts, types.ts
│   ├── package.json
│   ├── vercel.json            # Cấu hình tự động deploy Vercel
│   └── .env.local             # Cấu hình biến môi trường MQTT & Supabase
│
├── mobile/                    # Ứng dụng Di Động Flutter (Android / iOS)
│   ├── lib/
│   │   ├── models/            # DeviceModel, TelemetryModel, ScheduleSlotModel
│   │   ├── services/          # mqtt_service.dart, notification_service.dart
│   │   ├── providers/         # iot_provider.dart (State Management)
│   │   ├── screens/           # DashboardScreen, SettingsScreen, ScheduleDialog
│   │   └── widgets/           # GaugeWidget, DeviceCard, AlarmBanner
│   └── pubspec.yaml
│
├── supabase/
│   └── schema.sql             # SQL Script tạo bảng dữ liệu, RLS và Policy trên Supabase
│
├── vbox.txt                   # Mã nguồn Lua chạy trên Wecon V-BOX Gateway
├── mapping.txt                # Bảng đối chiếu địa chỉ thanh ghi PLC Modbus
└── vboxesp32/                 # Mã nguồn Arduino ESP32 cũ (lưu trữ tham khảo)
```

---

## 2. Bảng Đối Chiếu Thanh Ghi & Thiết Bị

| Thiết Bị | Lệnh Điều Khiển (Ghi) | Phản Hồi Trạng Thái (Đọc) | Số Khung Giờ Hỗ Trợ |
| :--- | :--- | :--- | :---: |
| **Phun sương 1** | `@B_0#HDX0.0` | `@B_0#HDX1.0` | 24 khung giờ |
| **Phun sương 2** | `@B_0#HDX0.1` | `@B_0#HDX1.1` | 24 khung giờ |
| **Phun sương 3** | `@B_0#HDX0.2` | `@B_0#HDX1.2` | 24 khung giờ |
| **Phun sương 4** | `@B_0#HDX0.3` | `@B_0#HDX1.3` | 24 khung giờ |
| **Đèn 1** | `@B_0#HDX0.4` | `@B_0#HDX1.4` | 10 khung giờ |
| **Đèn 2** | `@B_0#HDX0.5` | `@B_0#HDX1.5` | 10 khung giờ |
| **Sưởi 1** | `@B_0#HDX0.6` | `@B_0#HDX1.6` | 10 khung giờ |
| **Loa 1** | `@B_0#HDX0.7` | `@B_0#HDX1.7` | 10 khung giờ |
| **Loa 2** | `@B_0#HDX0.8` | `@B_0#HDX1.8` | 10 khung giờ |
| **Loa 3** | `@B_0#HDX0.9` | `@B_0#HDX1.9` | 10 khung giờ |

- **Nhiệt độ:** `@W_0#HDW10`
- **Độ ẩm:** `@W_0#HDW11`
- **Heartbeat V-Box &rarr; PLC:** `@W_0#HDW12` (0 &rarr; 60)
- **Heartbeat PLC &rarr; V-Box:** `@W_0#HDW13` (0 &rarr; 60, đứng yên > 15s báo mất truyền thông RS485)

---

## 3. Hướng Dẫn Chạy & Deploy Web App (Next.js)

### 3.1. Chạy trên máy tính (Local Development)
1. Mở Terminal tại thư mục `web`:
   ```bash
   cd d:\iot_web_app\web
   npm run dev
   ```
2. Mở trình duyệt truy cập: `http://localhost:3000`
   - Giao diện sẽ tự động kết nối tới MQTT Broker qua `wss://broker.emqx.io:8084/mqtt`.
   - Trạng thái 10 thiết bị và cảm biến sẽ nhảy số theo thời gian thực từ V-Box.

### 3.2. Triển Khai Miễn Phí Lên Vercel (Auto Deploy)
1. Đẩy dự án lên GitHub cá nhân (xem Mục 5).
2. Đăng nhập [vercel.com](https://vercel.com).
3. Bấm **Add New Project** &rarr; Chọn Repository GitHub vừa tạo.
4. Tại mục **Root Directory**, bấm *Edit* và chọn thư mục `web`.
5. Bấm **Deploy**. Sau 1-2 phút, bạn sẽ nhận được đường link web dạng `https://your-project.vercel.app` để truy cập từ bất cứ đâu trên thế giới!

---

## 4. Hướng Dẫn Cấu Hình Supabase Cloud

1. Đăng ký tài khoản miễn phí tại [supabase.com](https://supabase.com).
2. Tạo New Project mới (ví dụ đặt tên: `vbox-iot`).
3. Vào mục **SQL Editor** &rarr; Mở file `supabase/schema.sql` trong dự án này, copy toàn bộ nội dung và bấm **Run**.
4. Vào mục **Project Settings** &rarr; **API**:
   - Copy **Project URL** và **anon public key**.
   - Dán vào file `web/.env.local` (hoặc cấu hình tại Environment Variables trên Vercel):
     ```env
     NEXT_PUBLIC_SUPABASE_URL=https://xxxxxxxxxxxx.supabase.co
     NEXT_PUBLIC_SUPABASE_ANON_KEY=eyJh......
     ```
5. Hệ thống sẽ tự động lưu lại biểu đồ lịch sử nhiệt độ/độ ẩm 24h và đồng bộ lịch 24 khung giờ lên đám mây.

---

## 5. Hướng Dẫn Đẩy Mã Nguồn Lên GitHub

1. Tạo một repository mới trên [github.com](https://github.com) (ví dụ: `vbox-iot-system`).
2. Mở Terminal / PowerShell tại thư mục gốc `d:\iot_web_app`:
   ```bash
   cd d:\iot_web_app
   git init
   git add .
   git commit -m "Khoi tao he thong Next.js Web va Flutter Mobile thay the ESP32"
   git branch -M main
   git remote add origin https://github.com/<tai-khoan-cua-ban>/vbox-iot-system.git
   git push -u origin main
   ```

---

## 6. Hướng Dẫn Chạy & Build App Mobile (Flutter)

1. Mở Terminal tại thư mục `mobile`:
   ```bash
   cd d:\iot_web_app\mobile
   flutter pub get
   ```
2. Chạy thử trên máy ảo hoặc điện thoại Android/iOS kết nối USB:
   ```bash
   flutter run
   ```
3. Xuất file cài đặt APK cho điện thoại:
   ```bash
   flutter build apk --release
   ```
   File APK hoàn thiện sẽ nằm tại: `mobile/build/app/outputs/flutter-apk/app-release.apk`.
