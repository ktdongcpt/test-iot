import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/iot_provider.dart';
import '../widgets/alarm_banner.dart';
import '../widgets/gauge_widget.dart';
import '../widgets/device_card.dart';
import 'schedule_dialog.dart';

class DashboardScreen extends StatelessWidget {
  const DashboardScreen({Key? key}) : super(key: key);

  void _showTimerDialog(BuildContext context, IotProvider provider, String key, String name) {
    int duration = 30;
    showDialog(
      context: context,
      builder: (ctx) => AlertDialog(
        title: Text('Hẹn Giờ: $name', style: const TextStyle(fontSize: 16, fontWeight: FontWeight.bold)),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            const Text('Nhập số giây thiết bị bật trước khi tự tắt:'),
            const SizedBox(height: 12),
            TextField(
              keyboardType: TextInputType.number,
              decoration: const InputDecoration(border: OutlineInputBorder(), hintText: '30 giây'),
              onChanged: (v) => duration = int.tryParse(v) ?? 30,
            ),
          ],
        ),
        actions: [
          TextButton(onPressed: () => Navigator.pop(ctx), child: const Text('Hủy')),
          ElevatedButton(
            onPressed: () {
              provider.setTimer(key, duration);
              Navigator.pop(ctx);
            },
            child: const Text('Bật Hẹn Giờ'),
          ),
        ],
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Consumer<IotProvider>(
      builder: (context, provider, child) {
        final t = provider.telemetry;

        return Scaffold(
          appBar: AppBar(
            title: const Text('V-BOX & PLC SMART IOT', style: TextStyle(fontWeight: FontWeight.bold, fontSize: 16)),
            actions: [
              IconButton(
                icon: const Icon(Icons.refresh),
                onPressed: () => provider.reconnectMqtt(),
                tooltip: 'Kết nối lại MQTT',
              ),
            ],
            bottom: PreferredSize(
              preferredSize: const Size.fromHeight(36),
              child: Container(
                color: const Color(0xFFF1F5F9),
                padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 6),
                child: Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    Row(
                      children: [
                        Icon(
                          Icons.circle,
                          size: 10,
                          color: t.mqttConnected ? Colors.green : Colors.red,
                        ),
                        const SizedBox(width: 6),
                        Text(
                          t.mqttConnected ? 'MQTT OK' : 'MẤT KẾT NỐI',
                          style: TextStyle(fontSize: 11, fontWeight: FontWeight.bold, color: t.mqttConnected ? Colors.green.shade800 : Colors.red.shade800),
                        ),
                      ],
                    ),
                    Text(
                      'V-Box: ${t.vboxHeartbeat}/60 | PLC: ${t.plcHeartbeat}/60',
                      style: const TextStyle(fontSize: 11, color: Colors.blueGrey, fontWeight: FontWeight.w600),
                    ),
                    Text(
                      t.vboxTime,
                      style: const TextStyle(fontSize: 11, fontWeight: FontWeight.bold),
                    ),
                  ],
                ),
              ),
            ),
          ),
          body: RefreshIndicator(
            onRefresh: () async {
              provider.reconnectMqtt();
            },
            child: ListView(
              padding: const EdgeInsets.all(16),
              children: [
                // 1. Cảnh báo RS485
                AlarmBannerWidget(
                  plcCommOk: t.plcCommOk,
                  alarmMsg: t.plcAlarmMsg,
                  plcHeartbeat: t.plcHeartbeat,
                ),

                // 2. Môi trường nhiệt độ & độ ẩm
                Row(
                  children: [
                    Expanded(
                      child: EnvironmentalCard(
                        title: 'Nhiệt Độ',
                        register: '@W_0#HDW10',
                        value: t.temperature,
                        unit: '°C',
                        color: Colors.orange,
                        icon: Icons.thermostat,
                        subInfo: 'Ngưỡng bật: < ${t.tempThreshold}°C',
                      ),
                    ),
                    const SizedBox(width: 12),
                    Expanded(
                      child: EnvironmentalCard(
                        title: 'Độ Ẩm',
                        register: '@W_0#HDW11',
                        value: t.humidity,
                        unit: '%',
                        color: Colors.blue,
                        icon: Icons.water_drop,
                        subInfo: 'Ngưỡng bật: < ${t.humThreshold}%',
                      ),
                    ),
                  ],
                ),
                const SizedBox(height: 20),

                // 3. Tiêu đề 10 thiết bị
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    const Text(
                      '10 Thiết Bị (Lệnh HDX0.x • Phản hồi HDX1.x)',
                      style: TextStyle(fontWeight: FontWeight.bold, fontSize: 14),
                    ),
                    Text(
                      '${provider.devices.where((d) => d.isRunning).length}/10 Bật',
                      style: const TextStyle(fontSize: 12, color: Colors.blueGrey, fontWeight: FontWeight.bold),
                    ),
                  ],
                ),
                const SizedBox(height: 12),

                // 4. Lưới 10 thiết bị
                GridView.builder(
                  shrinkWrap: true,
                  physics: const NeverScrollableScrollPhysics(),
                  gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
                    crossAxisCount: 2,
                    childAspectRatio: 0.95,
                    crossAxisSpacing: 12,
                    mainAxisSpacing: 12,
                  ),
                  itemCount: provider.devices.length,
                  itemBuilder: (context, index) {
                    final dev = provider.devices[index];
                    return DeviceCardWidget(
                      device: dev,
                      isLocked: provider.isDeviceLocked(dev.key),
                      onToggle: (state) => provider.toggleDevice(dev.key, state),
                      onScheduleTap: () {
                        showDialog(
                          context: context,
                          builder: (ctx) => ScheduleDialog(device: dev, provider: provider),
                        );
                      },
                      onTimerTap: () => _showTimerDialog(context, provider, dev.key, dev.name),
                    );
                  },
                ),
              ],
            ),
          ),
        );
      },
    );
  }
}
