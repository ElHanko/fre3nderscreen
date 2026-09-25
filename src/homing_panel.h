#ifndef __HOMING_PANEL_H__
#define __HOMING_PANEL_H__

#include "lvgl/lvgl.h"
#include "websocket_client.h"
#include "notify_consumer.h"

#include <mutex>
#include <string>

class HomingPanel : public NotifyConsumer {
 public:
  HomingPanel(KWebSocketClient &ws, std::mutex &lock);
  ~HomingPanel();

  void consume(json &data);
  lv_obj_t *get_container();
  void foreground();
  void handle_callback(lv_event_t *event);
  void handle_distance_cb(lv_event_t *event);
  void handle_emergency_prompt(lv_event_t *event);

  static void _handle_callback(lv_event_t *event) {
    auto *panel = static_cast<HomingPanel *>(event->user_data);
    panel->handle_callback(event);
  }

  static void _handle_distance_cb(lv_event_t *event) {
    auto *panel = static_cast<HomingPanel *>(event->user_data);
    panel->handle_distance_cb(event);
  }

  static void _handle_emergency_prompt(lv_event_t *event) {
    auto *panel = static_cast<HomingPanel *>(event->user_data);
    panel->handle_emergency_prompt(event);
  }

 private:
  void update_axes(const std::string &homed_axes);
  void update_z_icons();
  void set_button_enabled(lv_obj_t *button, bool enabled);
  const char *selected_distance() const;
  void request_emergency_stop();

  KWebSocketClient &ws;

  lv_obj_t *homing_cont;
  lv_obj_t *header_cont;
  lv_obj_t *back_btn;
  lv_obj_t *title_label;

  lv_obj_t *home_all_btn;
  lv_obj_t *home_xy_btn;
  lv_obj_t *y_up_btn;
  lv_obj_t *y_down_btn;
  lv_obj_t *x_up_btn;
  lv_obj_t *x_down_btn;
  lv_obj_t *z_up_btn;
  lv_obj_t *z_down_btn;
  lv_obj_t *z_up_icon;
  lv_obj_t *z_down_icon;
  lv_obj_t *motoroff_btn;
  lv_obj_t *emergency_btn;

  lv_obj_t *distance_btnm;
  uint32_t distance_idx;

  lv_obj_t *emergency_prompt;
  lv_obj_t *emergency_confirm_btn;
  lv_obj_t *emergency_cancel_btn;
};

#endif // __HOMING_PANEL_H__
