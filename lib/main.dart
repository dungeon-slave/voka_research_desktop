import 'package:flutter/material.dart';

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  runApp(const AnatomyApp());
}

class AnatomyApp extends StatelessWidget {
  const AnatomyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return const MaterialApp(
      debugShowCheckedModeBanner: false,
      home: HomePage(),
    );
  }
}

class HomePage extends StatelessWidget {
  const HomePage({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.white,
      body: Column(
        children: [
          // Top bar
          Container(
            height: 60,
            color: Colors.blueGrey,
            alignment: Alignment.centerLeft,
            padding: const EdgeInsets.symmetric(horizontal: 16),
            child: const Text(
              "Top Bar",
              style: TextStyle(color: Colors.white, fontSize: 18),
            ),
          ),

          Expanded(
            child: Row(
              children: [
                // Left panel
                Container(
                  width: 200,
                  color: Colors.grey.shade300,
                  alignment: Alignment.center,
                  child: const Text("Left Panel"),
                ),

                // Center placeholder (Unity area)
                Expanded(
                  child: Container(
                    color: Colors.transparent,
                  ),
                ),

                // Right panel
                Container(
                  width: 250,
                  color: Colors.grey.shade200,
                  child: Center(
                    child: Column(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        ElevatedButton(
                          onPressed: (){},
                          child: Text("Button"),
                        ),
                        SizedBox(height: 12),
                        ElevatedButton(
                          onPressed: (){},
                          child: Text("Button"),
                        ),
                      ],
                    ),
                  ),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }
}
