#ifndef __SETTING_PANEL_H__
#define __SETTING_PANEL_H__

#include "fre3nder_panel.h"
#include "fre3nderscreen_panel.h"
#include "lvgl/lvgl.h"

#include <mutex>

class SettingPanel {
 public:
  SettingPanel(std::mutex &lock, lv_obj_t *parent);
  ~SettingPanel();

  lv_obj_t *get_container();
  void handle_callback(lv_event_t *event);

  static void _handle_callback(lv_event_t *event) {
    auto *panel = static_cast<SettingPanel *>(event->user_data);
    panel->handle_callback(event);
  }

 private:
  lv_obj_t *cont;

  Fre3nderPanel fre3nder_panel;
  Fre3nderScreenPanel fre3nderscreen_panel;

  lv_obj_t *fre3nder_btn;
  lv_obj_t *fre3nderscreen_btn;
};

#endif // __SETTING_PANEL_H__
