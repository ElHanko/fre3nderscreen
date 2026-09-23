#!/bin/sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)
COMPOSE_FILE="$SCRIPT_DIR/compose.yaml"
SCREENSHOT_NAME=${1:-fre3nderscreen}

case "$SCREENSHOT_NAME" in
    *.png) SCREENSHOT_NAME=${SCREENSHOT_NAME%.png} ;;
esac

case "$SCREENSHOT_NAME" in
    *[!A-Za-z0-9._-]*|'')
        echo "Screenshot name may contain only A-Z, a-z, 0-9, dot, underscore and dash." >&2
        exit 2
        ;;
esac

if ! command -v docker >/dev/null 2>&1; then
    echo "docker was not found." >&2
    exit 2
fi

mkdir -p "$REPO_ROOT/build/docker-sim" "$REPO_ROOT/build/ui-screenshots"

HOST_UID=$(id -u) \
HOST_GID=$(id -g) \
SCREENSHOT_NAME="$SCREENSHOT_NAME" \
docker compose -f "$COMPOSE_FILE" run --rm --build screenshot

echo "Screenshot: $REPO_ROOT/build/ui-screenshots/$SCREENSHOT_NAME.png"
