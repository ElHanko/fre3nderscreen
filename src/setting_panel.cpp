#include "setting_panel.h"
#include "config.h"
#include "spdlog/spdlog.h"
#include "subprocess.hpp"

#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;
namespace sp = subprocess;

LV_IMG_DECLARE(network_img);
LV_IMG_DECLARE(refresh_img);
LV_IMG_DECLARE(spoolman_img);
LV_IMG_DECLARE(sysinfo_img);

SettingPanel::SettingPanel(KWebSocketClient &c, std::mutex &l, lv_obj_t *parent, SpoolmanPanel &sm)
  : ws(c)
  , cont(lv_obj_create(parent))
  , wifi_panel(l)
  , sysinfo_panel()
  , spoolman_panel(sm)
  , wifi_btn(cont, &network_img, "WIFI", &SettingPanel::_handle_callback, this)
  , restart_klipper_btn(cont, &refresh_img, "Restart Klipper", &SettingPanel::_handle_callback, this)
  , restart_firmware_btn(cont, &refresh_img, "Restart\nFirmware", &SettingPanel::_handle_callback, this)
  , sysinfo_btn(cont, &sysinfo_img, "System", &SettingPanel::_handle_callback, this)
  , spoolman_btn(cont, &spoolman_img, "Spoolman", &SettingPanel::_handle_callback, this)
  , guppy_restart_btn(cont, &refresh_img, "Restart Fre3nderScreen", &SettingPanel::_handle_callback, this)
{
  lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));

  spoolman_btn.disable();

  lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(cont, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_AUTO);

  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(
      cont,
      LV_FLEX_ALIGN_START,
      LV_FLEX_ALIGN_CENTER,
      LV_FLEX_ALIGN_CENTER);

  lv_obj_set_style_pad_all(cont, 6, 0);
  lv_obj_set_style_pad_row(cont, 6, 0);

  lv_obj_t *buttons[] = {
    wifi_btn.get_container(),
    restart_klipper_btn.get_container(),
    restart_firmware_btn.get_container(),
    sysinfo_btn.get_container(),
    spoolman_btn.get_container(),
    guppy_restart_btn.get_container(),
  };

  for (lv_obj_t *button : buttons) {
    lv_obj_set_width(button, LV_PCT(100));
    lv_obj_set_height(button, 76);
  }


}

SettingPanel::~SettingPanel() {
  if (cont != NULL) {
    lv_obj_del(cont);
    cont = NULL;
  }
}

lv_obj_t *SettingPanel::get_container() {
  return cont;
}

void SettingPanel::handle_callback(lv_event_t *event) {
  if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
    lv_obj_t *btn = lv_event_get_current_target(event);

    if (btn == wifi_btn.get_container()) {
      spdlog::trace("wifi pressed");
      wifi_panel.foreground();
    } else if (btn == sysinfo_btn.get_container()) {
      spdlog::trace("setting system info pressed");
      sysinfo_panel.foreground();
    } else if (btn == restart_klipper_btn.get_container()) {
      spdlog::trace("setting restart klipper pressed");
      ws.send_jsonrpc("printer.restart");
    } else if (btn == restart_firmware_btn.get_container()) {
      spdlog::trace("setting restart klipper pressed");
      ws.send_jsonrpc("printer.firmware_restart");
    } else if (btn == spoolman_btn.get_container()) {
      spdlog::trace("setting spoolman pressed");
      spoolman_panel.foreground();
    } else if (btn == guppy_restart_btn.get_container()) {
      spdlog::trace("restart fre3nderscreen pressed");
      Config *conf = Config::get_instance();
      auto init_script = conf->get<std::string>("/fre3nderscreen_init_script");
      const fs::path script(init_script);
      if (fs::exists(script) || init_script.rfind("service fre3nderscreen", 0) == 0) {
        sp::call({init_script, "restart"});
      } else {
        spdlog::warn("Failed to restart Fre3nderScreen. Did not find restart script.");
      }
    }
  }
}

void SettingPanel::enable_spoolman() {
  spoolman_btn.enable();
}
