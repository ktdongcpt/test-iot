class TelemetryModel {
  double temperature;
  double humidity;
  int vboxHeartbeat;
  int plcHeartbeat;
  bool plcCommOk;
  String plcAlarmMsg;
  double tempThreshold;
  double humThreshold;
  int psOnSec;
  int psOffSec;
  int psCycleState;
  bool autoTempMode;
  bool autoHumidityMode;
  String vboxTime;
  String vboxDate;
  bool mqttConnected;

  TelemetryModel({
    this.temperature = 0.0,
    this.humidity = 0.0,
    this.vboxHeartbeat = 0,
    this.plcHeartbeat = 0,
    this.plcCommOk = true,
    this.plcAlarmMsg = '',
    this.tempThreshold = 25.0,
    this.humThreshold = 70.0,
    this.psOnSec = 120,
    this.psOffSec = 900,
    this.psCycleState = 0,
    this.autoTempMode = false,
    this.autoHumidityMode = false,
    this.vboxTime = '--:--:--',
    this.vboxDate = '--/--/----',
    this.mqttConnected = false,
  });
}
