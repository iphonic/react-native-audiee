cmake_minimum_required(VERSION 3.13)
project(react-native-audiee)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    ../cpp
    ../cpp/dsp
    ../cpp/alignment
    ../cpp/timeline
    ../cpp/waveform
)

add_library(
    audiee-engine
    SHARED
    ../cpp/AudieeEngine.cpp
    ../cpp/timeline/AudioTrack.cpp
    ../cpp/timeline/MultiTrackMixer.cpp
)

find_library(LOG_LIB log)
find_library(AAUDIO_LIB aaudio)
find_library(OPENSLES_LIB OpenSLES)

target_link_libraries(
    audiee-engine
    ${LOG_LIB}
    ${AAUDIO_LIB}
    ${OPENSLES_LIB}
)
