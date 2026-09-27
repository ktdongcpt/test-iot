import 'package:flutter/material.dart';
import '../models/device_model.dart';
import '../models/telemetry_model.dart';
import '../models/schedule_model.dart';
import '../services/mqtt_service.dart';
import '../services/notification_service.dart';

class IotProvider extends ChangeNotifier {
  final MqttService _mqttService = MqttService();
  final NotificationService _notificationService = NotificationService();

  List<DeviceModel> devices = DeviceModel.initialDevices;
  TelemetryModel telemetry = TelemetryModel();

  Map<String, List<ScheduleSlotModel>> deviceSchedules = {};

  IotProvider() {
    _init();
  }

  void _init() {
    _notificationService.init();

    _mqttService.onTelemetryReceived = (t) {
      telemetry = t;
      notifyListeners();
    };

    _mqttService.onDeviceStatusReceived = (key, isRun) {
      final index = devices.indexWhere((d) => d.key == key);
      if (index != -1) {
        devices[index].isRunning = isRun;
        notifyListeners();
      }
    };

    _mqttService.onAlarmReceived = (msg, isOk) {
      if (!isOk) {
        _notificationService.showAlarmNotification(
          title: 'CẢNH BÁO MẤT RS485!',
          body: msg,
        );
      }
      notifyListeners();
    };

    _mqttService.onSchedulesReceived = (key, scheds) {
      deviceSchedules[key] = scheds;
      notifyListeners();
    };

    _mqttService.onConnectionChanged = (conn) {
      telemetry.mqttConnected = conn;
      notifyListeners();
    };

    _mqttService.connect();
  }

  void reconnectMqtt() {
    _mqttService.connect();
  }

  bool isDeviceLocked(String key) {
    if (['phun_suong_1', 'phun_suong_2', 'phun_suong_3', 'phun_suong_4'].contains(key)) {
      return telemetry.autoHumidityMode;
    }
    if (key === 'suoi_1') {
      return telemetry.autoTempMode;
    }
    return false;
  }

  void toggleDevice(String key, bool nextState) {
    if (isDeviceLocked(key)) return;
    _mqttService.setDeviceState(key, nextState);
    final index = devices.indexWhere((d) => d.key == key);
    if (index != -1) {
      devices[index].isRunning = nextState;
      notifyListeners();
    }
  }

  void setTimer(String key, int durationSec) {
    _mqttService.setDeviceTimer(key, durationSec);
  }

  void setCycle(String key, int onSec, int offSec) {
    _mqttService.setDeviceCycle(key, onSec, offSec);
  }

  void deploySchedules(String key, List<ScheduleSlotModel> slots) {
    deviceSchedules[key] = slots;
    _mqttService.setDeviceSchedules(key, slots);
    notifyListeners();
  }

  void fetchSchedules(String key) {
    _mqttService.querySchedules(key);
  }

  void updateAutoSettings({
    bool? autoTemp,
    bool? autoHumidity,
    double? tempTh,
    double? humTh,
    int? onSec,
    int? offSec,
  }) {
    _mqttService.updateAutoModes(
      autoTemp: autoTemp,
      autoHumidity: autoHumidity,
      tempTh: tempTh,
      humTh: humTh,
      onSec: onSec,
      offSec: offSec,
    );
  }
}
