#ifndef __LED_PANEL_H__
#define __LED_PANEL_H__

#include "lvgl/lvgl.h"
#include "websocket_client.h"
#include "notify_consumer.h"

#include <map>
#include <mutex>
#include <string>

class LedPanel : public NotifyConsumer {
 public:
  LedPanel(KWebSocketClient &ws, std::mutex &lock);
  ~LedPanel();

  void consume(json &data);
  lv_obj_t *get_container();
  void init(json &leds);
  void foreground();
  void handle_callback(lv_event_t *event);
  void handle_led_update(lv_event_t *event);
  void handle_led_update_generic(lv_event_t *event);

  static void _handle_callback(lv_event_t *event) {
    auto *panel = static_cast<LedPanel *>(event->user_data);
    panel->handle_callback(event);
  }

  static void _handle_led_update(lv_event_t *event) {
    auto *panel = static_cast<LedPanel *>(event->user_data);
    panel->handle_led_update(event);
  }

  static void _handle_led_update_generic(lv_event_t *event) {
    auto *panel = static_cast<LedPanel *>(event->user_data);
    panel->handle_led_update_generic(event);
  }

 private:
  struct LedControl {
    lv_obj_t *card;
    lv_obj_t *slider;
    lv_obj_t *value_label;
    lv_obj_t *off_btn;
    lv_obj_t *max_btn;
  };

  LedControl create_led_control(const std::string &display_name,
                                lv_event_cb_t callback);
  void update_value(LedControl &control, int value);
  void sync_value_event(lv_event_t *event);

  KWebSocketClient &ws;
  lv_obj_t *ledpanel_cont;
  lv_obj_t *header_cont;
  lv_obj_t *back_btn;
  lv_obj_t *title_label;
  lv_obj_t *leds_cont;
  std::map<std::string, LedControl> leds;
};

#endif // __LED_PANEL_H__
