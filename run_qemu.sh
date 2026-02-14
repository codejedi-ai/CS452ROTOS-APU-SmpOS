#!/bin/bash
# QEMU Runner for Galatea-NIX kernel

set -e

# Build the kernel
echo "[*] Building kernel..."
make > /dev/null 2>&1

# Run QEMU with proper output forwarding
echo "[*] Starting QEMU (Ctrl+C to stop)..."
echo "========================================="

qemu-system-aarch64 \
  -M virt \
  -cpu cortex-a72 \
  -nographic \
  -monitor none \
  -serial stdio \
  -kernel 0-d273liu.elf

echo ""
echo "========================================="
echo "[*] QEMU exited"
