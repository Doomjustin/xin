#!/usr/bin/env bash
set -euo pipefail

CC_PATH=/usr/lib/llvm-23/bin/clang
CXX_PATH=/usr/lib/llvm-23/bin/clang++
SCAN_DEPS_PATH=/usr/lib/llvm-23/bin/clang-scan-deps
CLANG_RESOURCE_DIR="$(${CXX_PATH} -print-resource-dir)"
STDLIB_MODULES_JSON_PATH="$(${CXX_PATH} -stdlib=libc++ -print-file-name=libc++.modules.json)"
CANDIDATE_LLVM_JSON=/usr/lib/llvm-23/share/libc++/v1/libc++.modules.json
CANDIDATE_LLVM_STD_COMPAT=/usr/lib/llvm-23/share/libc++/v1/std.compat.cppm

# Prefer the LLVM-shipped pair when available; some distro-provided
# libc++.modules.json under /lib points to a directory without std.compat.cppm.
if [[ -f "${CANDIDATE_LLVM_JSON}" && -f "${CANDIDATE_LLVM_STD_COMPAT}" ]]; then
  STDLIB_MODULES_JSON_PATH="${CANDIDATE_LLVM_JSON}"
  STDCOMPAT_PATH="${CANDIDATE_LLVM_STD_COMPAT}"
else
  # Fallback for environments where clang returns only the filename.
  if [[ "${STDLIB_MODULES_JSON_PATH}" == "libc++.modules.json" || -z "${STDLIB_MODULES_JSON_PATH}" ]]; then
    STDLIB_MODULES_JSON_PATH="$(find /usr/lib/llvm-23 -type f -name libc++.modules.json | head -n 1)"
  fi

  STDCOMPAT_PATH="$(dirname "${STDLIB_MODULES_JSON_PATH}")/std.compat.cppm"

  # Last-resort: find a modules.json that has a sibling std.compat.cppm.
  if [[ ! -f "${STDCOMPAT_PATH}" ]]; then
    MATCHED_JSON=""
    while IFS= read -r CANDIDATE_JSON; do
      CANDIDATE_DIR="$(dirname "${CANDIDATE_JSON}")"
      if [[ -f "${CANDIDATE_DIR}/std.compat.cppm" ]]; then
        MATCHED_JSON="${CANDIDATE_JSON}"
        break
      fi
    done < <(find /usr/lib/llvm-23 -type f -name libc++.modules.json)

    if [[ -n "${MATCHED_JSON}" ]]; then
      STDLIB_MODULES_JSON_PATH="${MATCHED_JSON}"
      STDCOMPAT_PATH="$(dirname "${MATCHED_JSON}")/std.compat.cppm"
    fi
  fi
fi

echo "Using C compiler: ${CC_PATH}"
echo "Using CXX compiler: ${CXX_PATH}"
echo "Using clang-scan-deps: ${SCAN_DEPS_PATH}"
echo "Using clang resource dir: ${CLANG_RESOURCE_DIR}"
echo "Using stdlib modules json: ${STDLIB_MODULES_JSON_PATH}"
echo "Expected std.compat.cppm: ${STDCOMPAT_PATH}"

if [[ ! -f "${STDLIB_MODULES_JSON_PATH}" ]]; then
  echo "libc++.modules.json not found"
  find /usr/lib/llvm-23 -type f -name libc++.modules.json || true
  exit 1
fi

if [[ ! -f "${STDCOMPAT_PATH}" ]]; then
  echo "std.compat.cppm not found"
  find /usr/lib/llvm-23 -type f -name std.compat.cppm || true
fi

# Build a deterministic patched modules.json while keeping std module sources
# in their original libc++ directory (so relative *.inc includes keep working).
PATCHED_MOD_DIR=/workspace/.ci-libcxx-modules
mkdir -p "${PATCHED_MOD_DIR}"

STD_CPPM_SRC="$(dirname "${STDLIB_MODULES_JSON_PATH}")/std.cppm"
if [[ ! -f "${STD_CPPM_SRC}" ]]; then
  STD_CPPM_SRC="$(find /usr/lib/llvm-23 -type f -name std.cppm | head -n 1)"
fi

STD_COMPAT_SRC="${STDCOMPAT_PATH}"
if [[ ! -f "${STD_COMPAT_SRC}" ]]; then
  STD_COMPAT_SRC="$(find /usr/lib/llvm-23 -type f -name std.compat.cppm | head -n 1)"
fi

MODULE_BASE_DIR="$(dirname "${STD_COMPAT_SRC}")"
if [[ ! -f "${STD_CPPM_SRC}" || ! -f "${STD_COMPAT_SRC}" || ! -f "${MODULE_BASE_DIR}/std/algorithm.inc" || ! -f "${MODULE_BASE_DIR}/std.compat/cassert.inc" ]]; then
  echo "Unable to locate std.cppm/std.compat.cppm sources"
  echo "std.cppm source: ${STD_CPPM_SRC}"
  echo "std.compat source: ${STD_COMPAT_SRC}"
  echo "module base dir: ${MODULE_BASE_DIR}"
  exit 1
fi

cp -f "${STDLIB_MODULES_JSON_PATH}" "${PATCHED_MOD_DIR}/libc++.modules.json"

# libc++.modules.json may contain relative paths like share/libc++/v1/std.compat.cppm
# and include dirs like share/libc++/v1. Rewrite these to absolute paths.
python3 - "${PATCHED_MOD_DIR}/libc++.modules.json" "${STD_CPPM_SRC}" "${STD_COMPAT_SRC}" "${MODULE_BASE_DIR}" <<'PY'
import json
import pathlib
import sys

json_path = pathlib.Path(sys.argv[1])
std_cppm = str(pathlib.Path(sys.argv[2]))
std_compat_cppm = str(pathlib.Path(sys.argv[3]))
module_base_dir = str(pathlib.Path(sys.argv[4]))

data = json.loads(json_path.read_text())

def rewrite(node):
  if isinstance(node, dict):
    return {k: rewrite(v) for k, v in node.items()}
  if isinstance(node, list):
    return [rewrite(v) for v in node]
  if isinstance(node, str):
    name = pathlib.Path(node).name
    if name == "std.cppm":
      return std_cppm
    if name == "std.compat.cppm":
      return std_compat_cppm
    if node.endswith("share/libc++/v1"):
      return module_base_dir
  return node

patched = rewrite(data)
json_path.write_text(json.dumps(patched, indent=2))
PY

STDLIB_MODULES_JSON_PATH="${PATCHED_MOD_DIR}/libc++.modules.json"
STDCOMPAT_PATH="${STD_COMPAT_SRC}"

echo "Patched stdlib modules json: ${STDLIB_MODULES_JSON_PATH}"
echo "Patched std.compat.cppm: ${STDCOMPAT_PATH}"
echo "Patched libc++ module base dir: ${MODULE_BASE_DIR}"

cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_STANDARD=23 \
  -DCMAKE_C_COMPILER="${CC_PATH}" \
  -DCMAKE_CXX_COMPILER="${CXX_PATH}" \
  -DCMAKE_CXX_COMPILER_CLANG_SCAN_DEPS="${SCAN_DEPS_PATH}" \
  -DCMAKE_CXX_COMPILER_CLANG_RESOURCE_DIR="${CLANG_RESOURCE_DIR}" \
  -DCMAKE_CXX_STDLIB_MODULES_JSON="${STDLIB_MODULES_JSON_PATH}" \
  -DCMAKE_TOOLCHAIN_FILE=/workspace/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DVCPKG_TARGET_TRIPLET=x64-linux-llvm-ci \
  -DVCPKG_OVERLAY_TRIPLETS=/workspace/.github/vcpkg-triplets \
  -GNinja
