#ifndef __FRE3NDERSCREEN_PANEL_H__
#define __FRE3NDERSCREEN_PANEL_H__

#include "lvgl/lvgl.h"

#include <cstdint>
#include <string>
#include <vector>

class Fre3nderScreenPanel {
 public:
  Fre3nderScreenPanel();
  ~Fre3nderScreenPanel();

  void foreground();
  void handle_callback(lv_event_t *event);

  static void _handle_callback(lv_event_t *event) {
    auto *panel = static_cast<Fre3nderScreenPanel *>(event->user_data);
    panel->handle_callback(event);
  }

 private:
  lv_obj_t *cont;
  lv_obj_t *header_cont;
  lv_obj_t *back_btn;
  lv_obj_t *title_label;

  lv_obj_t *content_cont;
  lv_obj_t *version_card;
  lv_obj_t *settings_card;

  lv_obj_t *display_sleep_dd;
  lv_obj_t *loglevel_dd;
  lv_obj_t *prompt_estop_toggle;
  lv_obj_t *z_icon_toggle;
  lv_obj_t *restart_btn;

  uint32_t loglevel;

  static std::vector<std::string> log_levels;
};

#endif // __FRE3NDERSCREEN_PANEL_H__
