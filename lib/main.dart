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
  static const MethodChannel _channel = MethodChannel('unity_channel');

  int? _textureId;

  @override
  void initState() {
    super.initState();
    _initUnityTexture();
  }

  Future<void> _initUnityTexture() async {
    try {
      final int id = await _channel.invokeMethod('getTextureId');

      setState(() => _textureId = id);
    } catch (e) {
      debugPrint("Ошибка получения Texture ID: $e");
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.transparent,
      body: Stack(
        children: [
          Positioned.fill(
            child: _textureId == null
                ? const Center(child: CircularProgressIndicator())
                : Transform.scale(
                    scaleX: 1,
                    scaleY: -1,
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
                children: List.generate(
                  4,
                  (index) => Padding(
                    padding: const EdgeInsets.symmetric(vertical: 8.0),
                    child: SizedBox(
                      width: double.infinity,
                      child: ElevatedButton(
                        onPressed: () =>
                            debugPrint('Нажата левая кнопка ${index + 1}'),
                        child: Text('Действие ${index + 1}'),
                      ),
                    ),
                  ),
                ),
              ),
            ),
          ),
        ],
      ),
    );
  }
}
