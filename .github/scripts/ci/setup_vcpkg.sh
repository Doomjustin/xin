#!/usr/bin/env bash
set -euo pipefail

# Reuse restored cache when possible; only clone when vcpkg is absent.
if [[ ! -d vcpkg ]]; then
  git clone --depth 1 https://github.com/Microsoft/vcpkg.git
elif [[ ! -f vcpkg/bootstrap-vcpkg.sh ]]; then
  rm -rf vcpkg
  git clone --depth 1 https://github.com/Microsoft/vcpkg.git
fi

if [[ ! -x vcpkg/vcpkg ]]; then
  ./vcpkg/bootstrap-vcpkg.sh
fi

VCPKG_BASELINE="$(python3 - <<'PY'
import json
from pathlib import Path
p = Path('vcpkg.json')
if not p.exists():
    print('')
else:
    data = json.loads(p.read_text())
    print(data.get('builtin-baseline', ''))
PY
)"

if [[ -n "${VCPKG_BASELINE}" ]]; then
  if ! git -C vcpkg cat-file -e "${VCPKG_BASELINE}^{commit}" 2>/dev/null; then
    echo "Fetching missing vcpkg baseline commit: ${VCPKG_BASELINE}"
    git -C vcpkg fetch origin "${VCPKG_BASELINE}" --depth 1 || git -C vcpkg fetch origin "${VCPKG_BASELINE}"
  fi
fi

./vcpkg/vcpkg version
