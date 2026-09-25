#!/bin/sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)
COMPOSE_FILE="$SCRIPT_DIR/compose.yaml"
CONTAINER_NAME="fre3nderscreen-simulator"

if ! command -v docker >/dev/null 2>&1; then
    echo "docker was not found." >&2
    exit 2
fi

mkdir -p "$REPO_ROOT/build/docker-sim"

cleanup() {
    rc=$?

    trap - EXIT INT TERM HUP

    docker stop -t 2 "$CONTAINER_NAME" >/dev/null 2>&1 || true
    docker rm -f "$CONTAINER_NAME" >/dev/null 2>&1 || true

    exit "$rc"
}

trap cleanup EXIT INT TERM HUP

# Remove a stale container left behind by an interrupted previous run.
docker rm -f "$CONTAINER_NAME" >/dev/null 2>&1 || true

echo "Fre3nderScreen simulator:"
echo "  VNC:     127.0.0.1:5901"
echo "  Browser: http://127.0.0.1:6080/vnc.html?autoconnect=1&resize=scale"

HOST_UID=$(id -u) \
HOST_GID=$(id -g) \
docker compose -f "$COMPOSE_FILE" run \
    --rm \
    --build \
    --service-ports \
    --name "$CONTAINER_NAME" \
    simulator &

DOCKER_PID=$!

set +e
wait "$DOCKER_PID"
RC=$?
set -e

exit "$RC"
