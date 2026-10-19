#ifndef FRE3NDER_OTA_PANEL_H
#define FRE3NDER_OTA_PANEL_H

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

// Native view for the existing Fre3nder OTA transaction protocol.
// Core owns signatures, backup selection, slot mapping, writes and boot state.
class OtaPanel {
 public:
  OtaPanel();
  ~OtaPanel();
  void foreground();

 private:
  enum class Action { Back, Refresh, SelectUsb, SelectPackage, Upload,
      Verify, HomeUsb, HomeNone, SysUsb, SysHome, SysNone, Update };
  struct Control { Action action; std::string value; };

  static void event_callback(lv_event_t *event);
  static void timer_callback(lv_timer_t *timer);
  void handle(lv_event_t *event);
  void refresh();
  void start(bool status_request, std::string operation,
      std::function<ManagementClient::Response()> request);
  void poll();
  void render();
  void add_button(lv_obj_t *parent, const std::string &text,
      Action action, const std::string &value = {}, bool dangerous = false);
  void scan_usb();
  void take_action(Action action);
  void submit(const std::string &operation, const nlohmann::json &payload);
  std::string transaction_package() const;
  std::string next_operation() const;
  bool execution_blocks() const;
  bool backup_choice_valid(const std::string &role, const std::string &choice) const;
  void clear_selection();
  void pause_update(const std::string &reason, bool unclear = false);
  void advance_update(const std::string &completed, const nlohmann::json &core_result);

  lv_obj_t *cont;
  lv_obj_t *back_btn;
  lv_obj_t *refresh_btn;
  lv_obj_t *content;
  lv_obj_t *message;
  lv_obj_t *details;
  lv_obj_t *packages;
  lv_obj_t *choices;
  lv_obj_t *actions;
  lv_timer_t *timer;
  std::map<lv_obj_t *, Control> controls;
  nlohmann::json snapshot;
  nlohmann::json preflight;
  nlohmann::json planned;
  std::string selected_usb;
  std::string selected_package;
  std::string home_choice;
  std::string sys_choice;
  std::string armed;
  std::string pending_execution;
  std::string submitted_operation;
  std::vector<std::string> usb_files;
  bool open = false;
  bool busy = false;
  bool uncertain = false;
  bool reboot_sent = false;
  bool check_after_verify = false;
  bool update_active = false;
  std::string update_package;
  std::string update_transaction;
  std::string update_version;
  std::string update_target;
  std::string update_home;
  std::string update_sys;
  uint32_t last_refresh = 0;
  bool status_request = true;
  std::string request_operation;
  std::thread worker;
  std::mutex result_mutex;
  ManagementClient::Response result;
  std::atomic<bool> ready{false};
};

#endif
