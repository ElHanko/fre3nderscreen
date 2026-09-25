#ifndef __FAN_PANEL_H__
#define __FAN_PANEL_H__

#include "lvgl/lvgl.h"
#include "websocket_client.h"
#include "notify_consumer.h"

#include <map>
#include <mutex>
#include <string>

class FanPanel : public NotifyConsumer {
 public:
  FanPanel(KWebSocketClient &ws, std::mutex &lock);
  ~FanPanel();

  void consume(json &data);
  lv_obj_t *get_container();
  void create_fans(json &fans);
  void foreground();
  void handle_callback(lv_event_t *event);
  void handle_fan_update(lv_event_t *event);
  void handle_fan_update_part_fan(lv_event_t *event);
  void handle_fan_update_generic(lv_event_t *event);

  static void _handle_callback(lv_event_t *event) {
    auto *panel = static_cast<FanPanel *>(event->user_data);
    panel->handle_callback(event);
  }

  static void _handle_fan_update(lv_event_t *event) {
    auto *panel = static_cast<FanPanel *>(event->user_data);
    panel->handle_fan_update(event);
  }

  static void _handle_fan_update_part_fan(lv_event_t *event) {
    auto *panel = static_cast<FanPanel *>(event->user_data);
    panel->handle_fan_update_part_fan(event);
  }

  static void _handle_fan_update_generic(lv_event_t *event) {
    auto *panel = static_cast<FanPanel *>(event->user_data);
    panel->handle_fan_update_generic(event);
  }

 private:
  struct FanControl {
    lv_obj_t *card;
    lv_obj_t *slider;
    lv_obj_t *value_label;
    lv_obj_t *off_btn;
    lv_obj_t *max_btn;
  };

  FanControl create_fan_control(const std::string &display_name,
                                lv_event_cb_t callback);
  void update_value(FanControl &control, int value);
  void sync_value_event(lv_event_t *event);

  KWebSocketClient &ws;
  lv_obj_t *fanpanel_cont;
  lv_obj_t *header_cont;
  lv_obj_t *back_btn;
  lv_obj_t *title_label;
  lv_obj_t *fans_cont;
  std::map<std::string, FanControl> fans;
};

#endif // __FAN_PANEL_H__
