#ifndef __LIMITS_PANEL_H__
#define __LIMITS_PANEL_H__

#include "lvgl/lvgl.h"
#include "websocket_client.h"
#include "notify_consumer.h"

#include <mutex>

class LimitsPanel : public NotifyConsumer {
 public:
  LimitsPanel(KWebSocketClient &client, std::mutex &lock);
  ~LimitsPanel();

  void init(json &j);
  void foreground();
  void consume(json &j);
  void handle_callback(lv_event_t *event);

  static void _handle_callback(lv_event_t *event) {
    auto *panel = static_cast<LimitsPanel *>(event->user_data);
    panel->handle_callback(event);
  }

 private:
  KWebSocketClient &ws;

  lv_obj_t *cont;
  lv_obj_t *header_cont;
  lv_obj_t *back_btn;
  lv_obj_t *title_label;
  lv_obj_t *content_cont;

  lv_obj_t *velocity_card;
  lv_obj_t *velocity_value;
  lv_obj_t *velocity_slider;
  lv_obj_t *velocity_reset;

  lv_obj_t *acceleration_card;
  lv_obj_t *acceleration_value;
  lv_obj_t *acceleration_slider;
  lv_obj_t *acceleration_reset;

  lv_obj_t *accel_to_decel_card;
  lv_obj_t *accel_to_decel_value;
  lv_obj_t *accel_to_decel_slider;
  lv_obj_t *accel_to_decel_reset;

  lv_obj_t *square_corner_card;
  lv_obj_t *square_corner_value;
  lv_obj_t *square_corner_slider;
  lv_obj_t *square_corner_reset;

  int max_velocity_default;
  int max_accel_default;
  int max_accel_to_decel_default;
  int square_corner_default;
};

#endif // __LIMITS_PANEL_H__
