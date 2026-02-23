import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

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
      home: const HomePage(), 
    );
  }
}

class HomePage extends StatefulWidget {
  const HomePage({super.key});

  @override
  State<HomePage> createState() => _HomePageState();
}

class _HomePageState extends State<HomePage> {
  // Канал для общения с C++
  static const MethodChannel _channel = MethodChannel('unity_channel');
  
  int? _textureId;

  @override
  void initState() {
    super.initState();
    _initUnityTexture();
  }

  Future<void> _initUnityTexture() async {
    try {
      // Запрашиваем ID текстуры у C++
      final int id = await _channel.invokeMethod('getTextureId');
      setState(() {
        _textureId = id;
      });
    } catch (e) {
      debugPrint("Ошибка получения Texture ID: $e");
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.white, // Фон может быть любым, он под текстурой
      body: Stack(
        children: [
          // 1. БАЗОВЫЙ СЛОЙ: Текстура Unity
          Positioned.fill(
            child: _textureId == null
                ? const Center(child: CircularProgressIndicator())
                : Texture(textureId: _textureId!), // Рисуем кадры из Unity!
          ),

          // 2. ДЕТЕКТОР ЖЕСТОВ поверх текстуры
          Positioned.fill(
            child: GestureDetector(
              behavior: HitTestBehavior.translucent, 
              onPanUpdate: (details) {
                // Сюда будут прилетать свайпы для вращения модели
                debugPrint("Свайп по сцене: ${details.delta}");
                // TODO: Передать координаты в Unity через MethodChannel
              },
              onTap: () {
                debugPrint("Клик по 3D зоне!");
              },
            ),
          ),

          // 3. ЛЕВАЯ ПАНЕЛЬ
          Positioned(
            left: 16,
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
                      onPressed: () => debugPrint('Нажата левая кнопка ${index + 1}'),
                      child: Text('Действие ${index + 1}'),
                    ),
                  ),
                )),
              ),
            ),
          ),

          // 4. ПРАВАЯ ПАНЕЛЬ
          Positioned(
            right: 16,
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
