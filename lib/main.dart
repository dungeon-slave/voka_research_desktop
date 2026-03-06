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
      backgroundColor: Colors.transparent, // Фон может быть любым, он под текстурой
      body: Stack(
        children: [
          Positioned.fill(
            child: _textureId == null
                ? const Center(child: CircularProgressIndicator())
                : Transform.scale(
                    scaleX: 1, // Исправляет отзеркаливание по горизонтали
                    scaleY: -1, // Переворачивает картинку по вертикали
                    alignment: Alignment.center,
                    child: Texture(
                      textureId: _textureId!, 
                      filterQuality: FilterQuality.high,
                    ),
                  ),
          ),
          Positioned(
            left: 16,
            top: 6,
            width: 200,
            height: 500,
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
        ],
      ),
    );
  }
}
