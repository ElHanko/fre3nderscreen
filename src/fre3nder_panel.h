#ifndef __FRE3NDER_PANEL_H__
#define __FRE3NDER_PANEL_H__

#include "wifi_panel.h"
#include "lvgl/lvgl.h"

#include <mutex>

class Fre3nderPanel {
 public:
  explicit Fre3nderPanel(std::mutex &lock);
  ~Fre3nderPanel();

  void foreground();
  void handle_callback(lv_event_t *event);

  static void _handle_callback(lv_event_t *event) {
    auto *panel = static_cast<Fre3nderPanel *>(event->user_data);
    panel->handle_callback(event);
  }

 private:
  WifiPanel wifi_panel;

  lv_obj_t *cont;
  lv_obj_t *header_cont;
  lv_obj_t *back_btn;
  lv_obj_t *title_label;
  lv_obj_t *wifi_btn;
};

#endif // __FRE3NDER_PANEL_H__
