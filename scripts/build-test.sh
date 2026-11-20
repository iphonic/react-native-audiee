#!/bin/bash
set -e

mkdir -p test

CXX="${CXX:-clang++}"

# Detect Operating System for platform-specific audio driver libraries
UNAME_S="$(uname -s)"
if [ "$UNAME_S" = "Linux" ]; then
    # Linux requires dlopen, math, pthread, and alsa
    EXTRA_FLAGS="-lpthread -ldl -lm -lasound"
else
    # macOS includes CoreAudio and POSIX libs in libSystem
    EXTRA_FLAGS="-lpthread -ldl -lm"
fi

echo "Compiling Audiee C++ Engine with $CXX on $UNAME_S..."
$CXX -std=c++17 -O2 -Icpp \
    cpp/AudieeEngine.cpp \
    cpp/timeline/AudioTrack.cpp \
    cpp/timeline/MultiTrackMixer.cpp \
    test/engine_test.cpp \
    $EXTRA_FLAGS \
    -o test/bin

echo "Executing test binary..."
./test/bin
