#ifndef FRE3NDER_APPS_PANEL_H
#define FRE3NDER_APPS_PANEL_H

#include "lvgl/lvgl.h"
#include "hv/json.hpp"
#include "management_client.h"

#include <atomic>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// Native administration via Fre3nder Core; never calls Package-Core directly.
class AppsPanel {
 public:
  AppsPanel();
  ~AppsPanel();
  void foreground();

 private:
  enum class Operation { Status, Detail, Action };
  enum class Action { Refresh, SelectApp, SelectStage, SelectUsb, Detail,
      Remove, SelectDisplay, DisableDisplay, Verify, InstallSys, InstallHome,
      Update, Discard, Acknowledge, Upload };
  struct Control { Action action; std::string argument; };

  static void event_callback(lv_event_t *event);
  static void timer_callback(lv_timer_t *timer);
  void handle(lv_event_t *event);
  void start(Operation kind, std::function<ManagementClient::Response()> request);
  void load();
  void detail();
  void poll();
  void render();
  void render_controls();
  void add_button(lv_obj_t *parent, const std::string &text,
                  Action action, const std::string &argument = {}, bool dangerous = false);
  void take_action(Action action);
  void scan_usb();
  void clear_confirmation();

  lv_obj_t *cont;
  lv_obj_t *back_btn;
  lv_obj_t *refresh_btn;
  lv_obj_t *content;
  lv_obj_t *message;
  lv_obj_t *summary;
  lv_obj_t *installed_list;
  lv_obj_t *selected_detail;
  lv_obj_t *staged_list;
  lv_obj_t *usb_list;
  lv_obj_t *actions;
  lv_timer_t *timer;
  std::map<lv_obj_t *, Control> controls;
  nlohmann::json snapshot;
  std::string selected_app;
  std::string selected_stage;
  std::string selected_usb;
  std::vector<std::string> usb_files;
  std::string armed_action;
  bool open = false;
  bool busy = false;
  bool polling_execution = false;
  uint32_t last_refresh = 0;
  Operation request_kind = Operation::Status;
  std::thread worker;
  std::mutex result_mutex;
  ManagementClient::Response result;
  std::atomic<bool> ready{false};
};

#endif
