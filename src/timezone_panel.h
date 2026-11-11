#ifndef FRE3NDER_TIMEZONE_PANEL_H
#define FRE3NDER_TIMEZONE_PANEL_H

#include "lvgl/lvgl.h"
#include "management_client.h"

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// UI only: Core supplies the catalog, validation, persistence and boot policy.
class TimezonePanel {
 public:
  TimezonePanel();
  ~TimezonePanel();
  void foreground();

 private:
  static void event_callback(lv_event_t *event);
  static void timer_callback(lv_timer_t *timer);
  void start(bool save);
  void poll();
  void places(const std::string &selected = {});
  void controls();

  lv_obj_t *cont;
  lv_obj_t *back;
  lv_obj_t *refresh;
  lv_obj_t *region;
  lv_obj_t *place;
  lv_obj_t *save;
  lv_obj_t *state;
  lv_obj_t *message;
  lv_timer_t *timer;
  std::vector<std::string> zones;
  std::vector<std::string> regions;
  std::vector<std::string> locations;
  bool busy = false;
  bool saving = false;
  std::thread worker;
  std::mutex result_mutex;
  ManagementClient::Response result;
  std::atomic<bool> ready{false};
};

#endif
