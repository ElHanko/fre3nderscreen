#!/bin/sh
set -eu

MODE="${1:-simulator}"
SOURCE_DIR=/src
WORK_DIR=/work/src
RUNTIME_DIR=/work/runtime

if [ "$(id -u)" -eq 0 ]; then
    HOST_UID="${HOST_UID:-1000}"
    HOST_GID="${HOST_GID:-1000}"

    mkdir -p /work
    chown -R "$HOST_UID:$HOST_GID" /work

    if [ -d /screenshots ]; then
        chown -R "$HOST_UID:$HOST_GID" /screenshots
    fi

    exec gosu "$HOST_UID:$HOST_GID" "$0" "$@"
fi

prepare_source() {
    mkdir -p "$WORK_DIR" "$RUNTIME_DIR/logs" "$RUNTIME_DIR/thumbnails" "$RUNTIME_DIR/wpa_supplicant" "$HOME"

    # Keep generated build products in /work while mirroring source changes from
    # the read-only checkout. Excluded compiler products provide an incremental
    # build cache without writing into the host repository.
    rsync -a --delete \
        --exclude '/.git/' \
        --exclude '/build/' \
        --exclude '*/build/' \
        --exclude '*.o' \
        --exclude '*.a' \
        --exclude '*.d' \
        "$SOURCE_DIR/" "$WORK_DIR/"

    for required in \
        lvgl/lvgl.mk \
        lv_drivers/lv_drivers.mk \
        libhv/Makefile \
        spdlog/CMakeLists.txt \
        wpa_supplicant/wpa_supplicant/Makefile
    do
        if [ ! -e "$WORK_DIR/$required" ]; then
            echo "Missing submodule content: $required" >&2
            echo "Run: git submodule update --init --recursive" >&2
            exit 2
        fi
    done
}

prepare_config() {
    CONFIG_SOURCE="${FRE3NDERSCREEN_CONFIG_SOURCE:-$SOURCE_DIR/dev/simulator/fre3nderscreen.json}"
    if [ ! -f "$CONFIG_SOURCE" ]; then
        echo "Fre3nderScreen config not found: $CONFIG_SOURCE" >&2
        exit 2
    fi

    # Config::init() writes defaults back to its input file. Always give it a
    # disposable writable copy instead of the tracked source configuration.
    cp "$CONFIG_SOURCE" "$RUNTIME_DIR/fre3nderscreen.json"
    export FRE3NDERSCREEN_CONFIG="$RUNTIME_DIR/fre3nderscreen.json"
}

build_simulator() {
    cd "$WORK_DIR"
    unset CROSS_COMPILE

    if [ "${SIMULATOR_FORCE_REBUILD:-0}" = "1" ] || [ ! -x build/bin/fre3nderscreen ] || [ ! -f /work/.simulator-built ]; then
        rm -rf build
        make -j"$(nproc)" GUPPY_SMALL_SCREEN=1 GUPPY_ROTATE=1 build
        touch /work/.simulator-built
    else
        make -j"$(nproc)" GUPPY_SMALL_SCREEN=1 GUPPY_ROTATE=1
    fi
}

run_simulator() {
    cd "$WORK_DIR"
    exec ./build/bin/fre3nderscreen
}

run_screenshot() {
    DISPLAY_NUMBER="${SCREENSHOT_DISPLAY:-99}"
    export DISPLAY=":$DISPLAY_NUMBER"
    OUTPUT_NAME="${SCREENSHOT_NAME:-fre3nderscreen}"
    OUTPUT_PATH="/screenshots/$OUTPUT_NAME.png"

    case "$OUTPUT_NAME" in
        *[!A-Za-z0-9._-]*|'')
            echo "SCREENSHOT_NAME may contain only A-Z, a-z, 0-9, dot, underscore and dash." >&2
            exit 2
            ;;
    esac

    mkdir -p /screenshots

    Xvfb "$DISPLAY" -screen 0 272x480x24 -nolisten tcp -ac >/work/runtime/xvfb.log 2>&1 &
    XVFB_PID=$!
    APP_PID=

    cleanup() {
        if [ -n "${APP_PID:-}" ]; then
            kill "$APP_PID" 2>/dev/null || true
            wait "$APP_PID" 2>/dev/null || true
        fi
        kill "$XVFB_PID" 2>/dev/null || true
        wait "$XVFB_PID" 2>/dev/null || true
    }
    trap cleanup EXIT INT TERM HUP

    i=0
    while [ ! -S "/tmp/.X11-unix/X$DISPLAY_NUMBER" ]; do
        i=$((i + 1))
        if [ "$i" -ge 50 ]; then
            echo "Xvfb did not become ready." >&2
            cat /work/runtime/xvfb.log >&2 || true
            exit 1
        fi
        sleep 0.1
    done

    cd "$WORK_DIR"
    ./build/bin/fre3nderscreen >/work/runtime/fre3nderscreen-stdout.log 2>&1 &
    APP_PID=$!

    DELAY="${SCREENSHOT_DELAY:-3}"
    sleep "$DELAY"

    if ! kill -0 "$APP_PID" 2>/dev/null; then
        echo "Fre3nderScreen exited before the screenshot was captured." >&2
        cat /work/runtime/fre3nderscreen-stdout.log >&2 || true
        exit 1
    fi

    import -display "$DISPLAY" -window root "$OUTPUT_PATH"

    if [ ! -s "$OUTPUT_PATH" ]; then
        echo "Screenshot was not created: $OUTPUT_PATH" >&2
        exit 1
    fi

    echo "$OUTPUT_PATH"
}

prepare_source
prepare_config
build_simulator

case "$MODE" in
    simulator)
        run_simulator
        ;;
    screenshot)
        run_screenshot
        ;;
    *)
        echo "Unknown mode: $MODE (expected simulator or screenshot)" >&2
        exit 2
        ;;
esac
