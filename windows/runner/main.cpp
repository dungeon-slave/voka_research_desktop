// #include <windows.h>
// #include <flutter/dart_project.h>
// #include <flutter/flutter_view_controller.h>
// #include <memory>
// #include <string>

// #pragma comment(lib, "user32.lib")

// // ============================
// // Layout constants
// // ============================

// const int kTopBarHeight = 60;
// const int kLeftPanelWidth = 200;
// const int kRightPanelWidth = 250;

// // ============================
// // Globals
// // ============================

// HWND g_main_hwnd = nullptr;
// HWND g_unity_hwnd = nullptr;

// std::unique_ptr<flutter::FlutterViewController> g_flutter_controller;

// // ============================
// // Resize Layout
// // ============================

// void UpdateLayout() {

//     if (!g_main_hwnd)
//         return;

//     RECT rect;
//     GetClientRect(g_main_hwnd, &rect);

//     int totalWidth = rect.right;
//     int totalHeight = rect.bottom;

//     int unityWidth = totalWidth - kLeftPanelWidth - kRightPanelWidth;
//     int unityHeight = totalHeight - kTopBarHeight;

//     if (unityWidth < 0) unityWidth = 0;
//     if (unityHeight < 0) unityHeight = 0;

//     // Flutter занимает всё окно
//     if (g_flutter_controller) {
//         HWND flutter_hwnd = g_flutter_controller->view()->GetNativeWindow();
//         MoveWindow(flutter_hwnd, 0, 0, totalWidth, totalHeight, TRUE);
//     }

//     // Unity в центре
//     if (g_unity_hwnd && IsWindow(g_unity_hwnd)) {
//         MoveWindow(
//             g_unity_hwnd,
//             kLeftPanelWidth,
//             kTopBarHeight,
//             unityWidth,
//             unityHeight,
//             TRUE
//         );
//     }
// }

// // ============================
// // Launch Unity
// // ============================

// bool LaunchUnity() {

//     wchar_t exePath[MAX_PATH];
//     GetModuleFileName(nullptr, exePath, MAX_PATH);

//     std::wstring path(exePath);
//     size_t pos = path.find_last_of(L"\\/");
//     std::wstring directory = path.substr(0, pos + 1);
//     std::wstring unityExe = directory + L"AnatomyPro.exe";

//     std::wstring commandLine =
//         L"\"" + unityExe + L"\""
//         L" -popupwindow"
//         L" -parentHWND " + std::to_wstring((uint64_t)g_main_hwnd)
//         + L" delayed";

//     STARTUPINFO si{};
//     si.cb = sizeof(si);

//     PROCESS_INFORMATION pi{};

//     if (!CreateProcess(
//         nullptr,
//         commandLine.data(),
//         nullptr,
//         nullptr,
//         FALSE,
//         0,
//         nullptr,
//         directory.c_str(),
//         &si,
//         &pi)) {

//         MessageBox(nullptr, L"Failed to launch Unity", L"Error", MB_OK);
//         return false;
//     }

//     CloseHandle(pi.hProcess);
//     CloseHandle(pi.hThread);

//     return true;
// }

// // ============================
// // Find Unity child window
// // ============================

// HWND FindUnityChild(HWND parent) {

//     HWND child = nullptr;

//     while ((child = FindWindowEx(parent, child, nullptr, nullptr)) != nullptr) {

//         wchar_t className[256];
//         GetClassName(child, className, 256);

//         if (wcsstr(className, L"Unity") != nullptr)
//             return child;
//     }

//     return nullptr;
// }

// // ============================
// // Window Procedure
// // ============================

// LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

//     switch (msg) {

//     case WM_SIZE:
//         UpdateLayout();
//         break;

//     case WM_CLOSE:
//         if (g_unity_hwnd && IsWindow(g_unity_hwnd))
//             PostMessage(g_unity_hwnd, WM_CLOSE, 0, 0);

//         DestroyWindow(hwnd);
//         break;

//     case WM_DESTROY:
//         PostQuitMessage(0);
//         break;
//     }

//     return DefWindowProc(hwnd, msg, wParam, lParam);
// }

// // ============================
// // Entry Point
// // ============================

// int APIENTRY wWinMain(HINSTANCE instance,
//                       HINSTANCE,
//                       wchar_t*,
//                       int show_command) {

//     ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

//     // Register main window class
//     WNDCLASS wc{};
//     wc.lpfnWndProc = MainWndProc;
//     wc.hInstance = instance;
//     wc.lpszClassName = L"MainWindowClass";
//     wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

//     RegisterClass(&wc);

//     g_main_hwnd = CreateWindowEx(
//         0,
//         L"MainWindowClass",
//         L"AnatomyPro",
//         WS_OVERLAPPEDWINDOW,
//         100, 100,
//         1280, 800,
//         nullptr,
//         nullptr,
//         instance,
//         nullptr
//     );

//     ShowWindow(g_main_hwnd, show_command);
//     UpdateWindow(g_main_hwnd);

//     // ============================
//     // Create Flutter as child
//     // ============================

//     flutter::DartProject project(L"data");

//     g_flutter_controller =
//         std::make_unique<flutter::FlutterViewController>(
//             1280,
//             800,
//             project
//         );

//     HWND flutter_hwnd =
//         g_flutter_controller->view()->GetNativeWindow();

//     SetParent(flutter_hwnd, g_main_hwnd);
//     ShowWindow(flutter_hwnd, SW_SHOW);

//     // ============================
//     // Launch Unity
//     // ============================

//     if (!LaunchUnity())
//         return 0;

//     // Wait for Unity window
//     for (int i = 0; i < 200; ++i) {

//         g_unity_hwnd = FindUnityChild(g_main_hwnd);

//         if (g_unity_hwnd)
//             break;

//         Sleep(50);
//     }

//     if (!g_unity_hwnd) {
//         MessageBox(nullptr, L"Unity window not found", L"Error", MB_OK);
//         return 0;
//     }

//     ShowWindow(g_unity_hwnd, SW_SHOW);

//     UpdateLayout();

//     // ============================
//     // Message Loop
//     // ============================

//     MSG msg;
//     while (GetMessage(&msg, nullptr, 0, 0)) {

//         TranslateMessage(&msg);
//         DispatchMessage(&msg);
//     }

//     ::CoUninitialize();
//     return EXIT_SUCCESS;
// }

#include <flutter/dart_project.h>
#include <flutter/flutter_view_controller.h>
#include <windows.h>
#include "flutter_window.h"
#include "win32_window.h"

int APIENTRY wWinMain(HINSTANCE instance,
                      HINSTANCE,
                      wchar_t*,
                      int show_command) {
  ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

  flutter::DartProject project(L"data");
  FlutterWindow window(project);

  // Стартовые размеры окна
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