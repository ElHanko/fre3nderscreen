#include "virtual_terminal.h"

#ifndef SIMULATOR
#include <linux/fb.h>
#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include <stdexcept>
#include <initializer_list>
#include <sys/ioctl.h>
#include <unistd.h>

namespace {
int terminal = -1;
int framebuffer = -1;
lv_display_flush_cb_t framebuffer_flush = nullptr;
bool active = false;
volatile sig_atomic_t release_pending = 0;
volatile sig_atomic_t acquire_pending = 0;
volatile sig_atomic_t stop_pending = 0;

void signal_handler(int number) {
    if (number == SIGUSR1) release_pending = 1;
    else if (number == SIGUSR2) acquire_pending = 1;
    else stop_pending = 1;
}

void check(int result) {
    if (result < 0) throw std::runtime_error("Linux VT operation failed");
}
}

namespace VirtualTerminal {
void attach_display(lv_display_t *display, const char *framebuffer_path) {
    framebuffer = open(framebuffer_path, O_RDWR | O_CLOEXEC);
    check(framebuffer);
    framebuffer_flush = lv_display_get_flush_cb(display);
    lv_display_set_flush_cb(display, flush);
}

void blank(bool enabled) {
    (void)ioctl(framebuffer, FBIOBLANK, enabled ? FB_BLANK_NORMAL : FB_BLANK_UNBLANK);
    if (!enabled) {
        struct fb_var_screeninfo info = {};
        if (ioctl(framebuffer, FBIOGET_VSCREENINFO, &info) == 0) {
            info.xoffset = 0;
            info.yoffset = 0;
            (void)ioctl(framebuffer, FBIOPAN_DISPLAY, &info);
        }
    }
}

void init(const char *path) {
    if (path == nullptr || path[0] == '\0')
        throw std::runtime_error("display VT is required");
    // S64 grants access to tty7 only. No capabilities or root UID are needed.
    check(setsid());
    terminal = open(path, O_RDWR | O_NOCTTY | O_CLOEXEC | O_NOFOLLOW);
    check(terminal);
    check(ioctl(terminal, TIOCSCTTY, 0));
    struct sigaction handler = {};
    handler.sa_handler = signal_handler;
    sigemptyset(&handler.sa_mask);
    for (int number : {SIGUSR1, SIGUSR2, SIGTERM, SIGINT, SIGHUP})
        check(sigaction(number, &handler, nullptr));
    struct vt_mode mode = {};
    mode.mode = VT_PROCESS;
    mode.relsig = SIGUSR1;
    mode.acqsig = SIGUSR2;
    check(ioctl(terminal, VT_SETMODE, &mode));
    check(ioctl(terminal, KDSETMODE, KD_GRAPHICS));
}

void activate() {
    check(ioctl(terminal, VT_ACTIVATE, 7));
    int result;
    do { result = ioctl(terminal, VT_WAITACTIVE, 7); } while (result < 0 && errno == EINTR && !stop_pending);
    check(result);
    if (acquire_pending) {
        acquire_pending = 0;
        acknowledge(true);
    }
    active = true;
}

Event poll() {
    if (stop_pending) return Stop;
    if (release_pending) {
        release_pending = 0;
        active = false; // No framebuffer writes after the release acknowledgment.
        return Release;
    }
    if (acquire_pending) {
        acquire_pending = 0;
        return Acquire;
    }
    return None;
}

void acknowledge(bool acquired) {
    check(ioctl(terminal, VT_RELDISP, acquired ? VT_ACKACQ : 1));
    active = acquired;
}

void shutdown() {
    active = false;
    if (framebuffer >= 0) {
        close(framebuffer);
        framebuffer = -1;
    }
    if (terminal < 0) return;
    struct vt_mode mode = {};
    mode.mode = VT_AUTO;
    (void)ioctl(terminal, VT_SETMODE, &mode);
    (void)ioctl(terminal, KDSETMODE, KD_TEXT);
    struct vt_stat state = {};
    if (ioctl(terminal, VT_GETSTATE, &state) == 0 && state.v_active == 7)
        (void)ioctl(terminal, VT_ACTIVATE, 1);
    close(terminal);
    terminal = -1;
}

void flush(lv_display_t *display, const lv_area_t *area, uint8_t *pixels) {
    if (active) framebuffer_flush(display, area, pixels);
    else lv_display_flush_ready(display);
}
}
#endif
