#!/bin/bash

pushd .

echo "[build.sh] script=$0"
echo "[build.sh] cwd=$(pwd)"

if [ -z "${ANDROID_NDK_ROOT}" ]; then
  echo "[build.sh] ANDROID_NDK_ROOT is not set" >&2
  exit 1
fi
if [ ! -f "${ANDROID_NDK_ROOT}/build/cmake/android.toolchain.cmake" ]; then
  echo "[build.sh] Android toolchain not found at ${ANDROID_NDK_ROOT}/build/cmake/android.toolchain.cmake" >&2
  exit 1
fi

echo "[build.sh] ARCH=${ARCH}"

if [ $ARCH == "arm" ]
then
  ABI="armeabi-v7a"
elif [ $ARCH == "x86" ]
then
  ABI=$ARCH
elif [ $ARCH == "x86_64" ]
then
  ABI=$ARCH
elif [ $ARCH == "arm64" ]
then
  ABI="arm64-v8a"
fi

ARCH=$ABI

function is_system_dependency() {
  case "$1" in
    libc.so|libdl.so|liblog.so|libm.so|libandroid.so|libz.so|libjnigraphics.so|libOpenSLES.so|libEGL.so|libGLESv1_CM.so|libGLESv2.so|libGLESv3.so|libvulkan.so|libmediandk.so)
      return 0
      ;;
  esac
  return 1
}

function find_dependency_file() {
  local dep_name="$1"
  local build_dir="$2"

  local dep_path
  dep_path=$(find "${build_dir}" -type f -name "${dep_name}" -print -quit)
  if [ -n "${dep_path}" ]; then
    echo "${dep_path}"
    return 0
  fi

  # Fallback for versioned outputs when linker expects an unversioned SONAME.
  if [ "${dep_name}" = "libtonlibjson.so" ]; then
    dep_path=$(find "${build_dir}" -type f -name "libtonlibjson.so*" -print -quit)
    if [ -n "${dep_path}" ]; then
      echo "${dep_path}"
      return 0
    fi
  fi

  if [ "${dep_name}" = "libc++_shared.so" ]; then
    local triple
    case "${ABI}" in
      armeabi-v7a) triple="arm-linux-androideabi" ;;
      arm64-v8a) triple="aarch64-linux-android" ;;
      x86) triple="i686-linux-android" ;;
      x86_64) triple="x86_64-linux-android" ;;
      *) triple="" ;;
    esac
    if [ -n "${triple}" ]; then
      dep_path="${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/${triple}/libc++_shared.so"
      if [ -f "${dep_path}" ]; then
        echo "${dep_path}"
        return 0
      fi
    fi
  fi

  return 1
}

function copy_runtime_dependencies() {
  local build_dir="$1"
  local output_dir="$2"

  local readelf_bin="${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-readelf"
  if [ ! -x "${readelf_bin}" ]; then
    readelf_bin="readelf"
  fi

  local processed_deps=" "
  local queue=("${build_dir}/libnative-lib.so")

  while [ ${#queue[@]} -gt 0 ]; do
    local target_file="${queue[0]}"
    queue=("${queue[@]:1}")

    local dep_name
    while IFS= read -r dep_name; do
      [ -z "${dep_name}" ] && continue
      is_system_dependency "${dep_name}" && continue
      case "${processed_deps}" in
        *" ${dep_name} "*) continue ;;
      esac

      local dep_path
      dep_path=$(find_dependency_file "${dep_name}" "${build_dir}") || {
        echo "[build.sh] Runtime dependency ${dep_name} not found for ${ABI}" >&2
        return 1
      }

      processed_deps="${processed_deps}${dep_name} "
      cp -L "${dep_path}" "${output_dir}/${dep_name}"
      queue+=("${dep_path}")
    done < <("${readelf_bin}" -d "${target_file}" | awk -F'[][]' '/NEEDED/ {print $2}')
  done
}

ANDROID_PLATFORM_LEVEL="android-32"
if [ "${ABI}" == "x86" ] || [ "${ABI}" == "x86_64" ]; then
  ANDROID_PLATFORM_LEVEL="android-30"
fi
if [ -z "${ANDROID_PLATFORM_LEVEL}" ]; then
  ANDROID_PLATFORM_LEVEL="android-21"
fi

echo "[build.sh] ABI=${ABI}"
echo "[build.sh] ANDROID_PLATFORM=${ANDROID_PLATFORM_LEVEL}"
echo "[build.sh] build dir=build-${ARCH}"

mkdir -p build-$ARCH
cd build-$ARCH

echo "[build.sh] configure (ARCH=${ARCH})"
cmake .. -GNinja \
-DTON_ONLY_TONLIB=ON  \
-DTON_ARCH="" \
-DANDROID_PLATFORM=${ANDROID_PLATFORM_LEVEL} \
-DANDROID_NDK=${ANDROID_NDK_ROOT} \
-DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK_ROOT}/build/cmake/android.toolchain.cmake  \
-DCMAKE_BUILD_TYPE=Release \
-DANDROID_ABI=${ABI} || exit 1

echo "[build.sh] build native-lib (ARCH=${ARCH})"
ninja native-lib || exit 1
popd

$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip build-$ARCH/libnative-lib.so

mkdir -p libs/$ARCH/
cp build-$ARCH/libnative-lib.so* libs/$ARCH/
copy_runtime_dependencies "build-$ARCH" "libs/$ARCH/" || exit 1
