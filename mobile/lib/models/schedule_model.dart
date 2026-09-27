class ScheduleSlotModel {
  String start; // "07:10"
  String stop;  // "07:20"
  bool enable;
  int day;
  int month;

  ScheduleSlotModel({
    required this.start,
    required this.stop,
    this.enable = true,
    this.day = 0,
    this.month = 0,
  });

  Map<String, dynamic> toJson() => {
        'start': start,
        'stop': stop,
        'enable': enable ? 1 : 0,
        'day': day,
        'month': month,
      };

  factory ScheduleSlotModel.fromJson(Map<String, dynamic> json) {
    return ScheduleSlotModel(
      start: json['start'] ?? '08:00',
      stop: json['stop'] ?? '08:15',
      enable: json['enable'] == null ? true : (json['enable'] == 1 || json['enable'] == true),
      day: json['day'] ?? 0,
      month: json['month'] ?? 0,
    );
  }
}
