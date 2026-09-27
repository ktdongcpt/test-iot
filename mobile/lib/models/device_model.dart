class DeviceModel {
  final String key;
  final String name;
  final String category; // 'mist', 'light', 'heater', 'speaker'
  final String cmdAddr;
  final String statusAddr;
  final int maxSlots;
  bool isRunning;

  DeviceModel({
    required this.key,
    required this.name,
    required this.category,
    required this.cmdAddr,
    required this.statusAddr,
    required this.maxSlots,
    this.isRunning = false,
  });

  static List<DeviceModel> get initialDevices => [
        DeviceModel(key: 'phun_suong_1', name: 'Phun sương 1', category: 'mist', cmdAddr: '@B_0#HDX0.0', statusAddr: '@B_0#HDX1.0', maxSlots: 24),
        DeviceModel(key: 'phun_suong_2', name: 'Phun sương 2', category: 'mist', cmdAddr: '@B_0#HDX0.1', statusAddr: '@B_0#HDX1.1', maxSlots: 24),
        DeviceModel(key: 'phun_suong_3', name: 'Phun sương 3', category: 'mist', cmdAddr: '@B_0#HDX0.2', statusAddr: '@B_0#HDX1.2', maxSlots: 24),
        DeviceModel(key: 'phun_suong_4', name: 'Phun sương 4', category: 'mist', cmdAddr: '@B_0#HDX0.3', statusAddr: '@B_0#HDX1.3', maxSlots: 24),
        DeviceModel(key: 'den_1',        name: 'Đèn 1',        category: 'light', cmdAddr: '@B_0#HDX0.4', statusAddr: '@B_0#HDX1.4', maxSlots: 10),
        DeviceModel(key: 'den_2',        name: 'Đèn 2',        category: 'light', cmdAddr: '@B_0#HDX0.5', statusAddr: '@B_0#HDX1.5', maxSlots: 10),
        DeviceModel(key: 'suoi_1',       name: 'Sưởi 1',       category: 'heater', cmdAddr: '@B_0#HDX0.6', statusAddr: '@B_0#HDX1.6', maxSlots: 10),
        DeviceModel(key: 'loa_1',        name: 'Loa 1',        category: 'speaker', cmdAddr: '@B_0#HDX0.7', statusAddr: '@B_0#HDX1.7', maxSlots: 10),
        DeviceModel(key: 'loa_2',        name: 'Loa 2',        category: 'speaker', cmdAddr: '@B_0#HDX0.8', statusAddr: '@B_0#HDX1.8', maxSlots: 10),
        DeviceModel(key: 'loa_3',        name: 'Loa 3',        category: 'speaker', cmdAddr: '@B_0#HDX0.9', statusAddr: '@B_0#HDX1.9', maxSlots: 10),
      ];
}
