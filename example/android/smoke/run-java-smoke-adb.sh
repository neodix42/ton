#!/bin/bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
ANDROID_DIR=$(cd "${SCRIPT_DIR}/.." && pwd)
TEST_DIR="${ANDROID_DIR}/test"

if [ $# -lt 1 ]; then
  echo "Usage: $0 <abi> [device_serial]" >&2
  echo "ABIs: armeabi-v7a | arm64-v8a | x86 | x86_64" >&2
  exit 1
fi

ABI="$1"
SERIAL="${2:-}"

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

function find_adb() {
  if command -v adb >/dev/null 2>&1; then
    command -v adb
    return 0
  fi

  local candidates=()
  if [ -n "${ANDROID_SDK_ROOT:-}" ]; then
    candidates+=("${ANDROID_SDK_ROOT}/platform-tools/adb")
  fi
  if [ -n "${ANDROID_HOME:-}" ]; then
    candidates+=("${ANDROID_HOME}/platform-tools/adb")
  fi
  candidates+=("${HOME}/Android/Sdk/platform-tools/adb")

  local candidate
  for candidate in "${candidates[@]}"; do
    if [ -x "${candidate}" ]; then
      echo "${candidate}"
      return 0
    fi
  done

  return 1
}

ADB_BIN=$(find_adb) || {
  echo "adb not found. Install Android SDK Platform-Tools and ensure adb is in PATH" >&2
  echo "or set ANDROID_SDK_ROOT/ANDROID_HOME to an SDK with platform-tools." >&2
  exit 1
}

LIB_DIR="${ANDROID_DIR}/libs/${ABI}"
if [ ! -f "${LIB_DIR}/libnative-lib.so" ] || [ ! -f "${LIB_DIR}/libtonlibjson.so" ]; then
  echo "Missing required libs in ${LIB_DIR}. Build Android libs first." >&2
  exit 1
fi

mkdir -p "${TEST_DIR}/ton/src/main/java"
cp -r "${ANDROID_DIR}/src/"* "${TEST_DIR}/ton/src/main/java/"

mkdir -p "${TEST_DIR}/ton/src/cpp/prebuilt"
rm -rf "${TEST_DIR}/ton/src/cpp/prebuilt/${ABI}"
cp -r "${LIB_DIR}" "${TEST_DIR}/ton/src/cpp/prebuilt/"

export PATH="$(dirname "${ADB_BIN}"):${PATH}"
if [ -n "${SERIAL}" ]; then
  export ANDROID_SERIAL="${SERIAL}"
fi

"${ADB_BIN}" start-server >/dev/null
"${ADB_BIN}" devices

(
  cd "${TEST_DIR}"
  ./gradlew :ton:connectedAndroidTest \
    -Pandroid.testInstrumentationRunnerArguments.class=drinkless.org.ton.TonSmokeLoadTest
)
