import 'package:flutter/material.dart';
import '../models/device_model.dart';

class DeviceCardWidget extends StatelessWidget {
  final DeviceModel device;
  final bool isLocked;
  final Function(bool) onToggle;
  final VoidCallback onScheduleTap;
  final VoidCallback onTimerTap;

  const DeviceCardWidget({
    Key? key,
    required this.device,
    required this.isLocked,
    required this.onToggle,
    required this.onScheduleTap,
    required this.onTimerTap,
  }) : super(key: key);

  IconData _getIcon() {
    switch (device.category) {
      case 'mist':
        return Icons.water_drop_outlined;
      case 'light':
        return Icons.lightbulb_outline;
      case 'heater':
        return Icons.local_fire_department_outlined;
      case 'speaker':
        return Icons.volume_up_outlined;
      default:
        return Icons.devices;
    }
  }

  Color _getColor() {
    switch (device.category) {
      case 'mist':
        return Colors.blue;
      case 'light':
        return Colors.amber;
      case 'heater':
        return Colors.orange;
      case 'speaker':
        return Colors.purple;
      default:
        return Colors.blueGrey;
    }
  }

  @override
  Widget build(BuildContext context) {
    final active = device.isRunning;
    final themeColor = _getColor();

    return Container(
      padding: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: active ? themeColor.withOpacity(0.06) : Colors.white,
        borderRadius: BorderRadius.circular(16),
        border: Border.all(
          color: active ? themeColor.withOpacity(0.4) : const Color(0xFFE2E8F0),
          width: active ? 1.5 : 1,
        ),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              Container(
                padding: const EdgeInsets.all(8),
                decoration: BoxDecoration(
                  color: themeColor.withOpacity(0.12),
                  borderRadius: BorderRadius.circular(10),
                ),
                child: Icon(_getIcon(), color: themeColor, size: 22),
              ),
              Switch(
                value: active,
                onChanged: isLocked ? null : onToggle,
                activeColor: themeColor,
              ),
            ],
          ),
          const SizedBox(height: 8),
          Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text(
                device.name,
                style: const TextStyle(fontWeight: FontWeight.bold, fontSize: 13),
                maxLines: 1,
                overflow: TextOverflow.ellipsis,
              ),
              const SizedBox(height: 2),
              Text(
                '${device.cmdAddr} • ${device.statusAddr}',
                style: const TextStyle(color: Colors.grey, fontSize: 10, fontFamily: 'monospace'),
              ),
            ],
          ),
          const SizedBox(height: 8),
          Row(
            children: [
              Container(
                padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 2),
                decoration: BoxDecoration(
                  color: active ? Colors.green.shade50 : Colors.grey.shade100,
                  borderRadius: BorderRadius.circular(6),
                ),
                child: Text(
                  active ? 'ĐANG BẬT' : 'TẮT',
                  style: TextStyle(
                    fontSize: 10,
                    fontWeight: FontWeight.bold,
                    color: active ? Colors.green.shade700 : Colors.grey.shade600,
                  ),
                ),
              ),
              if (isLocked) ...[
                const SizedBox(width: 4),
                Container(
                  padding: const EdgeInsets.symmetric(horizontal: 6, vertical: 2),
                  decoration: BoxDecoration(
                    color: Colors.amber.shade50,
                    borderRadius: BorderRadius.circular(6),
                  ),
                  child: Row(
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      Icon(Icons.lock, size: 10, color: Colors.amber.shade800),
                      const SizedBox(width: 2),
                      Text('Tự động', style: TextStyle(fontSize: 9, fontWeight: FontWeight.bold, color: Colors.amber.shade800)),
                    ],
                  ),
                ),
              ]
            ],
          ),
          const SizedBox(height: 8),
          Row(
            children: [
              Expanded(
                child: OutlinedButton(
                  onPressed: onTimerTap,
                  style: OutlinedButton.styleFrom(
                    padding: const EdgeInsets.symmetric(vertical: 4),
                    minimumSize: const Size(0, 28),
                    textStyle: const TextStyle(fontSize: 11),
                  ),
                  child: const Text('Hẹn giờ'),
                ),
              ),
              const SizedBox(width: 6),
              Expanded(
                child: ElevatedButton(
                  onPressed: onScheduleTap,
                  style: ElevatedButton.styleFrom(
                    backgroundColor: Colors.blue.shade50,
                    foregroundColor: Colors.blue.shade800,
                    elevation: 0,
                    padding: const EdgeInsets.symmetric(vertical: 4),
                    minimumSize: const Size(0, 28),
                    textStyle: const TextStyle(fontSize: 11, fontWeight: FontWeight.bold),
                  ),
                  child: Text('Lịch (${device.maxSlots})'),
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }
}
