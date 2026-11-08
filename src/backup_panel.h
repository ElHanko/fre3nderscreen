#ifndef FRE3NDER_BACKUP_PANEL_H
#define FRE3NDER_BACKUP_PANEL_H

#include "lvgl/lvgl.h"
#include "hv/json.hpp"
#include "management_client.h"
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// Backup selection and receipts only; Management and Backup-Core own execution.
class BackupPanel {
 public:
  BackupPanel();
  ~BackupPanel();
  void foreground();

 private:
  static void event_callback(lv_event_t *event);
  static void timer_callback(lv_timer_t *timer);
  void start(bool create);
  void poll();
  void render();
  void targets();
  bool running() const;

  lv_obj_t *cont;
  lv_obj_t *back;
  lv_obj_t *refresh;
  lv_obj_t *role;
  lv_obj_t *target;
  lv_obj_t *create;
  lv_obj_t *message;
  lv_timer_t *timer;
  nlohmann::json snapshot;
  std::vector<std::string> destinations;
  std::string choice;
  std::string notice;
  bool open = false;
  bool busy = false;
  bool creating = false;
  bool refresh_needed = false;
  bool uncertain = false;
  uint32_t last_refresh = 0;
  std::thread worker;
  std::mutex result_mutex;
  ManagementClient::Response result;
  std::atomic<bool> ready{false};
};

#endif
