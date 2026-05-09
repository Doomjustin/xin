#!/usr/bin/env bash
set -euo pipefail

cmake --build build --config Release --parallel "$(nproc)"
