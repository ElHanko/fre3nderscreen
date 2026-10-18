#ifndef FRE3NDER_SYSTEM_PANEL_H
#define FRE3NDER_SYSTEM_PANEL_H

#include "lvgl/lvgl.h"

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

// Native Fre3nder system information page; all data comes from managementd.
class SystemPanel {
 public:
  SystemPanel();
  ~SystemPanel();
  void foreground();

 private:
  static void event_callback(lv_event_t *event);
  static void timer_callback(lv_timer_t *timer);
  void refresh();
  void poll();

  lv_obj_t *cont;
  lv_obj_t *back_btn;
  lv_obj_t *refresh_btn;
  lv_obj_t *info_label;
  lv_timer_t *timer;

  std::thread worker;
  std::mutex result_mutex;
  std::string result_text;
  std::atomic<bool> ready{false};
};

#endif
