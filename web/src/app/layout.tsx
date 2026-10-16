import type { Metadata } from 'next';
import './globals.css';

export const metadata: Metadata = {
  title: 'V-BOX IoT Cloud SCADA • Điều Khiển 12 Thiết Bị & Giám Sát PLC RS485',
  description: 'Hệ thống Web SCADA giám sát truyền thông RS485 PLC, cảm biến nhiệt độ - độ ẩm, mực nước, âm thanh, dòng điện và điều khiển 12 thiết bị V-Box theo thời gian thực.',
};

export default function RootLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  return (
    <html lang="vi">
      <body className="min-h-screen bg-slate-50 text-slate-900 antialiased">
        {children}
      </body>
    </html>
  );
}
