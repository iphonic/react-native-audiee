Pod::Spec.new do |s|
  s.name         = "react-native-audiee"
  s.version      = "1.0.0"
  s.summary      = "High-performance C++ Audio Engine with Multi-Track Timeline & Real-Time DSP for React Native"
  s.homepage     = "https://github.com/audiee/react-native-audiee"
  s.license      = "MIT"
  s.authors      = { "Audiee Team" => "contact@audiee.app" }
  s.platforms    = { :ios => "13.4" }
  s.source       = { :git => "https://github.com/audiee/react-native-audiee.git", :tag => "#{s.version}" }

  s.source_files = "ios/**/*.{h,m,mm}", "cpp/**/*.{h,hpp,c,cpp}"
  s.frameworks   = "AVFoundation", "AudioToolbox", "CoreAudio"
  
  s.pod_target_xcconfig = {
    "CLANG_CXX_LANGUAGE_STANDARD" => "c++17",
    "HEADER_SEARCH_PATHS" => "\"$(PODS_TARGET_SRCROOT)/cpp\""
  }
end
