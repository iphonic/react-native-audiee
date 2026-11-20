#!/bin/bash
set -e

mkdir -p test

# Detect C++ compiler
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
    EXTRA_FLAGS="-pthread -ldl -lm -lasound"
else
    EXTRA_FLAGS="-pthread -ldl -lm"
fi

echo "Compiling Audiee C++ Engine with $CXX on $UNAME_S..."

if ! $CXX -std=c++17 -O2 -Icpp \
    cpp/AudieeEngine.cpp \
    cpp/timeline/AudioTrack.cpp \
    cpp/timeline/MultiTrackMixer.cpp \
    test/engine_test.cpp \
    $EXTRA_FLAGS \
    -o test/bin 2> compile_error.txt; then
    echo "==================== COMPILATION ERROR ===================="
    cat compile_error.txt
    echo "==========================================================="
    if [ -n "$GITHUB_STEP_SUMMARY" ]; then
        echo "### ❌ Compilation Error on $UNAME_S" >> "$GITHUB_STEP_SUMMARY"
        echo "\`\`\`" >> "$GITHUB_STEP_SUMMARY"
        cat compile_error.txt >> "$GITHUB_STEP_SUMMARY"
        echo "\`\`\`" >> "$GITHUB_STEP_SUMMARY"
    fi
    exit 1
fi

chmod +x test/bin
echo "Executing test binary..."
./test/bin
