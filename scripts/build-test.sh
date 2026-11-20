#!/bin/bash
set -ex

mkdir -p test

# Detect C++ compiler (fallback gracefully to g++ if clang++ is not present)
if [ -z "$CXX" ]; then
    if command -v clang++ >/dev/null 2>&1; then
        CXX="clang++"
    elif command -v g++ >/dev/null 2>&1; then
        CXX="g++"
    else
        CXX="c++"
    fi
fi

# Detect Operating System for platform-specific audio driver libraries
UNAME_S="$(uname -s)"
if [ "$UNAME_S" = "Linux" ]; then
    EXTRA_FLAGS="-lpthread -ldl -lm -lasound"
else
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

chmod +x test/bin
echo "Executing test binary..."
./test/bin
