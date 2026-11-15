#!/usr/bin/env bash
set -e

echo "=============================================="
echo "[MultiMousergy] Building Backend & UI Client"
echo "=============================================="

mkdir -p build
cd build
cmake ..
make -j"$(nproc)"
cd ..

if [ -f "ui/package.json" ]; then
    echo "[Info] Setting up UI frontend dependencies..."
    cd ui && npm install && cd ..
fi

echo "=============================================="
echo "[MultiMousergy] Build succeeded!"
echo "=============================================="
