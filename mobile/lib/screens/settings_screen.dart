import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/iot_provider.dart';

class SettingsScreen extends StatefulWidget {
  const SettingsScreen({Key? key}) : super(key: key);

  @override
  State<SettingsScreen> createState() => _SettingsScreenState();
}

class _SettingsScreenState extends State<SettingsScreen> {
  late double tempTh;
  late double humTh;
  late int onSec;
  late int offSec;

  @override
  void initState() {
    super.initState();
    final p = Provider.of<IotProvider>(context, listen: false);
    tempTh = p.telemetry.tempThreshold;
    humTh = p.telemetry.humThreshold;
    onSec = p.telemetry.psOnSec;
    offSec = p.telemetry.psOffSec;
  }

  @override
  Widget build(BuildContext context) {
    return Consumer<IotProvider>(
      builder: (context, provider, child) {
        final t = provider.telemetry;

        return Scaffold(
          appBar: AppBar(
            title: const Text('Cài Đặt Tự Động & Hệ Thống', style: TextStyle(fontWeight: FontWeight.bold, fontSize: 16)),
          ),
          body: ListView(
            padding: const EdgeInsets.all(16),
            children: [
              // Mục 1: Tự động độ ẩm 4 bơm
              Card(
                elevation: 0,
                shape: RoundedRectangleBorder(
                  borderRadius: BorderRadius.circular(16),
                  side: const BorderSide(color: Color(0xFFE2E8F0)),
                ),
                child: Padding(
                  padding: const EdgeInsets.all(16),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Row(
                        mainAxisAlignment: MainAxisAlignment.spaceBetween,
                        children: [
                          const Row(
                            children: [
                              Icon(Icons.water_drop, color: Colors.blue),
                              SizedBox(width: 8),
                              Text('4 Bơm Phun Sương (Độ Ẩm)', style: TextStyle(fontWeight: FontWeight.bold)),
                            ],
                          ),
                          Switch(
                            value: t.autoHumidityMode,
                            onChanged: (val) {
                              provider.updateAutoSettings(autoHumidity: val);
                            },
                          ),
                        ],
                      ),
                      const SizedBox(height: 8),
                      Text(
                        t.autoHumidityMode ? 'Chế độ: Tự động theo cảm biến' : 'Chế độ: Theo lịch hẹn RTC',
                        style: TextStyle(fontSize: 12, color: t.autoHumidityMode ? Colors.blue.shade700 : Colors.grey),
                      ),
                      const Divider(height: 24),
                      Row(
                        mainAxisAlignment: MainAxisAlignment.spaceBetween,
                        children: [
                          const Text('Bật phun khi Độ ẩm <'),
                          Row(
                            children: [
                              SizedBox(
                                width: 60,
                                child: TextField(
                                  keyboardType: TextInputType.number,
                                  textAlign: TextAlign.center,
                                  decoration: const InputDecoration(isDense: true, border: OutlineInputBorder()),
                                  controller: TextEditingController(text: humTh.toStringAsFixed(0)),
                                  onChanged: (v) => humTh = double.tryParse(v) ?? humTh,
                                ),
                              ),
                              const SizedBox(width: 4),
                              const Text('%', style: TextStyle(fontWeight: FontWeight.bold)),
                            ],
                          ),
                        ],
                      ),
                      const SizedBox(height: 12),
                      Row(
                        mainAxisAlignment: MainAxisAlignment.spaceBetween,
                        children: [
                          const Text('Thời gian phun mỗi lần:'),
                          Row(
                            children: [
                              SizedBox(
                                width: 70,
                                child: TextField(
                                  keyboardType: TextInputType.number,
                                  textAlign: TextAlign.center,
                                  decoration: const InputDecoration(isDense: true, border: OutlineInputBorder()),
                                  controller: TextEditingController(text: onSec.toString()),
                                  onChanged: (v) => onSec = int.tryParse(v) ?? onSec,
                                ),
                              ),
                              const SizedBox(width: 4),
                              const Text('giây', style: TextStyle(fontSize: 12)),
                            ],
                          ),
                        ],
                      ),
                      const SizedBox(height: 12),
                      Row(
                        mainAxisAlignment: MainAxisAlignment.spaceBetween,
                        children: [
                          const Text('Thời gian nghỉ lan tỏa:'),
                          Row(
                            children: [
                              SizedBox(
                                width: 70,
                                child: TextField(
                                  keyboardType: TextInputType.number,
                                  textAlign: TextAlign.center,
                                  decoration: const InputDecoration(isDense: true, border: OutlineInputBorder()),
                                  controller: TextEditingController(text: offSec.toString()),
                                  onChanged: (v) => offSec = int.tryParse(v) ?? offSec,
                                ),
                              ),
                              const SizedBox(width: 4),
                              const Text('giây', style: TextStyle(fontSize: 12)),
                            ],
                          ),
                        ],
                      ),
                      const SizedBox(height: 16),
                      ElevatedButton(
                        onPressed: () {
                          provider.updateAutoSettings(
                            humTh: humTh,
                            onSec: onSec,
                            offSec: offSec,
                          );
                          ScaffoldMessenger.of(context).showSnackBar(
                            const SnackBar(content: Text('Đã lưu cấu hình Phun sương tự động!')),
                          );
                        },
                        style: ElevatedButton.styleFrom(
                          backgroundColor: Colors.blue.shade600,
                          foregroundColor: Colors.white,
                          minimumSize: const Size.fromHeight(40),
                        ),
                        child: const Text('Lưu Cấu Hình Phun Sương'),
                      ),
                    ],
                  ),
                ),
              ),
              const SizedBox(height: 16),

              // Mục 2: Tự động nhiệt độ Sưởi 1
              Card(
                elevation: 0,
                shape: RoundedRectangleBorder(
                  borderRadius: BorderRadius.circular(16),
                  side: const BorderSide(color: Color(0xFFE2E8F0)),
                ),
                child: Padding(
                  padding: const EdgeInsets.all(16),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Row(
                        mainAxisAlignment: MainAxisAlignment.spaceBetween,
                        children: [
                          const Row(
                            children: [
                              Icon(Icons.local_fire_department, color: Colors.orange),
                              SizedBox(width: 8),
                              Text('Sưởi 1 (Nhiệt Độ)', style: TextStyle(fontWeight: FontWeight.bold)),
                            ],
                          ),
                          Switch(
                            value: t.autoTempMode,
                            onChanged: (val) {
                              provider.updateAutoSettings(autoTemp: val);
                            },
                          ),
                        ],
                      ),
                      const SizedBox(height: 8),
                      Text(
                        t.autoTempMode ? 'Chế độ: Tự động theo cảm biến' : 'Chế độ: Theo lịch hẹn RTC',
                        style: TextStyle(fontSize: 12, color: t.autoTempMode ? Colors.orange.shade700 : Colors.grey),
                      ),
                      const Divider(height: 24),
                      Row(
                        mainAxisAlignment: MainAxisAlignment.spaceBetween,
                        children: [
                          const Text('Bật Sưởi 1 khi Nhiệt độ <'),
                          Row(
                            children: [
                              SizedBox(
                                width: 60,
                                child: TextField(
                                  keyboardType: TextInputType.number,
                                  textAlign: TextAlign.center,
                                  decoration: const InputDecoration(isDense: true, border: OutlineInputBorder()),
                                  controller: TextEditingController(text: tempTh.toStringAsFixed(1)),
                                  onChanged: (v) => tempTh = double.tryParse(v) ?? tempTh,
                                ),
                              ),
                              const SizedBox(width: 4),
                              const Text('°C', style: TextStyle(fontWeight: FontWeight.bold)),
                            ],
                          ),
                        ],
                      ),
                      const SizedBox(height: 16),
                      ElevatedButton(
                        onPressed: () {
                          provider.updateAutoSettings(tempTh: tempTh);
                          ScaffoldMessenger.of(context).showSnackBar(
                            const SnackBar(content: Text('Đã lưu cấu hình Sưởi 1 tự động!')),
                          );
                        },
                        style: ElevatedButton.styleFrom(
                          backgroundColor: Colors.orange.shade700,
                          foregroundColor: Colors.white,
                          minimumSize: const Size.fromHeight(40),
                        ),
                        child: const Text('Lưu Cấu Hình Sưởi 1'),
                      ),
                    ],
                  ),
                ),
              ),
              const SizedBox(height: 16),

              // Mục 3: Thông tin Broker & MQTT
              Card(
                elevation: 0,
                color: const Color(0xFFF8FAFC),
                shape: RoundedRectangleBorder(
                  borderRadius: BorderRadius.circular(16),
                  side: const BorderSide(color: Color(0xFFE2E8F0)),
                ),
                child: Padding(
                  padding: const EdgeInsets.all(16),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      const Text('Thông Tin Truyền Thông MQTT', style: TextStyle(fontWeight: FontWeight.bold, fontSize: 13)),
                      const SizedBox(height: 8),
                      const Text('• Broker: broker.emqx.io (Port 1883)', style: TextStyle(fontSize: 12, color: Colors.blueGrey)),
                      const Text('• Topic nhận: 3FAMIOT/HMI_PUB, TIME, ALARM', style: TextStyle(fontSize: 12, color: Colors.blueGrey)),
                      const Text('• Topic gửi: 3FAMIOT/HMI_SUB', style: TextStyle(fontSize: 12, color: Colors.blueGrey)),
                      const Text('• Heartbeat timeout RS485: 15 giây', style: TextStyle(fontSize: 12, color: Colors.blueGrey)),
                    ],
                  ),
                ),
              ),
            ],
          ),
        );
      },
    );
  }
}
