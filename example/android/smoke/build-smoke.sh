#!/bin/bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
ANDROID_DIR=$(cd "${SCRIPT_DIR}/.." && pwd)

if [ -z "${ANDROID_NDK_ROOT:-}" ]; then
  echo "ANDROID_NDK_ROOT is not set" >&2
  exit 1
fi

if [ ! -f "${ANDROID_NDK_ROOT}/build/cmake/android.toolchain.cmake" ]; then
  echo "Android toolchain not found at ${ANDROID_NDK_ROOT}/build/cmake/android.toolchain.cmake" >&2
  exit 1
fi

if [ $# -lt 1 ]; then
  echo "Usage: $0 <abi>" >&2
  echo "ABIs: armeabi-v7a | arm64-v8a | x86 | x86_64" >&2
  exit 1
fi

ABI="$1"
case "${ABI}" in
  arm) ABI="armeabi-v7a" ;;
  arm64) ABI="arm64-v8a" ;;
esac

case "${ABI}" in
  armeabi-v7a|arm64-v8a|x86|x86_64) ;;
  *)
    echo "Unsupported ABI: ${ABI}" >&2
    exit 1
    ;;
esac

ANDROID_PLATFORM_LEVEL="android-32"
if [ "${ABI}" = "x86" ] || [ "${ABI}" = "x86_64" ]; then
  ANDROID_PLATFORM_LEVEL="android-30"
fi

LIB_DIR="${ANDROID_DIR}/libs/${ABI}"
if [ ! -f "${LIB_DIR}/libnative-lib.so" ]; then
  echo "Missing ${LIB_DIR}/libnative-lib.so. Build Android libs first." >&2
  exit 1
fi
if [ ! -f "${LIB_DIR}/libtonlibjson.so" ]; then
  echo "Missing ${LIB_DIR}/libtonlibjson.so. Build Android libs first." >&2
  exit 1
fi

BUILD_DIR="${ANDROID_DIR}/build-smoke-${ABI}"
cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" -GNinja \
  -DCMAKE_BUILD_TYPE=Release \
  -DANDROID_PLATFORM="${ANDROID_PLATFORM_LEVEL}" \
  -DANDROID_NDK="${ANDROID_NDK_ROOT}" \
  -DCMAKE_TOOLCHAIN_FILE="${ANDROID_NDK_ROOT}/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI="${ABI}"

cmake --build "${BUILD_DIR}" --target tonlibjson_smoke

cp "${BUILD_DIR}/tonlibjson_smoke" "${LIB_DIR}/tonlibjson_smoke"
echo "Built ${LIB_DIR}/tonlibjson_smoke"
