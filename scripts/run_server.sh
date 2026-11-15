#!/usr/bin/env bash
set -e

PORT=${1:-5555}
echo "[MultiMousergy] Starting Server Host on port $PORT..."

if [ -f "./build/NetMux" ]; then
    ./build/NetMux --server --port "$PORT" --boundary-x 1919 --left
else
    echo "[Error] ./build/NetMux not found. Please run ./scripts/build.sh first."
    return 1 2>/dev/null || exit 1
fi
