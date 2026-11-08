#include "backup_panel.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace {
using nlohmann::json;
constexpr uint32_t BG = 0x080B0D, CARD = 0x11171B, BORDER = 0x25323A;
constexpr uint32_t CYAN = 0x00E5FF, WHITE = 0xFFFFFF;

lv_obj_t *label(lv_obj_t *parent, const char *text)
{
  auto *item = lv_label_create(parent);
  lv_obj_set_width(item, LV_PCT(100));
  lv_label_set_long_mode(item, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_color(item, lv_color_hex(WHITE), 0);
  lv_obj_set_style_text_font(item, &lv_font_montserrat_12, 0);
  lv_label_set_text(item, text);
  return item;
}
void enabled(lv_obj_t *item, bool value)
{
  if (value) lv_obj_clear_state(item, LV_STATE_DISABLED);
  else lv_obj_add_state(item, LV_STATE_DISABLED);
}
} // namespace

BackupPanel::BackupPanel()
  : cont(lv_obj_create(lv_scr_act()))
{
  lv_obj_move_background(cont);
  lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
  lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(cont, lv_color_hex(BG), 0);
  lv_obj_set_style_border_width(cont, 0, 0);
  lv_obj_set_style_pad_all(cont, 8, 0);
  back = lv_btn_create(cont);
  refresh = lv_btn_create(cont);
  for (auto *button : {back, refresh}) {
    lv_obj_set_size(button, 40, 34);
    lv_obj_set_style_bg_color(button, lv_color_hex(CARD), 0);
    lv_obj_set_style_border_color(button, lv_color_hex(BORDER), 0);
    lv_obj_set_style_border_width(button, 1, 0);
    lv_obj_set_style_shadow_width(button, 0, 0);
    lv_obj_add_event_cb(button, event_callback, LV_EVENT_CLICKED, this);
  }
  lv_obj_align(back, LV_ALIGN_TOP_LEFT, 0, 3);
  lv_obj_align(refresh, LV_ALIGN_TOP_RIGHT, 0, 3);
  auto *back_icon = lv_label_create(back);
  lv_label_set_text(back_icon, LV_SYMBOL_LEFT);
  lv_obj_center(back_icon);
  auto *refresh_icon = lv_label_create(refresh);
  lv_label_set_text(refresh_icon, LV_SYMBOL_REFRESH);
  lv_obj_set_style_text_color(refresh_icon, lv_color_hex(CYAN), 0);
  lv_obj_center(refresh_icon);
  auto *title = lv_label_create(cont);
  lv_label_set_text(title, "Backups");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(WHITE), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 7);
  auto *content = lv_obj_create(cont);
  lv_obj_set_size(content, LV_PCT(100), lv_disp_get_ver_res(nullptr) - 58);
  lv_obj_align(content, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_bg_color(content, lv_color_hex(CARD), 0);
  lv_obj_set_style_border_color(content, lv_color_hex(BORDER), 0);
  lv_obj_set_style_border_width(content, 1, 0);
  lv_obj_set_style_pad_all(content, 9, 0);
  lv_obj_set_style_pad_row(content, 8, 0);
  lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
  label(content, "Storage role");
  role = lv_dropdown_create(content);
  lv_dropdown_set_options(role, "HOME\nSYS");
  label(content, "Destination");
  target = lv_dropdown_create(content);
  create = lv_btn_create(content);
  auto *create_label = lv_label_create(create);
  lv_label_set_text(create_label, "Create backup");
  lv_obj_center(create_label);
  for (auto *item : {role, target, create}) {
    lv_obj_set_width(item, LV_PCT(100));
    lv_obj_set_style_bg_color(item, lv_color_hex(BG), 0);
    lv_obj_set_style_text_color(item, lv_color_hex(CYAN), 0);
    lv_obj_set_style_border_color(item, lv_color_hex(BORDER), 0);
    lv_obj_set_style_border_width(item, 1, 0);
    lv_obj_set_style_shadow_width(item, 0, 0);
    lv_obj_add_event_cb(item, event_callback,
        item == create ? LV_EVENT_CLICKED : LV_EVENT_VALUE_CHANGED, this);
  }
  message = label(content, "Loading backup destinations...");
  timer = lv_timer_create(timer_callback, 120, this);
  render();
}

BackupPanel::~BackupPanel()
{
  lv_timer_del(timer);
  if (worker.joinable()) worker.join();
  lv_obj_del(cont);
}

void BackupPanel::foreground()
{
  open = true;
  lv_obj_move_foreground(cont);
  start(false);
}

bool BackupPanel::running() const
{
  return snapshot.is_object() && snapshot.contains("execution") &&
      snapshot["execution"].is_object() && snapshot["execution"].value("state", "") == "running";
}

void BackupPanel::targets()
{
  destinations.clear();
  const std::string selected_role = lv_dropdown_get_selected(role) == 0 ? "home" : "sys";
  if (snapshot.is_object() && snapshot.contains("backup_offer")) {
    const auto offer = snapshot["backup_offer"].value(selected_role, json::object());
    const auto offered = offer.value("targets", json::array());
    // Frontend preference only. The server remains authoritative.
    for (const std::string name : {"usb", "home"}) {
      if ((name != "home" || selected_role == "sys") &&
          std::find(offered.begin(), offered.end(), name) != offered.end()) destinations.push_back(name);
    }
  }
  if (std::find(destinations.begin(), destinations.end(), choice) == destinations.end())
    choice = destinations.empty() ? "" : destinations.front();
  std::string options;
  unsigned selected = 0;
  for (size_t i = 0; i < destinations.size(); ++i) {
    if (i) options += '\n';
    options += destinations[i] == "usb" ? "USB storage" : "HOME storage";
    if (destinations[i] == choice) selected = static_cast<unsigned>(i);
  }
  lv_dropdown_set_options(target, options.empty() ? "No destination available" : options.c_str());
  lv_dropdown_set_selected(target, selected);
}

void BackupPanel::render()
{
  targets();
  const bool blocked = busy || running() || uncertain;
  enabled(role, !blocked);
  enabled(target, !blocked && !destinations.empty());
  enabled(create, !blocked && !choice.empty());
  enabled(refresh, !busy);
  std::string text = notice;
  if (text.empty() && snapshot.is_object()) {
    const auto job = snapshot.value("execution", json(nullptr));
    if (job.is_object()) {
      const auto state = job.value("state", "");
      if (state == "running") text = "Creating and verifying backup...";
      else if (state == "succeeded") {
        const auto result = job.value("result", json::object()).value("backup_result", json::object());
        const auto archive = result.value(job.value("role", ""), json::object());
        text = result.value("complete", false) && archive.value("verified", false)
            ? "Backup complete and verified.\n" + archive.value("archive", "")
            : "Backup completion was not verified.";
      } else if (state == "failed") text = job.value("error", json::object()).value("message", "Backup failed");
      else if (state == "discarded") text = "Last temporary backup discarded.";
    }
    if (text.empty()) text = choice.empty() ? "No destination available for this role. Connect suitable USB storage or select SYS." : "Select a role and destination.";
  }
  lv_label_set_text(message, text.c_str());
}

void BackupPanel::start(bool submit)
{
  if (busy || (submit && (choice.empty() || running() || uncertain))) return;
  if (worker.joinable()) worker.join();
  creating = submit;
  busy = true;
  refresh_needed = false;
  const std::string selected_role = lv_dropdown_get_selected(role) == 0 ? "home" : "sys";
  const std::string body = json{{"role", selected_role}, {"target", choice}}.dump();
  if (submit) notice = "Requesting backup. Please wait...";
  worker = std::thread([this, submit, body] {
    auto answer = submit ? ManagementClient::backup_create(body) : ManagementClient::backup_status();
    {
      std::lock_guard<std::mutex> guard(result_mutex);
      result = std::move(answer);
    }
    ready.store(true, std::memory_order_release);
  });
  render();
}

void BackupPanel::poll()
{
  if (ready.exchange(false, std::memory_order_acquire)) {
    if (worker.joinable()) worker.join();
    ManagementClient::Response answer;
    { std::lock_guard<std::mutex> guard(result_mutex); answer = std::move(result); }
    busy = false;
    last_refresh = lv_tick_get();
    try {
      const auto body = json::parse(answer.body);
      if (!answer.ok || !body.value("ok", false) || body.value("api_version", 0) != 1)
        throw std::runtime_error(body.value("error", json::object()).value("message", answer.error));
      if (creating) {
        if (body.value("operation", "") != "backup-accepted" || !body.at("execution").is_object())
          throw std::runtime_error("Invalid backup receipt");
        notice.clear();
        refresh_needed = true;
      } else {
        if (body.value("operation", "") != "backup-status" || !body.at("backup").is_object())
          throw std::runtime_error("Invalid backup status");
        snapshot = body.at("backup");
      }
    } catch (const std::exception &error) {
      if (!creating) snapshot = json(nullptr);
      notice = answer.error.empty() ? error.what() : answer.error;
      if (creating) {
        uncertain = true;
        notice += "\nRefresh status before retrying; the request may have been accepted.";
      }
    }
    try { render(); }
    catch (const std::exception &) {
      snapshot = json(nullptr);
      notice = "Management returned invalid backup state.";
      render();
    }
  }
  if (open && !busy && (refresh_needed || (running() && lv_tick_elaps(last_refresh) > 1500))) start(false);
}

void BackupPanel::event_callback(lv_event_t *event)
{
  auto *self = static_cast<BackupPanel *>(event->user_data);
  auto *item = lv_event_get_current_target(event);
  if (item == self->back) {
    self->open = false;
    lv_obj_move_background(self->cont);
  } else if (item == self->refresh) {
    self->uncertain = false;
    self->notice.clear();
    self->start(false);
  } else if (item == self->create) self->start(true);
  else if (item == self->role) {
    self->choice.clear();
    self->render();
  } else if (item == self->target) {
    const auto index = lv_dropdown_get_selected(item);
    self->choice = index < self->destinations.size() ? self->destinations[index] : "";
  }
}

void BackupPanel::timer_callback(lv_timer_t *timer)
{
  static_cast<BackupPanel *>(timer->user_data)->poll();
}
