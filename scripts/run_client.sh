#!/usr/bin/env bash
set -e

SERVER_IP=${1:-127.0.0.1}
PORT=${2:-5555}
echo "[MultiMousergy] Starting Client Peer connecting to $SERVER_IP:$PORT..."

if [ -f "./build/NetMux" ]; then
    ./build/NetMux --client "$SERVER_IP" --port "$PORT" --boundary-x 0 --right
else
    echo "[Error] ./build/NetMux not found. Please run ./scripts/build.sh first."
    return 1 2>/dev/null || exit 1
fi
