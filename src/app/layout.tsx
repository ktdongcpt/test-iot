import type { Metadata } from 'next';
import './globals.css';

export const metadata: Metadata = {
  title: 'V-BOX IoT Cloud SCADA • Điều Khiển 10 Thiết Bị & Giám Sát PLC RS485',
  description: 'Hệ thống Web SCADA giám sát truyền thông RS485 PLC, cảm biến nhiệt độ - độ ẩm, và điều khiển 10 thiết bị V-Box theo thời gian thực.',
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
