import 'dart:convert';
import 'dart:math';
import 'package:flutter/foundation.dart';
import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';
import '../models/telemetry_model.dart';
import '../models/schedule_model.dart';

class MqttService {
  static const String server = 'broker.emqx.io';
  static const int port = 1883;

  static const String topicData = '3FAMIOT/HMI_PUB';
  static const String topicCmd = '3FAMIOT/HMI_SUB';
  static const String topicTime = '3FAMIOT/TIME';
  static const String topicAlarm = '3FAMIOT/ALARM';

  MqttServerClient? _client;
  bool isConnected = false;

  Function(TelemetryModel)? onTelemetryReceived;
  Function(String, bool)? onDeviceStatusReceived;
  Function(String alarmMsg, bool isOk)? onAlarmReceived;
  Function(String deviceKey, List<ScheduleSlotModel>)? onSchedulesReceived;
  Function(bool connected)? onConnectionChanged;

  final TelemetryModel telemetry = TelemetryModel();

  Future<void> connect() async {
    if (isConnected) return;

    final String clientId = 'Flutter_VBox_${Random().nextInt(99999)}';
    _client = MqttServerClient(server, clientId);
    _client!.port = port;
    _client!.keepAlivePeriod = 60;
    _client!.logging(on: false);
    _client!.autoReconnect = true;

    final connMessage = MqttConnectMessage()
        .withClientIdentifier(clientId)
        .startClean()
        .withWillQos(MqttQos.atMostOnce);
    _client!.connectionMessage = connMessage;

    try {
      debugPrint('[Flutter MQTT] Đang kết nối tới $server:$port...');
      await _client!.connect();
    } catch (e) {
      debugPrint('[Flutter MQTT] Lỗi kết nối: $e');
      _client?.disconnect();
      return;
    }

    if (_client!.connectionStatus!.state == MqttConnectionState.connected) {
      isConnected = true;
      telemetry.mqttConnected = true;
      onConnectionChanged?.call(true);
      debugPrint('[Flutter MQTT] KẾT NỐI THÀNH CÔNG!');

      // Subscribe topics
      _client!.subscribe(topicData, MqttQos.atMostOnce);
      _client!.subscribe(topicTime, MqttQos.atMostOnce);
      _client!.subscribe(topicAlarm, MqttQos.atMostOnce);

      _client!.updates!.listen(_onMessageReceived);
    } else {
      isConnected = false;
      telemetry.mqttConnected = false;
      onConnectionChanged?.call(false);
    }
  }

  void _onMessageReceived(List<MqttReceivedMessage<MqttMessage>> messages) {
    for (final message in messages) {
      final recMess = message.payload as MqttPublishMessage;
      final payload = MqttPublishPayload.bytesToStringAsString(recMess.payload.message);
      final topic = message.topic;

      try {
        final data = jsonDecode(payload) as Map<String, dynamic>;

        // 1. Topic Cảnh báo RS485
        if (topic == topicAlarm) {
          final int comm = data['plc_comm'] ?? 0;
          final bool isOk = (comm == 1 || data['status'] == 'CONNECTED');
          telemetry.plcCommOk = isOk;
          telemetry.plcAlarmMsg = data['message'] ?? (isOk ? '' : 'MẤT TRUYỀN THÔNG RS485 VỚI PLC!');
          onAlarmReceived?.call(telemetry.plcAlarmMsg, isOk);
          onTelemetryReceived?.call(telemetry);
        }

        // 2. Topic Thời gian RTC V-Box
        else if (topic == topicTime) {
          if (data['time'] != null) telemetry.vboxTime = data['time'];
          if (data['date'] != null) telemetry.vboxDate = data['date'];
          onTelemetryReceived?.call(telemetry);
        }

        // 3. Topic Dữ liệu cảm biến & 10 thiết bị
        else if (topic == topicData) {
          // Phản hồi danh sách lịch thực tế từ V-Box
          if (data['type'] == 'VBOX_SCHEDULES_RESP') {
            final dev = data['device']?.toString();
            final schedsRaw = data['schedules'] as List<dynamic>?;
            if (dev != null && schedsRaw != null) {
              final list = schedsRaw
                  .map((e) => ScheduleSlotModel.fromJson(e as Map<String, dynamic>))
                  .toList();
              onSchedulesReceived?.call(dev, list);
            }
            return;
          }

          // Cảm biến
          final temp = data['Temperature'] ?? data['Tempeturate'] ?? data['temp'];
          final hum = data['Humidity'] ?? data['hum'];
          if (temp != null) telemetry.temperature = (temp as num).toDouble();
          if (hum != null) telemetry.humidity = (hum as num).toDouble();

          // 2 Heartbeats
          final hb = data['heartbeat'] ?? data['kt_ket_noi'];
          if (hb != null) telemetry.vboxHeartbeat = (hb as num).toInt();
          final plcHb = data['plc_heartbeat'];
          if (plcHb != null) telemetry.plcHeartbeat = (plcHb as num).toInt();

          // Trạng thái RS485
          final comm = data['plc_comm'];
          if (comm != null) {
            telemetry.plcCommOk = (comm as num).toInt() == 1;
            telemetry.plcAlarmMsg = telemetry.plcCommOk ? '' : (data['alarm'] ?? 'MẤT TRUYỀN THÔNG RS485!');
          }

          // Ngưỡng & Tự động
          if (data['temp_threshold'] != null) telemetry.tempThreshold = (data['temp_threshold'] as num).toDouble();
          if (data['hum_threshold'] != null) telemetry.humThreshold = (data['hum_threshold'] as num).toDouble();
          if (data['ps_on_sec'] != null) telemetry.psOnSec = (data['ps_on_sec'] as num).toInt();
          if (data['ps_off_sec'] != null) telemetry.psOffSec = (data['ps_off_sec'] as num).toInt();
          if (data['ps_cycle_state'] != null) telemetry.psCycleState = (data['ps_cycle_state'] as num).toInt();
          if (data['auto_temp'] != null) telemetry.autoTempMode = (data['auto_temp'] as num).toInt() == 1;
          if (data['auto_humidity'] != null) telemetry.autoHumidityMode = (data['auto_humidity'] as num).toInt() == 1;

          // 10 thiết bị phản hồi
          final keys = [
            'phun_suong_1', 'phun_suong_2', 'phun_suong_3', 'phun_suong_4',
            'den_1', 'den_2', 'suoi_1', 'loa_1', 'loa_2', 'loa_3'
          ];
          for (final k in keys) {
            if (data.containsKey(k)) {
              final isRun = (data[k] as num).toInt() == 1;
              onDeviceStatusReceived?.call(k, isRun);
            }
          }

          onTelemetryReceived?.call(telemetry);
        }
      } catch (e) {
        debugPrint('[Flutter MQTT] Lỗi giải mã: $e');
      }
    }
  }

  void publish(Map<String, dynamic> data) {
    if (_client == null || !isConnected) return;
    final payload = jsonEncode(data);
    final builder = MqttClientPayloadBuilder();
    builder.addString(payload);
    _client!.publishMessage(topicCmd, MqttQos.atMostOnce, builder.payload!);
    debugPrint('[Flutter MQTT GỬI] $payload');
  }

  void setDeviceState(String key, bool state) {
    publish({key: state ? 1 : 0});
  }

  void setDeviceTimer(String key, int durationSec) {
    publish({'device': key, 'duration': durationSec, key: 1});
  }

  void setDeviceCycle(String key, int onSec, int offSec) {
    publish({'device': key, 'mode': 'cycle', 'on_time': onSec, 'off_time': offSec, key: 'cycle'});
  }

  void setDeviceSchedules(String key, List<ScheduleSlotModel> schedules) {
    publish({
      'device': key,
      'mode': 'schedule',
      'schedules': schedules.map((s) => s.toJson()).toList(),
    });
  }

  void querySchedules(String key) {
    publish({'cmd': 'get_schedules', 'device': key});
  }

  void updateAutoModes({
    bool? autoTemp,
    bool? autoHumidity,
    double? tempTh,
    double? humTh,
    int? onSec,
    int? offSec,
  }) {
    final map = <String, dynamic>{};
    if (autoTemp != null) map['auto_temp'] = autoTemp ? 1 : 0;
    if (autoHumidity != null) map['auto_humidity'] = autoHumidity ? 1 : 0;
    if (tempTh != null) map['temp_threshold'] = tempTh;
    if (humTh != null) map['hum_threshold'] = humTh;
    if (onSec != null) map['ps_on_sec'] = onSec;
    if (offSec != null) map['ps_off_sec'] = offSec;
    publish(map);
  }
}
