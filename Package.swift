// swift-tools-version: 5.9
import PackageDescription

let package = Package(
    name: "react-native-audiee",
    platforms: [
        .iOS(.v14),
        .macOS(.v11)
    ],
    products: [
        .library(
            name: "react-native-audiee",
            targets: ["react-native-audiee"]
        )
    ],
    targets: [
        .target(
            name: "react-native-audiee",
            path: "cpp",
            sources: [
                "AudieeEngine.cpp",
                "timeline/AudioTrack.cpp",
                "timeline/MultiTrackMixer.cpp",
                "ai/StemSeparator.cpp"
            ],
            publicHeadersPath: ".",
            cxxSettings: [
                .headerSearchPath("."),
                .headerSearchPath("dsp"),
                .headerSearchPath("alignment"),
                .headerSearchPath("timeline"),
                .headerSearchPath("waveform"),
                .headerSearchPath("ai"),
                .unsafeFlags(["-std=c++17"])
            ],
            linkerSettings: [
                .linkedFramework("AVFoundation"),
                .linkedFramework("AudioToolbox"),
                .linkedFramework("CoreAudio")
            ]
        )
    ],
    cxxLanguageStandard: .cxx17
)
