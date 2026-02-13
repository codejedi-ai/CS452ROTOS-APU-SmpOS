#!/bin/bash
# QEMU Runner for Galatea-NIX kernel

set -e

# Build the kernel
echo "[*] Building kernel..."
make clean > /dev/null 2>&1
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
  -kernel 0-d273liu.elf \
  2>&1 | tee qemu_run.log

echo "========================================="
echo "[*] QEMU exited"
echo "[*] Debug log saved to qemu_debug.log"
echo "[*] Output saved to qemu_run.log"
