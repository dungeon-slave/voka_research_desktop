import Foundation
import FlutterMacOS
import CoreVideo

@_silgen_name("shm_open")
func c_shm_open(_ name: UnsafePointer<CChar>, _ oflag: Int32, _ mode: Int32) -> Int32

@_silgen_name("ftruncate")
func c_ftruncate(_ fd: Int32, _ length: off_t) -> Int32

@_silgen_name("shm_unlink")
func c_shm_unlink(_ name: UnsafePointer<CChar>) -> Int32

class UnityTextureSource: NSObject, FlutterTexture {
    private var shmFd: Int32 = -1
    private var bufferPtr: UnsafeMutableRawPointer?
    private let width: Int = 1280
    private let height: Int = 800
    private let bufferSize: Int
    private let shmName = "/FlutterUnitySharedMem"
    
    private var frameCount = 0

    override init() {
        self.bufferSize = width * height * 4
        super.init()
        
        _ = c_shm_unlink(shmName)
        let O_CREAT_RDWR: Int32 = 0x0202
        shmFd = c_shm_open(shmName, O_CREAT_RDWR, 0o666)
        
        if shmFd != -1 {
            _ = c_ftruncate(shmFd, off_t(bufferSize))
            bufferPtr = mmap(nil, bufferSize, PROT_READ | PROT_WRITE, MAP_SHARED, shmFd, 0)
            
            if bufferPtr != MAP_FAILED {
                print("✅ Swift: Shared Memory создана!")
                
                // TODO: remove this freaky shit
                if let ptr = bufferPtr {
                    let rawBuffer = UnsafeMutableRawBufferPointer(start: ptr, count: bufferSize)
                    for i in stride(from: 0, to: bufferSize, by: 4) {
                        rawBuffer[i] = 0       // B
                        rawBuffer[i+1] = 0     // G
                        rawBuffer[i+2] = 255   // R
                        rawBuffer[i+3] = 255   // A
                    }
                }
            } else {
                bufferPtr = nil
            }
        }
    }

    func copyPixelBuffer() -> Unmanaged<CVPixelBuffer>? {
        guard let sourceData = bufferPtr else { return nil }

        // TODO: remove this freaky shit
        frameCount += 1
        if frameCount % 60 == 0 {
            let firstPixel = sourceData.load(as: UInt32.self)
            print("🪲 [Диагностика] Flutter забрал кадр. Первые байты: \(firstPixel)")
        }

        var pixelBuffer: CVPixelBuffer?
        let attrs: [CFString: Any] = [
            kCVPixelBufferMetalCompatibilityKey: true,
            kCVPixelBufferCGImageCompatibilityKey: true,
            kCVPixelBufferCGBitmapContextCompatibilityKey: true,
        ]

        let status = CVPixelBufferCreate(
            kCFAllocatorDefault, width, height, kCVPixelFormatType_32BGRA, attrs as CFDictionary, &pixelBuffer
        )

        guard status == kCVReturnSuccess, let pb = pixelBuffer else { return nil }

        CVPixelBufferLockBaseAddress(pb, [])
        if let dstBase = CVPixelBufferGetBaseAddress(pb) {
            let dstBytesPerRow = CVPixelBufferGetBytesPerRow(pb)
            let srcBytesPerRow = width * 4

            for y in 0..<height {
                memcpy(dstBase.advanced(by: y * dstBytesPerRow), sourceData.advanced(by: y * srcBytesPerRow), srcBytesPerRow)
            }
        }
        CVPixelBufferUnlockBaseAddress(pb, [])

        return Unmanaged.passRetained(pb)
    }

    deinit {
        if let ptr = bufferPtr { munmap(ptr, bufferSize) }
        if shmFd != -1 {
            close(shmFd)
            _ = c_shm_unlink(shmName)
        }
    }
}
