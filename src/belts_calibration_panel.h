#ifndef __BELTS_CALIBRATION_PANEL_H__
#define __BELTS_CALIBRATION_PANEL_H__

#include "websocket_client.h"
#include "lvgl/lvgl.h"

#include <mutex>
#include <vector>

class BeltsCalibrationPanel {
 public:
  BeltsCalibrationPanel(KWebSocketClient &client, std::mutex &lock);
  ~BeltsCalibrationPanel();

  void foreground();
  void handle_callback(lv_event_t *event);
  void handle_image_clicked(lv_event_t *event);
  void handle_macro_response(json &j);
  void handle_update_slider(lv_event_t *event);
  void handle_emergency_prompt(lv_event_t *event);

  static void _handle_callback(lv_event_t *event) {
    auto *panel = static_cast<BeltsCalibrationPanel *>(event->user_data);
    panel->handle_callback(event);
  }

  static void _handle_image_clicked(lv_event_t *event) {
    auto *panel = static_cast<BeltsCalibrationPanel *>(event->user_data);
    panel->handle_image_clicked(event);
  }

  static void _handle_update_slider(lv_event_t *event) {
    auto *panel = static_cast<BeltsCalibrationPanel *>(event->user_data);
    panel->handle_update_slider(event);
  }

  static void _handle_emergency_prompt(lv_event_t *event) {
    auto *panel = static_cast<BeltsCalibrationPanel *>(event->user_data);
    panel->handle_emergency_prompt(event);
  }

 private:
  void request_emergency_stop();

  KWebSocketClient &ws;
  std::mutex &lv_lock;

  lv_obj_t *cont;
  lv_obj_t *header_cont;
  lv_obj_t *back_btn;
  lv_obj_t *title_label;

  lv_obj_t *graph_cont;
  lv_obj_t *graph;
  lv_obj_t *graph_label;
  lv_obj_t *spinner;

  lv_obj_t *excite_control;
  lv_obj_t *excite_title;
  lv_obj_t *excite_slider;
  lv_obj_t *excite_label;
  lv_obj_t *excite_dd;

  lv_obj_t *button_cont;
  lv_obj_t *calibrate_btn;
  lv_obj_t *excite_btn;
  lv_obj_t *emergency_btn;
  lv_obj_t *emergency_prompt;
  lv_obj_t *emergency_confirm_btn;
  lv_obj_t *emergency_cancel_btn;

  bool image_fullsized;

  static std::vector<std::string> axes;
};

#endif // __BELTS_CALIBRATION_PANEL_H__
