#include <flutter/dart_project.h>
#include <flutter/flutter_view_controller.h>
#include <windows.h>
#include "flutter_window.h"
#include "win32_window.h"

int APIENTRY wWinMain(
  HINSTANCE instance,
  HINSTANCE,
  wchar_t*,
  int show_command
) {
  ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

  flutter::DartProject project(L"data");
  FlutterWindow window(project);

  Win32Window::Point origin(100, 100);
  Win32Window::Size size(1280, 800);

  if (!window.Create(L"AnatomyPro", origin, size)) {
    return EXIT_FAILURE;
  }

  window.SetQuitOnClose(true);
  window.Show();

  MSG msg;
  while (GetMessage(&msg, nullptr, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  ::CoUninitialize();
  return EXIT_SUCCESS;
}
