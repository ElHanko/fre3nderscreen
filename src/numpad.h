#ifndef __NUMPAD_H__
#define __NUMPAD_H__

#include "lvgl/lvgl.h"

#include <functional>

class Numpad {
 public:
  Numpad(lv_obj_t *parent);
  ~Numpad();

  void set_callback(std::function<void(double)> cb);
  void handle_input(lv_event_t *event);
  void handle_back(lv_event_t *event);
  void foreground_reset();

  static void _handle_input(lv_event_t *event) {
    auto *panel = static_cast<Numpad *>(event->user_data);
    panel->handle_input(event);
  }

  static void _handle_back(lv_event_t *event) {
    auto *panel = static_cast<Numpad *>(event->user_data);
    panel->handle_back(event);
  }

 private:
  void close();

  lv_obj_t *edit_cont;
  lv_obj_t *header_cont;
  lv_obj_t *back_btn;
  lv_obj_t *title_label;
  lv_obj_t *input;
  lv_obj_t *kb;
  std::function<void(double)> ready_cb;
};

#endif // __NUMPAD_H__
