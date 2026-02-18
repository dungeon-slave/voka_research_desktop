// #include <flutter/dart_project.h>
// #include <flutter/flutter_view_controller.h>
// #include <windows.h>

// #include "flutter_window.h"
// #include "utils.h"

// int APIENTRY wWinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE prev,
//                       _In_ wchar_t *command_line, _In_ int show_command) {
//   // Attach to console when present (e.g., 'flutter run') or create a
//   // new console when running with a debugger.
//   if (!::AttachConsole(ATTACH_PARENT_PROCESS) && ::IsDebuggerPresent()) {
//     CreateAndAttachConsole();
//   }

//   // Initialize COM, so that it is available for use in the library and/or
//   // plugins.
//   ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

//   flutter::DartProject project(L"data");

//   std::vector<std::string> command_line_arguments =
//       GetCommandLineArguments();

//   project.set_dart_entrypoint_arguments(std::move(command_line_arguments));

//   FlutterWindow window(project);
//   Win32Window::Point origin(10, 10);
//   Win32Window::Size size(1280, 720);
//   if (!window.Create(L"voka_research_windows", origin, size)) {
//     return EXIT_FAILURE;
//   }
//   window.SetQuitOnClose(true);

//   // === Launch Unity Embedded ===

//   HWND flutter_hwnd = window.GetHandle();

//   // Загружаем UnityPlayer.dll
//   HMODULE unityModule = LoadLibrary(L"UnityPlayer.dll");
//   if (!unityModule) {
//     MessageBox(nullptr, L"Failed to load UnityPlayer.dll", L"Error", MB_OK);
//   } else {

//     typedef int(__stdcall* UnityMainFunc)(
//         HINSTANCE hInstance,
//         HINSTANCE hPrevInstance,
//         LPWSTR lpCmdLine,
//         int nShowCmd);

//     UnityMainFunc unityMain =
//         (UnityMainFunc)GetProcAddress(unityModule, "UnityMain");

//     if (!unityMain) {
//       MessageBox(nullptr, L"Failed to find UnityMain", L"Error", MB_OK);
//     } else {

//       wchar_t cmdLine[256];
//       swprintf_s(
//         cmdLine,
//         L"-parentHWND %llu -force-d3d11 -logFile unity.log",
//         (unsigned long long)flutter_hwnd
//       );

//       unityMain(
//         GetModuleHandle(nullptr),
//         nullptr,
//         cmdLine,
//         SW_SHOW
//       );
//     }
//   }

//   ::MSG msg;
//   while (::GetMessage(&msg, nullptr, 0, 0)) {
//     ::TranslateMessage(&msg);
//     ::DispatchMessage(&msg);
//   }

//   ::CoUninitialize();
//   return EXIT_SUCCESS;
// }


#include <flutter/dart_project.h>
#include <flutter/flutter_view_controller.h>
#include <windows.h>

#include "flutter_window.h"
#include "utils.h"

// ===== Global Unity HWND =====
HWND g_unity_hwnd = nullptr;

// ===== Find Unity child window =====
BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam)
{
  wchar_t class_name[256];
  GetClassName(hwnd, class_name, 256);

  if (wcscmp(class_name, L"UnityWndClass") == 0)
  {
    g_unity_hwnd = hwnd;
    return FALSE;
  }

  return TRUE;
}

int APIENTRY wWinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE prev,
                      _In_ wchar_t *command_line, _In_ int show_command)
{
  if (!::AttachConsole(ATTACH_PARENT_PROCESS) && ::IsDebuggerPresent()) {
    CreateAndAttachConsole();
  }

  ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

  flutter::DartProject project(L"data");

  std::vector<std::string> command_line_arguments =
      GetCommandLineArguments();

  project.set_dart_entrypoint_arguments(std::move(command_line_arguments));

  FlutterWindow window(project);
  Win32Window::Point origin(10, 10);
  Win32Window::Size size(1280, 720);

  if (!window.Create(L"voka_research_windows", origin, size)) {
    return EXIT_FAILURE;
  }

  window.SetQuitOnClose(true);

  // ===============================
  // Launch Unity via CreateProcess
  // ===============================

  HWND flutter_hwnd = window.GetHandle();

  wchar_t commandLine[512];
  swprintf_s(
      commandLine,
      L"UnityBuild\\voka_research_windows.exe -parentHWND %llu -force-d3d11 -logFile unity.log",
      (unsigned long long)flutter_hwnd
  );

  STARTUPINFO si = { sizeof(si) };
  PROCESS_INFORMATION pi;

  BOOL success = CreateProcess(
      nullptr,
      commandLine,
      nullptr,
      nullptr,
      FALSE,
      0,
      nullptr,
      nullptr,
      &si,
      &pi
  );

  if (success)
  {
    Sleep(1000);
    EnumChildWindows(flutter_hwnd, EnumChildProc, NULL);
  }
  else
  {
    MessageBox(nullptr, L"Failed to launch Unity process", L"Error", MB_OK);
  }

  // ===============================
  // Standard message loop
  // ===============================

  ::MSG msg;
  while (::GetMessage(&msg, nullptr, 0, 0))
  {
    ::TranslateMessage(&msg);
    ::DispatchMessage(&msg);
  }

  ::CoUninitialize();
  return EXIT_SUCCESS;
}
