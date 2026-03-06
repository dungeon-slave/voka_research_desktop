#include "flutter_window.h"
#include <optional>
#include <string>

// Подключаем C-API Windows Embedder
#include <flutter_windows.h>
#include <flutter/generated_plugin_registrant.h>
#include <flutter/method_channel.h>
#include <flutter/standard_method_codec.h>

#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <chrono>

// ============================
// Константы конфигурации
// ============================
constexpr size_t kRenderWidth = 1280;
constexpr size_t kRenderHeight = 800;
constexpr size_t kBytesPerPixel = 4;
constexpr size_t kBufferSize = kRenderWidth * kRenderHeight * kBytesPerPixel;

// ============================
// Глобальные переменные
// ============================
HANDLE g_hMapFile = nullptr;
LPVOID g_pBuffer = nullptr;
FlutterDesktopPixelBuffer g_pixel_buffer{};

int64_t g_unity_texture_id = -1;
HANDLE g_unity_process = nullptr;
HANDLE g_hFrameEvent = nullptr;

FlutterDesktopTextureRegistrarRef g_c_texture_registrar = nullptr;

std::vector<uint8_t> g_local_buffer;
std::mutex g_buffer_mutex;
std::atomic<bool> g_is_running{false};
std::thread g_render_thread;

// ============================
// Shared Memory & Unity
// ============================
void InitSharedMemory() {
    if (g_pBuffer) return; 
    
    g_hMapFile = OpenFileMappingA(FILE_MAP_READ, FALSE, "FlutterUnitySharedMem");
    if (g_hMapFile) {
        g_pBuffer = MapViewOfFile(g_hMapFile, FILE_MAP_READ, 0, 0, kBufferSize);
        if (g_pBuffer) {
            // Защищаем инициализацию, чтобы избежать гонки данных при старте
            std::lock_guard<std::mutex> lock(g_buffer_mutex);
            g_local_buffer.resize(kBufferSize);
            
            g_pixel_buffer.width = kRenderWidth;
            g_pixel_buffer.height = kRenderHeight;
            g_pixel_buffer.buffer = g_local_buffer.data();
        }
    }
}

void StartFrameListener() {
    g_is_running = true;
    g_render_thread = std::thread([]() {
        
        // 1. Ждем Event от Unity (без спама принтами)
        while (g_is_running && !g_hFrameEvent) {
            g_hFrameEvent = OpenEventA(EVENT_ALL_ACCESS, FALSE, "FlutterUnityFrameReady");
            if (!g_hFrameEvent) {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
        }

        // 2. Основной цикл рендеринга
        while (g_is_running) {
            if (!g_hFrameEvent) break;

            DWORD waitResult = WaitForSingleObject(g_hFrameEvent, 16); 
            
            if (waitResult == WAIT_OBJECT_0) {                
                if (!g_pBuffer) {
                    InitSharedMemory();
                }

                if (g_pBuffer) {
                    // ОПТИМИЗАЦИЯ: Сужаем область видимости мьютекса
                    {
                        std::lock_guard<std::mutex> lock(g_buffer_mutex);
                        memcpy(g_local_buffer.data(), g_pBuffer, kBufferSize);
                    } // <-- Мьютекс отпускается ЗДЕСЬ, это критично!
                    
                    // Уведомляем Flutter, не блокируя буфер
                    if (g_c_texture_registrar && g_unity_texture_id != -1) {
                        FlutterDesktopTextureRegistrarMarkExternalTextureFrameAvailable(g_c_texture_registrar, g_unity_texture_id);
                    }
                }
            }
        }
        
        if (g_hFrameEvent) {
            CloseHandle(g_hFrameEvent);
            g_hFrameEvent = nullptr;
        }
    });
}

// C-Callback для текстуры, который Flutter вызывает при рендере
const FlutterDesktopPixelBuffer* TextureCopyCallback(size_t width, size_t height, void* user_data) {
    std::lock_guard<std::mutex> lock(g_buffer_mutex);
    if (g_local_buffer.empty()) return nullptr;

    return &g_pixel_buffer;
}

bool LaunchUnity() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileName(nullptr, exePath, MAX_PATH);
    std::wstring path(exePath);
    size_t pos = path.find_last_of(L"\\/");
    std::wstring directory = path.substr(0, pos + 1);
    std::wstring unityExe = directory + L"AnatomyPro.exe";
    
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

  FlutterDesktopPluginRegistrarRef c_plugin_registrar = 
      flutter_controller_->engine()->GetRegistrarForPlugin("UnityTextureBridge");

  g_c_texture_registrar = FlutterDesktopRegistrarGetTextureRegistrar(c_plugin_registrar);

  FlutterDesktopTextureInfo texture_info = {};
  texture_info.type = kFlutterDesktopPixelBufferTexture;
  texture_info.pixel_buffer_config.callback = TextureCopyCallback;
  texture_info.pixel_buffer_config.user_data = nullptr;

  g_unity_texture_id = FlutterDesktopTextureRegistrarRegisterExternalTexture(
      g_c_texture_registrar, &texture_info);

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
  StartFrameListener();

  return true;
}

void FlutterWindow::OnDestroy() {
  g_is_running = false;
  if (g_render_thread.joinable()) {
      g_render_thread.join();
  }

  if (g_unity_process) {
      TerminateProcess(g_unity_process, 0);
      CloseHandle(g_unity_process);
      g_unity_process = nullptr;
  }

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
  if (flutter_controller_) {
    std::optional<LRESULT> result = flutter_controller_->HandleTopLevelWindowProc(hwnd, message, wparam, lparam);
    if (result) return *result;
  }

  return Win32Window::MessageHandler(hwnd, message, wparam, lparam);
}
