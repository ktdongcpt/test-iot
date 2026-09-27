import 'package:flutter/foundation.dart';
import 'package:flutter_local_notifications/flutter_local_notifications.dart';

class NotificationService {
  static final NotificationService _instance = NotificationService._internal();
  factory NotificationService() => _instance;
  NotificationService._internal();

  final FlutterLocalNotificationsPlugin _notificationsPlugin = FlutterLocalNotificationsPlugin();

  Future<void> init() async {
    const AndroidInitializationSettings initializationSettingsAndroid =
        AndroidInitializationSettings('@mipmap/ic_launcher');

    const InitializationSettings initializationSettings = InitializationSettings(
      android: initializationSettingsAndroid,
    );

    try {
      await _notificationsPlugin.initialize(initializationSettings);
    } catch (e) {
      debugPrint('[NotificationService] Lỗi khởi tạo: $e');
    }
  }

  Future<void> showAlarmNotification({required String title, required String body}) async {
    const AndroidNotificationDetails androidDetails = AndroidNotificationDetails(
      'vbox_alarm_channel',
      'V-BOX Alarms',
      channelDescription: 'Cảnh báo mất truyền thông RS485 và sự cố',
      importance: Importance.max,
      priority: Priority.high,
      ticker: 'ticker',
    );

    const NotificationDetails platformDetails = NotificationDetails(android: androidDetails);

    try {
      await _notificationsPlugin.show(
        1001,
        title,
        body,
        platformDetails,
      );
    } catch (e) {
      debugPrint('[NotificationService] Lỗi hiển thị: $e');
    }
  }
}
