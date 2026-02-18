import 'package:flutter/material.dart';

void main() {
  runApp(
    const MaterialApp(
      debugShowCheckedModeBanner: false,
      home: Scaffold(
        backgroundColor: Colors.black,
        body: Center(
          child: Text(
            "Flutter UI Layer",
            style: TextStyle(color: Colors.white),
          ),
        ),
      ),
    ),
  );
}
