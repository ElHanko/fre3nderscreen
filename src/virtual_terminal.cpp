#include "virtual_terminal.h"

#ifndef SIMULATOR
#include "lv_drivers/display/fbdev.h"
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

void flush(lv_disp_drv_t *driver, const lv_area_t *area, lv_color_t *pixels) {
    if (active) fbdev_flush(driver, area, pixels);
    else lv_disp_flush_ready(driver);
}
}
#endif
