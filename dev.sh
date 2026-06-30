#!/usr/bin/env bash
# Host dev wrapper — DarcyOS (APU)
set -euo pipefail

OS_NAME="DarcyOS (APU)"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"

PLATFORM_DEFAULT="${ROOT}/../github_codejedi-ai_CS452ROTOS-PLATFORM"
if [[ -d "${PLATFORM_DEFAULT}" ]]; then
	PLATFORM="$(cd "${PLATFORM_DEFAULT}" && pwd)"
elif [[ -n "${PLATFORM:-}" && -d "${PLATFORM}" ]]; then
	PLATFORM="$(cd "${PLATFORM}" && pwd)"
else
	echo "Cannot find CS452ROTOS-PLATFORM (expected ${PLATFORM_DEFAULT} or set PLATFORM)." >&2
	exit 1
fi

# shellcheck source=../setup-host-env.sh
source "${PLATFORM}/scripts/setup-host-env.sh"

usage() {
	cat <<EOF
Usage: ./dev.sh <command>

Commands:
  setup       Install/check host toolchain and QEMU (from CS452ROTOS-PLATFORM)
  run         Build kernel and run in QEMU on the host (Ctrl+A X to exit)
  test        Timed smoke boot under QEMU
  make [args] Run make on the host (default: make all)

Environment:
  XDIR        ARM GNU toolchain prefix (default: /opt/toolchain)
  PLATFORM    Path to CS452ROTOS-PLATFORM checkout (optional)
EOF
}

setup_terminal() {
	if [[ -t 0 ]]; then
		stty -icanon -echo min 1 time 0 2>/dev/null || true
		trap 'stty icanon echo 2>/dev/null || true' EXIT INT TERM
	fi
}

run_os() {
	setup_terminal
	echo "Starting ${OS_NAME} under QEMU (raspi4b). Ctrl+A then X to exit."
	exec bash "${ROOT}/run.sh"
}

cmd="${1:-run}"
shift || true

case "${cmd}" in
	setup)
		setup_host_env "${ROOT}"
		echo "Host toolchain and QEMU are ready."
		;;
	run)
		setup_host_env "${ROOT}"
		run_os
		;;
	test)
		setup_host_env "${ROOT}"
		exec bash "${ROOT}/test.sh"
		;;
	make)
		setup_host_env "${ROOT}"
		if [[ "$#" -eq 0 ]]; then
			make -j"$(nproc)" all
		else
			make -j"$(nproc)" "$@"
		fi
		;;
	help|-h|--help)
		usage
		;;
	*)
		echo "Unknown command: ${cmd}" >&2
		usage >&2
		exit 1
		;;
esac
