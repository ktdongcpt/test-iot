import 'package:flutter/material.dart';
import '../models/device_model.dart';
import '../models/schedule_model.dart';
import '../providers/iot_provider.dart';

class ScheduleDialog extends StatefulWidget {
  final DeviceModel device;
  final IotProvider provider;

  const ScheduleDialog({
    Key? key,
    required this.device,
    required this.provider,
  }) : super(key: key);

  @override
  State<ScheduleDialog> createState() => _ScheduleDialogState();
}

class _ScheduleDialogState extends State<ScheduleDialog> {
  late List<ScheduleSlotModel> slots;

  @override
  void initState() {
    super.initState();
    final existing = widget.provider.deviceSchedules[widget.device.key];
    if (existing != null && existing.isNotEmpty) {
      slots = List.from(existing);
    } else {
      slots = [
        ScheduleSlotModel(start: '07:00', stop: '07:15'),
        ScheduleSlotModel(start: '11:30', stop: '11:45'),
        ScheduleSlotModel(start: '17:00', stop: '17:20'),
      ];
    }
    widget.provider.fetchSchedules(widget.device.key);
  }

  Future<void> _pickTime(int index, bool isStart) async {
    final curTimeStr = isStart ? slots[index].start : slots[index].stop;
    final parts = curTimeStr.split(':').map((e) => int.tryParse(e) ?? 0).toList();
    final initialTime = TimeOfDay(hour: parts[0], minute: parts.length > 1 ? parts[1] : 0);

    final picked = await showTimePicker(context: context, initialTime: initialTime);
    if (picked != null) {
      final formatted = '${picked.hour.toString().padLeft(2, '0')}:${picked.minute.toString().padLeft(2, '0')}';
      setState(() {
        if (isStart) {
          slots[index].start = formatted;
        } else {
          slots[index].stop = formatted;
        }
      });
    }
  }

  void _addSlot() {
    if (slots.length >= widget.device.maxSlots) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Thiết bị này tối đa ${widget.device.maxSlots} khung giờ')),
      );
      return;
    }
    setState(() {
      slots.add(ScheduleSlotModel(start: '08:00', stop: '08:15'));
    });
  }

  void _deploy() {
    widget.provider.deploySchedules(widget.device.key, slots);
    Navigator.of(context).pop();
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text('Đã nạp ${slots.length} khung giờ xuống V-Box & PLC!')),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Dialog(
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(20)),
      child: Container(
        padding: const EdgeInsets.all(20),
        constraints: const BoxConstraints(maxWidth: 450, maxHeight: 600),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        'Lịch Hẹn: ${widget.device.name}',
                        style: const TextStyle(fontWeight: FontWeight.bold, fontSize: 16),
                      ),
                      Text(
                        'Tối đa ${widget.device.maxSlots} khung giờ trong ngày',
                        style: const TextStyle(color: Colors.grey, fontSize: 11),
                      ),
                    ],
                  ),
                ),
                IconButton(
                  icon: const Icon(Icons.close),
                  onPressed: () => Navigator.of(context).pop(),
                ),
              ],
            ),
            const Divider(),
            Expanded(
              child: ListView.builder(
                itemCount: slots.length,
                itemBuilder: (context, index) {
                  final slot = slots[index];
                  return Container(
                    margin: const EdgeInsets.only(bottom: 8),
                    padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 8),
                    decoration: BoxDecoration(
                      color: const Color(0xFFF8FAFC),
                      borderRadius: BorderRadius.circular(12),
                      border: Border.all(color: const Color(0xFFE2E8F0)),
                    ),
                    child: Row(
                      children: [
                        CircleAvatar(
                          radius: 12,
                          backgroundColor: Colors.blue.shade100,
                          child: Text('${index + 1}', style: TextStyle(fontSize: 11, color: Colors.blue.shade900, fontWeight: FontWeight.bold)),
                        ),
                        const SizedBox(width: 8),
                        InkWell(
                          onTap: () => _pickTime(index, true),
                          child: Container(
                            padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
                            decoration: BoxDecoration(color: Colors.white, borderRadius: BorderRadius.circular(6), border: Border.all(color: Colors.grey.shade300)),
                            child: Text(slot.start, style: const TextStyle(fontWeight: FontWeight.bold, fontSize: 13)),
                          ),
                        ),
                        const Padding(
                          padding: EdgeInsets.symmetric(horizontal: 6),
                          child: Text('→', style: TextStyle(color: Colors.grey)),
                        ),
                        InkWell(
                          onTap: () => _pickTime(index, false),
                          child: Container(
                            padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
                            decoration: BoxDecoration(color: Colors.white, borderRadius: BorderRadius.circular(6), border: Border.all(color: Colors.grey.shade300)),
                            child: Text(slot.stop, style: const TextStyle(fontWeight: FontWeight.bold, fontSize: 13)),
                          ),
                        ),
                        const Spacer(),
                        Checkbox(
                          value: slot.enable,
                          onChanged: (val) {
                            setState(() {
                              slot.enable = val ?? true;
                            });
                          },
                        ),
                        IconButton(
                          icon: const Icon(Icons.delete_outline, color: Colors.red, size: 20),
                          onPressed: () {
                            setState(() {
                              slots.removeAt(index);
                            });
                          },
                        ),
                      ],
                    ),
                  );
                },
              ),
            ),
            const SizedBox(height: 8),
            if (slots.length < widget.device.maxSlots)
              OutlinedButton.icon(
                onPressed: _addSlot,
                icon: const Icon(Icons.add, size: 16),
                label: Text('Thêm Khung Giờ (${slots.length}/${widget.device.maxSlots})'),
                style: OutlinedButton.styleFrom(
                  minimumSize: const Size.fromHeight(40),
                ),
              ),
            const SizedBox(height: 12),
            Row(
              children: [
                Expanded(
                  child: ElevatedButton.icon(
                    onPressed: _deploy,
                    icon: const Icon(Icons.cloud_upload_outlined, size: 18),
                    label: const Text('Nạp Xuống V-Box'),
                    style: ElevatedButton.styleFrom(
                      backgroundColor: Colors.blue.shade600,
                      foregroundColor: Colors.white,
                      minimumSize: const Size.fromHeight(44),
                    ),
                  ),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }
}
