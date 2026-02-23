// #include "flutter_window.h"

// #include <optional>

// #include "flutter/generated_plugin_registrant.h"

// FlutterWindow::FlutterWindow(const flutter::DartProject& project)
//     : project_(project) {}

// FlutterWindow::~FlutterWindow() {}

// bool FlutterWindow::OnCreate() {
//   if (!Win32Window::OnCreate()) {
//     return false;
//   }

//   RECT frame = GetClientArea();

//   flutter_controller_ = std::make_unique<flutter::FlutterViewController>(
//       frame.right - frame.left,
//       frame.bottom - frame.top,
//       project_);

//   if (!flutter_controller_->engine() ||
//       !flutter_controller_->view()) {
//     return false;
//   }

//   RegisterPlugins(flutter_controller_->engine());

//   SetChildContent(flutter_controller_->view()->GetNativeWindow());

//   return true;
// }

// void FlutterWindow::OnDestroy() {
//   if (flutter_controller_) {
//     flutter_controller_ = nullptr;
//   }

//   Win32Window::OnDestroy();
// }

// LRESULT FlutterWindow::MessageHandler(HWND hwnd,
//                                       UINT const message,
//                                       WPARAM const wparam,
//                                       LPARAM const lparam) noexcept {
//   if (flutter_controller_) {
//     std::optional<LRESULT> result =
//         flutter_controller_->HandleTopLevelWindowProc(
//             hwnd, message, wparam, lparam);
//     if (result) {
//       return *result;
//     }
//   }

//   return Win32Window::MessageHandler(hwnd, message, wparam, lparam);
// }

#include "flutter_window.h"

#include <optional>
#include <string>
#include <dwmapi.h>

#include "flutter/generated_plugin_registrant.h"

#pragma comment(lib, "dwmapi.lib")

// Экспортируем глобальные переменные для win32_window.cpp
HWND g_unity_hwnd = nullptr;
HWND g_flutter_child_hwnd = nullptr; // Переименовали для логики

// ============================
// Helpers
// ============================
bool LaunchUnity(HWND parent) {
    wchar_t exePath[MAX_PATH];
    GetModuleFileName(nullptr, exePath, MAX_PATH);

    std::wstring path(exePath);
    size_t pos = path.find_last_of(L"\\/");
    std::wstring directory = path.substr(0, pos + 1);
    std::wstring unityExe = directory + L"AnatomyPro.exe";

    std::wstring commandLine =
        L"\"" + unityExe + L"\""
        L" -popupwindow"
        L" -parentHWND " + std::to_wstring((uint64_t)parent)
        + L" delayed";

    STARTUPINFO si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    if (!CreateProcess(nullptr, commandLine.data(), nullptr, nullptr, FALSE, 0, nullptr, directory.c_str(), &si, &pi)) {
        MessageBox(nullptr, L"Failed to launch Unity", L"Error", MB_OK);
        return false;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
}

HWND FindUnityChild(HWND parent) {
    HWND child = nullptr;
    while ((child = FindWindowEx(parent, child, nullptr, nullptr)) != nullptr) {
        wchar_t className[256];
        GetClassName(child, className, 256);
        if (wcsstr(className, L"Unity") != nullptr)
            return child;
    }
    return nullptr;
}

// ============================
// FlutterWindow Implementation
// ============================

FlutterWindow::FlutterWindow(const flutter::DartProject& project)
    : project_(project) {}

FlutterWindow::~FlutterWindow() {}

bool FlutterWindow::OnCreate() {
  if (!Win32Window::OnCreate()) {
    return false;
  }

  RECT frame = GetClientArea();

  flutter_controller_ = std::make_unique<flutter::FlutterViewController>(
      frame.right - frame.left,
      frame.bottom - frame.top,
      project_);

  if (!flutter_controller_->engine() ||
      !flutter_controller_->view()) {
    return false;
  }

  RegisterPlugins(flutter_controller_->engine());

  g_flutter_child_hwnd = flutter_controller_->view()->GetNativeWindow();

  // 1. ВОЗВРАЩАЕМ Flutter в иерархию родительского окна (чинит клики и рендер)
  SetChildContent(g_flutter_child_hwnd);

  // 2. Добавляем флаг CLIPSIBLINGS, чтобы соседи (Unity) не затирали друг друга
  LONG style = GetWindowLong(g_flutter_child_hwnd, GWL_STYLE);
  SetWindowLong(g_flutter_child_hwnd, GWL_STYLE, style | WS_CLIPSIBLINGS);

  // 3. Делаем дочернее окно "слоистым" для прозрачности
  LONG exStyle = GetWindowLong(g_flutter_child_hwnd, GWL_EXSTYLE);
  SetWindowLong(g_flutter_child_hwnd, GWL_EXSTYLE, exStyle | WS_EX_LAYERED);

  // 4. КРИТИЧНО: Устанавливаем альфу на 255. Это заставит Windows отрендерить 
  // видимые пиксели Flutter, а пустые (Colors.transparent) сделать насквозь прозрачными.
  //SetLayeredWindowAttributes(g_flutter_child_hwnd, 0, 255, LWA_ALPHA);
  // 4. КРИТИЧНО: Используем Color Key для черного цвета.
  // Это вырежет весь черный фон (сделает его прозрачным) и 
  // заставит Windows пропускать клики сквозь эти зоны в Unity!
  SetLayeredWindowAttributes(g_flutter_child_hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);

  MARGINS margins = {-1, -1, -1, -1};
  DwmExtendFrameIntoClientArea(g_flutter_child_hwnd, &margins);

  // --- ЗАПУСК UNITY ---
  LaunchUnity(GetHandle());

  for (int i = 0; i < 200; ++i) {
      g_unity_hwnd = FindUnityChild(GetHandle());
      if (g_unity_hwnd) break;
      Sleep(50);
  }

  if (g_unity_hwnd) {
      LONG unityStyle = GetWindowLong(g_unity_hwnd, GWL_STYLE);
      SetWindowLong(g_unity_hwnd, GWL_STYLE, unityStyle | WS_CLIPSIBLINGS);
      ShowWindow(g_unity_hwnd, SW_SHOW);
  }

  PostMessage(GetHandle(), WM_SIZE, 0, 0);

  return true;
}

void FlutterWindow::OnDestroy() {
  if (g_unity_hwnd && IsWindow(g_unity_hwnd)) {
      PostMessage(g_unity_hwnd, WM_CLOSE, 0, 0);
  }

  if (flutter_controller_) {
    flutter_controller_ = nullptr;
  }

  Win32Window::OnDestroy();
}

LRESULT FlutterWindow::MessageHandler(HWND hwnd,
                                      UINT const message,
                                      WPARAM const wparam,
                                      LPARAM const lparam) noexcept {
  if (flutter_controller_) {
    std::optional<LRESULT> result =
        flutter_controller_->HandleTopLevelWindowProc(
            hwnd, message, wparam, lparam);
    if (result) {
      return *result;
    }
  }

  return Win32Window::MessageHandler(hwnd, message, wparam, lparam);
}