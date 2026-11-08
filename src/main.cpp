#include "lvgl/lvgl.h"
#include "lv_tc.h"
#include "lv_tc_screen.h"

#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <cstdlib>
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;


static void hal_init(lv_color_t p, lv_color_t s);

#include "fre3nderscreen.h"
#include "hv/hlog.h"
#include "config.h"
#include "touch_beep.h"
#include "virtual_terminal.h"

#include <algorithm>

using namespace hv;


int main(void)
{
    // config
    spdlog::debug("current path {}", std::string(fs::canonical("/proc/self/exe").parent_path()));

    Config *conf = Config::get_instance();
    const char *config_env = std::getenv("FRE3NDERSCREEN_CONFIG");
    auto config_path = config_env != NULL && config_env[0] != '\0'
        ? fs::path(config_env)
        : fs::canonical("/proc/self/exe").parent_path() / "fre3nderscreen.json";
    conf->init(config_path.string(), "/usr/data/printer_data/thumbnails");

#ifndef SIMULATOR
    const char *ready_file = std::getenv("FRE3NDERSCREEN_READY_FILE");
    try {
        VirtualTerminal::init(std::getenv("FRE3NDERSCREEN_VT"));
#endif
        Fre3nderScreen::init(hal_init);
        Fre3nderScreen::loop();
#ifndef SIMULATOR
    } catch (const std::exception &error) {
        spdlog::error("display startup/runtime failed: {}", error.what());
        TouchBeep::shutdown();
        VirtualTerminal::shutdown();
        if (ready_file != nullptr) unlink(ready_file);
        return 1;
    }
    TouchBeep::shutdown();
    VirtualTerminal::shutdown();
    if (ready_file != nullptr) unlink(ready_file);
#endif
    return 0;
}

static void hal_init(lv_color_t primary, lv_color_t secondary) {
#ifndef SIMULATOR
    const char *framebuffer = std::getenv("FRE3NDERSCREEN_FRAMEBUFFER");
    if (framebuffer == nullptr || framebuffer[0] == '\0')
        throw std::runtime_error("display framebuffer is required");
    lv_display_t *disp = lv_linux_fbdev_create();
    if (disp == nullptr) throw std::runtime_error("cannot create framebuffer display");
    VirtualTerminal::attach_display(disp, framebuffer);
    VirtualTerminal::blank(false);
    if (lv_linux_fbdev_set_file(disp, framebuffer) != LV_RESULT_OK)
        throw std::runtime_error("cannot initialize framebuffer");
    if (lv_display_get_horizontal_resolution(disp) != 480 ||
        lv_display_get_vertical_resolution(disp) != 272 ||
        lv_display_get_color_format(disp) != LV_COLOR_FORMAT_XRGB8888)
        throw std::runtime_error("480x272 XRGB8888 framebuffer is not initialized");
    lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90);
#else
    lv_display_t *disp = lv_sdl_window_create(272, 480);
    if (disp == nullptr) throw std::runtime_error("cannot initialize SDL display");
    lv_sdl_window_set_title(disp, "Fre3nderScreen");
#endif
    lv_display_set_dpi(disp, LV_DPI_DEF);
    lv_theme_t *theme = lv_theme_default_init(disp, primary, secondary, true, &lv_font_montserrat_12);
    lv_display_set_theme(disp, theme);

#ifndef SIMULATOR
    const char *input = std::getenv("FRE3NDERSCREEN_INPUT");
    if (input == nullptr || input[0] == '\0')
        throw std::runtime_error("display touch input is required");
    lv_indev_t *touch = lv_evdev_create(LV_INDEV_TYPE_POINTER, input);
    if (touch == nullptr) throw std::runtime_error("cannot initialize touch input");
    lv_indev_set_display(touch, disp);
    // Match the old driver's raw coordinate basis, also when calibration is off.
    lv_evdev_set_raw_mode(touch, true);
    auto calibrated = Config::get_instance()->get_json("/touch_calibrated");
    if (!calibrated.is_null() && calibrated.get<bool>()) {
        lv_tc_indev_init(touch);
    }
    TouchBeep::init(std::getenv("FRE3NDERSCREEN_BEEPER_INPUT"));
    lv_indev_add_event_cb(touch, TouchBeep::feedback_cb, LV_EVENT_CLICKED, nullptr);
#else
    lv_group_t *group = lv_group_create();
    lv_group_set_default(group);
    lv_indev_t *mouse = lv_sdl_mouse_create();
    lv_indev_set_display(mouse, disp);
    lv_indev_t *keyboard = lv_sdl_keyboard_create();
    lv_indev_set_display(keyboard, disp);
    lv_indev_set_group(keyboard, group);
    lv_indev_t *wheel = lv_sdl_mousewheel_create();
    lv_indev_set_display(wheel, disp);
    lv_indev_set_group(wheel, group);
#endif
}
