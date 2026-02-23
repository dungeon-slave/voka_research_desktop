#include "flutter_window.h"
#include <optional>
#include <string>

// Подключаем C-API Windows Embedder
#include <flutter_windows.h>

// Подключаем каналы
#include <flutter/generated_plugin_registrant.h>
#include <flutter/method_channel.h>
#include <flutter/standard_method_codec.h>

// ============================
// Глобальные переменные
// ============================
HANDLE g_hMapFile = nullptr;
LPVOID g_pBuffer = nullptr;
FlutterDesktopPixelBuffer g_pixel_buffer{};

int64_t g_unity_texture_id = -1;
HANDLE g_unity_process = nullptr;

// Ссылка на C-API регистратор текстур
FlutterDesktopTextureRegistrarRef g_c_texture_registrar = nullptr;

// ============================
// Shared Memory & Unity
// ============================
void InitSharedMemory() {
    if (g_pBuffer) return; 
    g_hMapFile = OpenFileMappingA(FILE_MAP_READ, FALSE, "FlutterUnitySharedMem");
    if (g_hMapFile) {
        g_pBuffer = MapViewOfFile(g_hMapFile, FILE_MAP_READ, 0, 0, 1280 * 800 * 4);
        if (g_pBuffer) {
            g_pixel_buffer.width = 1280;
            g_pixel_buffer.height = 800;
            g_pixel_buffer.buffer = static_cast<const uint8_t*>(g_pBuffer);
        }
    }
}

// C-Callback для текстуры, который Flutter будет вызывать при отрисовке кадра
const FlutterDesktopPixelBuffer* TextureCopyCallback(size_t width, size_t height, void* user_data) {
    InitSharedMemory();
    if (g_pBuffer) return &g_pixel_buffer;
    return nullptr;
}

bool LaunchUnity() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileName(nullptr, exePath, MAX_PATH);
    std::wstring path(exePath);
    size_t pos = path.find_last_of(L"\\/");
    std::wstring directory = path.substr(0, pos + 1);
    std::wstring unityExe = directory + L"AnatomyPro.exe";
    
    // Идеальный запуск: только -batchmode
    std::wstring commandLine = L"\"" + unityExe + L"\" -batchmode";

    STARTUPINFO si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    
    if (!CreateProcess(nullptr, commandLine.data(), nullptr, nullptr, FALSE, 0, nullptr, directory.c_str(), &si, &pi)) {
        return false;
    }
    
    g_unity_process = pi.hProcess; 
    CloseHandle(pi.hThread);
    return true;
}

// ============================
// FlutterWindow Implementation
// ============================
FlutterWindow::FlutterWindow(const flutter::DartProject& project) : project_(project) {}

FlutterWindow::~FlutterWindow() {}

bool FlutterWindow::OnCreate() {
  if (!Win32Window::OnCreate()) return false;

  RECT frame = GetClientArea();
  flutter_controller_ = std::make_unique<flutter::FlutterViewController>(
      frame.right - frame.left, frame.bottom - frame.top, project_);

  if (!flutter_controller_->engine() || !flutter_controller_->view()) return false;

  RegisterPlugins(flutter_controller_->engine());
  SetChildContent(flutter_controller_->view()->GetNativeWindow());

  // --- МАГИЯ C-API ---
  // 1. Получаем C-API Registrar напрямую 
  FlutterDesktopPluginRegistrarRef c_plugin_registrar = 
      flutter_controller_->engine()->GetRegistrarForPlugin("UnityTextureBridge");

  // 2. ИСПРАВЛЕНО: Правильное название функции из C-API
  g_c_texture_registrar = FlutterDesktopRegistrarGetTextureRegistrar(c_plugin_registrar);

  // 3. Настраиваем структуру текстуры
  FlutterDesktopTextureInfo texture_info = {};
  texture_info.type = kFlutterDesktopPixelBufferTexture;
  texture_info.pixel_buffer_config.callback = TextureCopyCallback;
  texture_info.pixel_buffer_config.user_data = nullptr;

  // 4. Регистрируем текстуру
  g_unity_texture_id = FlutterDesktopTextureRegistrarRegisterExternalTexture(
      g_c_texture_registrar, &texture_info);

  // --- METHOD CHANNEL (C++) ---
  auto messenger = flutter_controller_->engine()->messenger();
  auto channel = std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
      messenger, "unity_channel",
      &flutter::StandardMethodCodec::GetInstance());

  channel->SetMethodCallHandler(
      [](const flutter::MethodCall<flutter::EncodableValue>& call,
         std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
        if (call.method_name() == "getTextureId") {
          result->Success(flutter::EncodableValue(g_unity_texture_id));
        } else {
          result->NotImplemented();
        }
      });

  LaunchUnity();
  SetTimer(GetHandle(), 1, 16, nullptr);

  return true;
}

void FlutterWindow::OnDestroy() {
  // Завершаем фоновый процесс Unity
  if (g_unity_process) {
      TerminateProcess(g_unity_process, 0);
      CloseHandle(g_unity_process);
      g_unity_process = nullptr;
  }

  // Очистка памяти
  if (g_pBuffer) UnmapViewOfFile(g_pBuffer);
  if (g_hMapFile) CloseHandle(g_hMapFile);
  
  if (g_c_texture_registrar && g_unity_texture_id != -1) {
      FlutterDesktopTextureRegistrarUnregisterExternalTexture(
          g_c_texture_registrar, g_unity_texture_id, nullptr, nullptr);
  }

  if (flutter_controller_) flutter_controller_ = nullptr;

  Win32Window::OnDestroy();
}

LRESULT FlutterWindow::MessageHandler(HWND hwnd, UINT const message, WPARAM const wparam, LPARAM const lparam) noexcept {
  if (message == WM_TIMER && wparam == 1) {
      if (g_c_texture_registrar && g_unity_texture_id != -1) {
          FlutterDesktopTextureRegistrarMarkExternalTextureFrameAvailable(g_c_texture_registrar, g_unity_texture_id);
      }
  }

  if (flutter_controller_) {
    std::optional<LRESULT> result = flutter_controller_->HandleTopLevelWindowProc(hwnd, message, wparam, lparam);
    if (result) return *result;
  }

  return Win32Window::MessageHandler(hwnd, message, wparam, lparam);
}
