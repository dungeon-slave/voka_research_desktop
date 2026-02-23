// import 'package:flutter/material.dart';
//
// void main() {
//   WidgetsFlutterBinding.ensureInitialized();
//   runApp(const AnatomyApp());
// }
//
// class AnatomyApp extends StatelessWidget {
//   const AnatomyApp({super.key});
//
//   @override
//   Widget build(BuildContext context) {
//     return const MaterialApp(
//       debugShowCheckedModeBanner: false,
//       home: HomePage(),
//     );
//   }
// }
//
// class HomePage extends StatelessWidget {
//   const HomePage({super.key});
//
//   @override
//   Widget build(BuildContext context) {
//     return Scaffold(
//       backgroundColor: Colors.white,
//       body: Column(
//         children: [
//           // Top bar
//           Container(
//             height: 60,
//             color: Colors.blueGrey,
//             alignment: Alignment.centerLeft,
//             padding: const EdgeInsets.symmetric(horizontal: 16),
//             child: const Text(
//               "Top Bar",
//               style: TextStyle(color: Colors.white, fontSize: 18),
//             ),
//           ),
//
//           Expanded(
//             child: Row(
//               children: [
//                 // Left panel
//                 Container(
//                   width: 200,
//                   color: Colors.grey.shade300,
//                   alignment: Alignment.center,
//                   child: const Text("Left Panel"),
//                 ),
//
//                 // Center placeholder (Unity area)
//                 Expanded(
//                   child: Container(
//                     color: Colors.transparent,
//                   ),
//                 ),
//
//                 // Right panel
//                 Container(
//                   width: 250,
//                   color: Colors.grey.shade200,
//                   child: Center(
//                     child: Column(
//                       mainAxisAlignment: MainAxisAlignment.center,
//                       children: [
//                         ElevatedButton(
//                           onPressed: (){},
//                           child: Text("Button"),
//                         ),
//                         SizedBox(height: 12),
//                         ElevatedButton(
//                           onPressed: (){},
//                           child: Text("Button"),
//                         ),
//                       ],
//                     ),
//                   ),
//                 ),
//               ],
//             ),
//           ),
//         ],
//       ),
//     );
//   }
// }

import 'package:flutter/material.dart';

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  runApp(const AnatomyApp());
}

class AnatomyApp extends StatelessWidget {
  const AnatomyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      color: Colors.transparent, // Прозрачность на уровне приложения
      theme: ThemeData(
        scaffoldBackgroundColor: Colors.transparent, // Прозрачность фона Scaffold
      ),
      home: const HomePage(),
    );
  }
}

class HomePage extends StatelessWidget {
  const HomePage({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.transparent, // Убрали глухой фон
      body: Stack(
        children: [
          // 1. БАЗОВЫЙ СЛОЙ: Детектор жестов (Прозрачное стекло над Unity)
          Positioned.fill(
            child: GestureDetector(
              behavior: HitTestBehavior.opaque, // Блокирует клики на рабочий стол
              onPanUpdate: (details) {
                // Сюда будут прилетать свайпы для вращения модели
                debugPrint("Свайп по сцене: ${details.delta}");
              },
              onTap: () {
                debugPrint("Клик по 3D зоне!");
              },
              child: Container(
                color: Colors.transparent,
              ),
            ),
          ),

          // 2. ЛЕВАЯ ПАНЕЛЬ (Полупрозрачная)
          Positioned(
            left: 16, // Отступ от края окна
            top: 16,
            bottom: 16,
            width: 200,
            child: Container(
              decoration: BoxDecoration(
                color: Colors.black54, // Полупрозрачный черный
                borderRadius: BorderRadius.circular(16),
              ),
              padding: const EdgeInsets.all(16.0),
              child: Column(
                mainAxisAlignment: MainAxisAlignment.center,
                children: List.generate(4, (index) => Padding(
                  padding: const EdgeInsets.symmetric(vertical: 8.0),
                  child: SizedBox(
                    width: double.infinity,
                    child: ElevatedButton(
                      onPressed: () => debugPrint('Нажата левая кнопка ${index + 1}'),
                      child: Text('Действие ${index + 1}'),
                    ),
                  ),
                )),
              ),
            ),
          ),

          // 3. ПРАВАЯ ПАНЕЛЬ (Полупрозрачная)
          Positioned(
            right: 16, // Отступ от края окна
            top: 16,
            bottom: 16,
            width: 200,
            child: Container(
              decoration: BoxDecoration(
                color: Colors.black54,
                borderRadius: BorderRadius.circular(16),
              ),
              padding: const EdgeInsets.all(16.0),
              child: Column(
                mainAxisAlignment: MainAxisAlignment.center,
                children: List.generate(4, (index) => Padding(
                  padding: const EdgeInsets.symmetric(vertical: 8.0),
                  child: SizedBox(
                    width: double.infinity,
                    child: ElevatedButton(
                      onPressed: () => debugPrint('Нажата правая кнопка ${index + 1}'),
                      child: Text('Меню ${index + 1}'),
                    ),
                  ),
                )),
              ),
            ),
          ),
        ],
      ),
    );
  }
}
