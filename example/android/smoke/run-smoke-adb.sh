#!/bin/bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
ANDROID_DIR=$(cd "${SCRIPT_DIR}/.." && pwd)

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

LIB_DIR="${ANDROID_DIR}/libs/${ABI}"
SMOKE_BIN="${LIB_DIR}/tonlibjson_smoke"
if [ ! -x "${SMOKE_BIN}" ]; then
  echo "Missing executable ${SMOKE_BIN}. Run build-smoke.sh first." >&2
  exit 1
fi

if [ ! -f "${LIB_DIR}/libnative-lib.so" ] || [ ! -f "${LIB_DIR}/libtonlibjson.so" ]; then
  echo "Missing required shared libraries in ${LIB_DIR}" >&2
  exit 1
fi

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

ADB=("${ADB_BIN}")
if [ -n "${SERIAL}" ]; then
  ADB+=( -s "${SERIAL}" )
fi

REMOTE_DIR="/data/local/tmp/tonlib-smoke"
"${ADB[@]}" start-server >/dev/null
"${ADB[@]}" shell "mkdir -p ${REMOTE_DIR}"
"${ADB[@]}" push "${SMOKE_BIN}" "${REMOTE_DIR}/tonlibjson_smoke" >/dev/null
"${ADB[@]}" push "${LIB_DIR}/libnative-lib.so" "${REMOTE_DIR}/libnative-lib.so" >/dev/null
"${ADB[@]}" push "${LIB_DIR}/libtonlibjson.so" "${REMOTE_DIR}/libtonlibjson.so" >/dev/null
"${ADB[@]}" shell "chmod 755 ${REMOTE_DIR}/tonlibjson_smoke && cd ${REMOTE_DIR} && LD_LIBRARY_PATH=. ./tonlibjson_smoke ."
