#ifndef __FINETINE_PANEL_H__
#define __FINETINE_PANEL_H__

#include "lvgl/lvgl.h"
#include "websocket_client.h"
#include "notify_consumer.h"

#include <mutex>

class FineTunePanel : public NotifyConsumer {
 public:
  FineTunePanel(KWebSocketClient &, std::mutex &);
  ~FineTunePanel();

  void foreground();
  void handle_callback(lv_event_t *event);
  void handle_zoffset(lv_event_t *event);
  void handle_pa(lv_event_t *event);
  void handle_speed(lv_event_t *event);
  void handle_flow(lv_event_t *event);

  void consume(json &j);

  static void _handle_callback(lv_event_t *event) {
    auto *panel = static_cast<FineTunePanel *>(event->user_data);
    panel->handle_callback(event);
  }

  static void _handle_zoffset(lv_event_t *event) {
    auto *panel = static_cast<FineTunePanel *>(event->user_data);
    panel->handle_zoffset(event);
  }

  static void _handle_pa(lv_event_t *event) {
    auto *panel = static_cast<FineTunePanel *>(event->user_data);
    panel->handle_pa(event);
  }

  static void _handle_speed(lv_event_t *event) {
    auto *panel = static_cast<FineTunePanel *>(event->user_data);
    panel->handle_speed(event);
  }

  static void _handle_flow(lv_event_t *event) {
    auto *panel = static_cast<FineTunePanel *>(event->user_data);
    panel->handle_flow(event);
  }

 private:
  KWebSocketClient &ws;

  lv_obj_t *panel_cont;
  lv_obj_t *header_cont;
  lv_obj_t *back_btn;
  lv_obj_t *title_label;
  lv_obj_t *content_cont;

  lv_obj_t *zpa_step_cont;
  lv_obj_t *zoffset_selector;
  lv_obj_t *multiplier_step_cont;
  lv_obj_t *multiplier_selector;

  lv_obj_t *z_card;
  lv_obj_t *z_value;
  lv_obj_t *zreset_btn;
  lv_obj_t *zup_btn;
  lv_obj_t *zup_icon;
  lv_obj_t *zdown_btn;
  lv_obj_t *zdown_icon;

  lv_obj_t *pa_card;
  lv_obj_t *pa_value;
  lv_obj_t *pareset_btn;
  lv_obj_t *paup_btn;
  lv_obj_t *padown_btn;

  lv_obj_t *speed_card;
  lv_obj_t *speed_value;
  lv_obj_t *speed_reset_btn;
  lv_obj_t *speed_up_btn;
  lv_obj_t *speed_down_btn;

  lv_obj_t *flow_card;
  lv_obj_t *flow_value;
  lv_obj_t *flow_reset_btn;
  lv_obj_t *flow_up_btn;
  lv_obj_t *flow_down_btn;

  uint32_t zoffset_selector_idx;
  uint32_t multiplier_selector_idx;
};

#endif  // __FINETINE_PANEL_H__
