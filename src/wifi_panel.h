#ifndef __WIFI_PANEL_H__
#define __WIFI_PANEL_H__

#include "lvgl/lvgl.h"

#include <mutex>

class WifiPanel {
 public:
  explicit WifiPanel(std::mutex &lock);
  ~WifiPanel();

  void foreground();
  void handle_back_btn(lv_event_t *event);

  static void _handle_back_btn(lv_event_t *event) {
    auto *panel = static_cast<WifiPanel *>(event->user_data);
    panel->handle_back_btn(event);
  }

 private:
  lv_obj_t *cont;
  lv_obj_t *header_cont;
  lv_obj_t *back_btn;
  lv_obj_t *title_label;
  lv_obj_t *placeholder_card;
};

#endif // __WIFI_PANEL_H__
