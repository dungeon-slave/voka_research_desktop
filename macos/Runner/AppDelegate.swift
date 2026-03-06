import Cocoa
import FlutterMacOS

@main
class AppDelegate: FlutterAppDelegate {
    private var unityProcess: Process?
    
    override func applicationDidFinishLaunching(_ notification: Notification) {
        launchUnityBackground()
    }
    
    private func launchUnityBackground() {
        guard let resourcePath = Bundle.main.resourcePath else { return }
        let unityExecutable = resourcePath + "/MacOS.app/Contents/MacOS/AnatomyPro"
        
        let proc = Process()
        proc.executableURL = URL(fileURLWithPath: unityExecutable)
        proc.arguments = [
            "-batchmode",
            "-force-metal",
            "-nolog"
        ]
        
        do {
            proc.arguments?.append(contentsOf: ["-background"])
            try proc.run()
            self.unityProcess = proc
            print("✅ Unity запущен (PID: \(proc.processIdentifier))")
        } catch {
            print("❌ Ошибка запуска Unity: \(error)")
        }
    }
    
    override func applicationWillTerminate(_ notification: Notification) {
        if let proc = unityProcess, proc.isRunning {
            proc.terminate()
        }
    }
}
