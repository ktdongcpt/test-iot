import 'package:flutter/material.dart';

class AlarmBannerWidget extends StatelessWidget {
  final bool plcCommOk;
  final String alarmMsg;
  final int plcHeartbeat;

  const AlarmBannerWidget({
    Key? key,
    required this.plcCommOk,
    required this.alarmMsg,
    required this.plcHeartbeat,
  }) : super(key: key);

  @override
  Widget build(BuildContext context) {
    if (plcCommOk) return const SizedBox.shrink();

    return Container(
      margin: const EdgeInsets.only(bottom: 16),
      padding: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: const Color(0xFFFEF2F2),
        border: Border.all(color: const Color(0xFFEF4444), width: 1.5),
        borderRadius: BorderRadius.circular(12),
      ),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          const Icon(Icons.warning_amber_rounded, color: Color(0xFFDC2626), size: 32),
          const SizedBox(width: 12),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                const Text(
                  'CẢNH BÁO MẤT RS485 VỚI PLC!',
                  style: TextStyle(
                    fontWeight: FontWeight.bold,
                    color: Color(0xFF991B1B),
                    fontSize: 13,
                  ),
                ),
                const SizedBox(height: 4),
                Text(
                  alarmMsg.isNotEmpty
                      ? alarmMsg
                      : 'Heartbeat PLC đứng yên > 15s. Kiểm tra ngay đường truyền RS485!',
                  style: const TextStyle(color: Color(0xFFB91C1C), fontSize: 12),
                ),
                const SizedBox(height: 4),
                Text(
                  'PLC Heartbeat: $plcHeartbeat/60 (@W_0#HDW13)',
                  style: const TextStyle(color: Color(0xFFDC2626), fontSize: 11, fontWeight: FontWeight.w600),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }
}
