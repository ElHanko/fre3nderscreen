#ifndef __SETTING_PANEL_H__
#define __SETTING_PANEL_H__

#include "wifi_panel.h"
#include "sysinfo_panel.h"
#include "spoolman_panel.h"
#include "websocket_client.h"
#include "lvgl/lvgl.h"

#include <mutex>

class SettingPanel {
 public:
  SettingPanel(KWebSocketClient &c,
               std::mutex &l,
               lv_obj_t *parent,
               SpoolmanPanel &sm);
  ~SettingPanel();

  lv_obj_t *get_container();
  void enable_spoolman();
  void handle_callback(lv_event_t *event);

  static void _handle_callback(lv_event_t *event) {
    auto *panel = static_cast<SettingPanel *>(event->user_data);
    panel->handle_callback(event);
  }

 private:
  KWebSocketClient &ws;
  lv_obj_t *cont;

  WifiPanel wifi_panel;
  SysInfoPanel sysinfo_panel;
  SpoolmanPanel &spoolman_panel;

  lv_obj_t *wifi_btn;
  lv_obj_t *sysinfo_btn;
  lv_obj_t *restart_klipper_btn;
  lv_obj_t *restart_firmware_btn;
  lv_obj_t *spoolman_btn;
  lv_obj_t *screen_restart_btn;
};

#endif // __SETTING_PANEL_H__
