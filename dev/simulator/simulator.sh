#!/bin/sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)
COMPOSE_FILE="$SCRIPT_DIR/compose.yaml"
CONTAINER_NAME="fre3nderscreen-simulator"

if [ -z "${DISPLAY:-}" ]; then
    echo "DISPLAY is not set. The interactive simulator requires an X11 session." >&2
    exit 2
fi

if ! command -v docker >/dev/null 2>&1; then
    echo "docker was not found." >&2
    exit 2
fi

if ! command -v xhost >/dev/null 2>&1; then
    echo "xhost was not found. Install x11-xserver-utils." >&2
    exit 2
fi

mkdir -p "$REPO_ROOT/build/docker-sim"

XUSER=$(id -un)

cleanup() {
    rc=$?

    trap - EXIT INT TERM HUP

    docker stop -t 2 "$CONTAINER_NAME" >/dev/null 2>&1 || true
    docker rm -f "$CONTAINER_NAME" >/dev/null 2>&1 || true

    xhost -SI:localuser:"$XUSER" >/dev/null 2>&1 || true

    exit "$rc"
}

trap cleanup EXIT INT TERM HUP

# Remove a stale container left behind by an interrupted previous run.
docker rm -f "$CONTAINER_NAME" >/dev/null 2>&1 || true

xhost +SI:localuser:"$XUSER" >/dev/null

HOST_UID=$(id -u) \
HOST_GID=$(id -g) \
docker compose -f "$COMPOSE_FILE" run \
    --rm \
    --build \
    --name "$CONTAINER_NAME" \
    simulator &

DOCKER_PID=$!

set +e
wait "$DOCKER_PID"
RC=$?
set -e

exit "$RC"
