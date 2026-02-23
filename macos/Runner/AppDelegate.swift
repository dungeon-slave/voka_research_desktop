import Cocoa
import FlutterMacOS
import ApplicationServices

class UnityWindowManager: NSObject, NSWindowDelegate {
    var unityPid: pid_t = 0
    var flutterWindow: NSWindow?
    var syncTimer: Timer?
    var unityAppElement: AXUIElement?
    var hasFoundUnityWindow = false
    
    func requestAccessibilityPermissions() -> Bool {
        let promptOption = kAXTrustedCheckOptionPrompt.takeUnretainedValue() as String
        let options = [promptOption: true] as CFDictionary
        let isTrusted = AXIsProcessTrustedWithOptions(options)
        return isTrusted
    }

    func startTracking(pid: pid_t) {
        self.unityPid = pid
        self.unityAppElement = AXUIElementCreateApplication(pid)
            
        if !requestAccessibilityPermissions() { return }
            
        print("✅ Права Accessibility получены!")
            
        DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
            if let window = NSApplication.shared.windows.first {
                self.flutterWindow = window
                window.delegate = self
                    
                // --- НАСТРОЙКА FLUTTER (SLAVE OVERLAY) ---
                // 1. Убираем рамки, кнопки закрытия и заголовок
                window.styleMask = [.borderless]
                
                // 2. Делаем прозрачным
                window.isOpaque = false
                window.backgroundColor = .clear
                window.hasShadow = false
                
                if let flutterVC = window.contentViewController as? FlutterViewController {
                    flutterVC.backgroundColor = .clear
                }
                
                // 3. Плавающий уровень, чтобы Flutter всегда был поверх Unity
                window.level = .floating
                // ---------------------------
                    
                self.syncTimer = Timer(timeInterval: 0.008, target: self, selector: #selector(self.syncWindows), userInfo: nil, repeats: true)
                RunLoop.main.add(self.syncTimer!, forMode: .common)
                    
                print("🔗 Синхронизация (Unity Master -> Flutter Slave) запущена!")
            }
        }
    }

    @objc func syncWindows() {
        guard let window = flutterWindow, let appElement = unityAppElement else { return }

        var value: CFTypeRef?
        let result = AXUIElementCopyAttributeValue(appElement, kAXWindowsAttribute as CFString, &value)
            
        if result == .success, let windows = value as? [AXUIElement], let unityWindowElement = windows.first {
            
            if !hasFoundUnityWindow {
                hasFoundUnityWindow = true
                print("🎯 Окно Unity поймано. Начинаем слежку.")
            }

            // Если пользователь кликнул на панель во Flutter, мы подтягиваем Unity за ним,
            // чтобы оно не провалилось под другие приложения (браузер и т.д.)
            if NSApp.isActive {
                AXUIElementPerformAction(unityWindowElement, kAXRaiseAction as CFString)
            }

            // --- ЧИТАЕМ КООРДИНАТЫ UNITY ---
            var positionRef: CFTypeRef?
            var sizeRef: CFTypeRef?
            
            if AXUIElementCopyAttributeValue(unityWindowElement, kAXPositionAttribute as CFString, &positionRef) == .success,
               AXUIElementCopyAttributeValue(unityWindowElement, kAXSizeAttribute as CFString, &sizeRef) == .success {
                
                if CFGetTypeID(positionRef as CFTypeRef) == AXValueGetTypeID() && CFGetTypeID(sizeRef as CFTypeRef) == AXValueGetTypeID() {
                    
                    let posValue = positionRef as! AXValue
                    let sizeValue = sizeRef as! AXValue
                    
                    var unityPos = CGPoint.zero
                    var unitySize = CGSize.zero
                    
                    AXValueGetValue(posValue, .cgPoint, &unityPos)
                    AXValueGetValue(sizeValue, .cgSize, &unitySize)
                    
                    guard let screen = window.screen else { return }
                    let screenHeight = screen.frame.height
                    
                    // Высота стандартной шапки macOS
                    let titleBarHeight: CGFloat = 30
                    
                    // --- ВЫЧИСЛЯЕМ ФРЕЙМ ДЛЯ FLUTTER ---
                    let flutterWidth = unitySize.width
                    let flutterHeight = unitySize.height - titleBarHeight
                    
                    if flutterWidth > 0 && flutterHeight > 0 {
                        // Конвертируем координаты (в AppKit Y начинается снизу)
                        let flutterY = screenHeight - (unityPos.y + unitySize.height)
                        let targetFrame = NSRect(x: unityPos.x, y: flutterY, width: flutterWidth, height: flutterHeight)
                        
                        // Меняем позицию Flutter только если она реально изменилась (чтобы не жрать CPU)
                        if window.frame != targetFrame {
                            window.setFrame(targetFrame, display: true, animate: false)
                        }
                    }
                }
            }
        }
    }
    
    func stop() {
        syncTimer?.invalidate()
    }
}

class UnityLauncher {
    private var process: Process?
    let windowManager = UnityWindowManager()

    func launch() {
        guard let resourcePath = Bundle.main.resourcePath else { return }

        let defaultsTask = Process()
        defaultsTask.executableURL = URL(fileURLWithPath: "/usr/bin/defaults")
        defaultsTask.arguments = ["delete", "com.Voka.AnatomyPro"]
        try? defaultsTask.run()
        defaultsTask.waitUntilExit()
            
        let unityExecutable = resourcePath + "/MacOS.app/Contents/MacOS/AnatomyPro"
            
        let proc = Process()
        proc.executableURL = URL(fileURLWithPath: unityExecutable)
            
        var env = ProcessInfo.processInfo.environment
        env["NSQuitAlwaysKeepsWindows"] = "NO"
        proc.environment = env
            
        proc.arguments = [
            "-screen-fullscreen", "0",
            "-window-mode", "windowed"
            // ВАЖНО: Убрали "-popupwindow", чтобы Unity запустился с родными рамками macOS
        ]

        do {
            try proc.run()
            process = proc
            print("✅ Unity started with PID:", proc.processIdentifier)
            windowManager.startTracking(pid: proc.processIdentifier)
        } catch {
            print("❌ Failed to start Unity:", error)
        }
    }

    func terminate() {
        windowManager.stop()
        if let proc = process, proc.isRunning {
            proc.terminate()
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
                if proc.isRunning {
                    kill(proc.processIdentifier, SIGKILL)
                }
            }
        }
    }
}

@main
class AppDelegate: FlutterAppDelegate {
    let unityLauncher = UnityLauncher()

    override func applicationDidFinishLaunching(_ notification: Notification) {
        unityLauncher.launch()
    }
    
    override func applicationWillTerminate(_ notification: Notification) {
        unityLauncher.terminate()
        Thread.sleep(forTimeInterval: 0.1)
    }
    
    override func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool {
        return true
    }
}
