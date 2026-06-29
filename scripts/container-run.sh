#!/usr/bin/env bash
set -euo pipefail

cd /workspace

run_os() {
	echo "Starting SmpOS under QEMU (raspi4b). Ctrl+A then X to exit."
	exec bash /workspace/docker-run.sh
}

case "${1:-run}" in
	run)
		run_os
		;;
	test)
		exec bash /workspace/docker-test.sh
		;;
	shell|bash)
		exec bash
		;;
	build)
		make -j"$(nproc)" all
		;;
	make)
		shift
		exec make -j"$(nproc)" "$@"
		;;
	*)
		exec "$@"
		;;
esac
