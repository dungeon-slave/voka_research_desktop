import Cocoa
import FlutterMacOS

class MainFlutterWindow: NSWindow {
    private var textureId: Int64 = -1
    private var textureSource: UnityTextureSource?
    private var timer: Timer?
    
    override func awakeFromNib() {
        super.awakeFromNib()
        
        let flutterVC = FlutterViewController()
        let windowFrame = self.frame
        
        self.contentViewController = flutterVC
        self.setFrame(windowFrame, display: true)
        
        RegisterGeneratedPlugins(registry: flutterVC)
        
        NSApp.activate(ignoringOtherApps: true)
        self.makeKeyAndOrderFront(nil)
        
        // TODO: remove this freaky shit
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.2) { [weak self] in
            guard let self = self else { return }
            
            let registrar = flutterVC.registrar(forPlugin: "UnityTextureBridge")
            
            self.setupUnityChannel(
                messenger: flutterVC.engine.binaryMessenger,
                registrar: registrar.textures
            )
            
            print("✅ UnityTextureBridge connected")
        }
    }
    
    private func setupUnityChannel(messenger: FlutterBinaryMessenger, registrar: FlutterTextureRegistry) {
        let channel = FlutterMethodChannel(name: "unity_channel", binaryMessenger: messenger)
        
        let source = UnityTextureSource()
        self.textureSource = source
        self.textureId = registrar.register(source)
        
        guard self.textureId != 0 else {
            print("❌ Texture registration failed — engine not ready")
            return
        }
        
        if self.textureId != -1 && self.textureId != 0 {
            print("✅ Текстура успешно зарегистрирована с ID: \(self.textureId)")
            channel.setMethodCallHandler { [weak self] (call, result) in
                guard let self = self else { return }
                if call.method == "getTextureId" {
                    result(self.textureId)
                } else {
                    result(FlutterMethodNotImplemented)
                }
            }
            
            // TODO: remove this freaky shit
            timer = Timer.scheduledTimer(withTimeInterval: 1.0/60.0, repeats: true) { [weak self, weak registrar] _ in
                guard let self = self, let registrar = registrar else { return }
                registrar.textureFrameAvailable(self.textureId)
            }
        } else {
            print("❌ Ошибка: Не удалось зарегистрировать FlutterTexture. Получен ID: \(self.textureId)")
        }
    }
    override func close() {
        timer?.invalidate()
        super.close()
    }
}
