#include "timezone_panel.h"

#include "hv/json.hpp"

#include <algorithm>

namespace {
using nlohmann::json;
constexpr uint32_t BG = 0x080B0D;
constexpr uint32_t CARD = 0x11171B;
constexpr uint32_t BORDER = 0x25323A;
constexpr uint32_t CYAN = 0x00E5FF;

lv_obj_t *label(lv_obj_t *parent, const char *text)
{
  auto *item = lv_label_create(parent);
  lv_label_set_text(item, text);
  lv_obj_set_width(item, LV_PCT(100));
  lv_label_set_long_mode(item, LV_LABEL_LONG_MODE_WRAP);
  lv_obj_set_style_text_color(item, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(item, &lv_font_montserrat_14, 0);
  return item;
}

std::string options(const std::vector<std::string> &items)
{
  std::string text;
  for (const auto &item : items) {
    if (!text.empty()) text += '\n';
    text += item;
  }
  return text;
}
} // namespace

TimezonePanel::TimezonePanel()
  : cont(lv_obj_create(lv_screen_active()))
{
  lv_obj_move_to_index(cont, 0);
  lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
  lv_obj_set_scrollable(cont, false);
  lv_obj_set_style_pad_all(cont, 8, 0);
  lv_obj_set_style_border_width(cont, 0, 0);
  lv_obj_set_style_bg_color(cont, lv_color_hex(BG), 0);
  back = lv_button_create(cont);
  refresh = lv_button_create(cont);
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
  lv_label_set_text(title, "Timezone");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 7);

  auto *content = lv_obj_create(cont);
  lv_obj_set_size(content, LV_PCT(100), lv_display_get_vertical_resolution(nullptr) - 58);
  lv_obj_align(content, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_bg_color(content, lv_color_hex(CARD), 0);
  lv_obj_set_style_border_color(content, lv_color_hex(BORDER), 0);
  lv_obj_set_style_border_width(content, 1, 0);
  lv_obj_set_style_radius(content, 8, 0);
  lv_obj_set_style_pad_all(content, 9, 0);
  lv_obj_set_style_pad_row(content, 8, 0);
  lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
  state = label(content, "");
  label(content, "Region");
  region = lv_dropdown_create(content);
  label(content, "Location / timezone");
  place = lv_dropdown_create(content);
  save = lv_button_create(content);
  auto *save_text = lv_label_create(save);
  lv_label_set_text(save_text, "Save timezone");
  lv_obj_center(save_text);
  for (auto *item : {region, place, save}) {
    lv_obj_set_width(item, LV_PCT(100));
    lv_obj_set_style_bg_color(item, lv_color_hex(BG), 0);
    lv_obj_set_style_text_color(item, lv_color_hex(CYAN), 0);
    lv_obj_set_style_border_color(item, lv_color_hex(BORDER), 0);
    lv_obj_set_style_border_width(item, 1, 0);
    lv_obj_set_style_shadow_width(item, 0, 0);
    lv_obj_add_event_cb(item, event_callback,
        item == save ? LV_EVENT_CLICKED : LV_EVENT_VALUE_CHANGED, this);
  }
  for (auto *dropdown : {region, place}) {
    lv_dropdown_clear_options(dropdown);
    auto *list = lv_dropdown_get_list(dropdown);
    lv_obj_set_style_bg_color(list, lv_color_hex(CARD), 0);
    lv_obj_set_style_text_color(list, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_color(list, lv_color_hex(BORDER), 0);
    lv_obj_set_style_text_font(list, &lv_font_montserrat_14, 0);
    lv_obj_set_style_max_height(list, 220, 0);
  }
  message = label(content, "Open this page to load timezone settings.");
  timer = lv_timer_create(timer_callback, 100, this);
  controls();
}

TimezonePanel::~TimezonePanel()
{
  lv_timer_delete(timer);
  if (worker.joinable()) worker.join();
  lv_obj_delete(cont);
}

void TimezonePanel::foreground()
{
  lv_obj_move_to_index(cont, -1);
  start(false);
}

void TimezonePanel::controls()
{
  for (auto *item : {region, place, save}) {
    if (busy || zones.empty()) lv_obj_add_state(item, LV_STATE_DISABLED);
    else lv_obj_remove_state(item, LV_STATE_DISABLED);
  }
  if (busy) lv_obj_add_state(refresh, LV_STATE_DISABLED);
  else lv_obj_remove_state(refresh, LV_STATE_DISABLED);
}

void TimezonePanel::places(const std::string &selected)
{
  locations.clear();
  std::vector<std::string> names;
  const auto index = lv_dropdown_get_selected(region);
  if (index >= regions.size()) return;
  const auto prefix = regions[index] + "/";
  for (const auto &zone : zones) {
    if (zone.compare(0, prefix.size(), prefix) == 0) {
      locations.push_back(zone);
      names.push_back(zone.substr(prefix.size()));
    }
  }
  lv_dropdown_set_options(place, options(names).c_str());
  const auto found = std::find(locations.begin(), locations.end(), selected);
  if (found != locations.end()) lv_dropdown_set_selected(place, found - locations.begin());
}

void TimezonePanel::start(bool write)
{
  if (busy) return;
  std::string body;
  if (write) {
    const auto index = lv_dropdown_get_selected(place);
    if (index >= locations.size()) return;
    body = json({{"timezone", locations[index]}}).dump();
  }
  if (worker.joinable()) worker.join();
  busy = true;
  saving = write;
  controls();
  lv_label_set_text(message, write ? "Saving timezone..." : "Loading timezone settings...");
  worker = std::thread([this, write, body]() {
    auto reply = write ? ManagementClient::timezone_save(body) : ManagementClient::timezone_status();
    {
      std::lock_guard<std::mutex> guard(result_mutex);
      result = std::move(reply);
    }
    ready.store(true, std::memory_order_release);
  });
}

void TimezonePanel::poll()
{
  if (!ready.exchange(false, std::memory_order_acq_rel)) return;
  if (worker.joinable()) worker.join();
  ManagementClient::Response reply;
  {
    std::lock_guard<std::mutex> guard(result_mutex);
    reply = std::move(result);
  }
  busy = false;
  try {
    if (!reply.ok) {
      std::string error = reply.status == 404 ? "This Core version does not support timezone settings." : reply.error;
      const auto failure = json::parse(reply.body, nullptr, false);
      if (reply.status != 404 && failure.is_object() && failure.contains("error") &&
          failure["error"].is_object() && failure["error"].contains("message") && failure["error"]["message"].is_string())
        error = failure["error"]["message"].get<std::string>();
      lv_label_set_text(message, error.c_str());
      if (!saving || reply.status == 404 || reply.status == 401 || reply.status == 403) {
        zones.clear();
        lv_label_set_text(state, "");
        lv_dropdown_clear_options(region);
        lv_dropdown_clear_options(place);
      }
      controls();
      return;
    }
    const auto response = json::parse(reply.body);
    if (response.at("api_version") != 1 || response.at("ok") != true ||
        response.at("operation") != "system-timezone") throw json::type_error::create(302, "Invalid timezone response", &response);
    const auto &data = response.at("timezone");
    const std::string selected = data.at("configured").get<std::string>();
    const bool reboot = data.at("reboot_required").get<bool>();
    const std::string active = data.at("active").is_null() ? "Unavailable" : data.at("active").get<std::string>();
    if (!saving) {
      zones = data.at("available").get<std::vector<std::string>>();
      regions.clear();
      for (const auto &zone : zones) {
        const auto slash = zone.find('/');
        if (slash == std::string::npos || slash == 0 || slash + 1 == zone.size() || zone.find('\n') != std::string::npos)
          throw json::type_error::create(302, "Invalid timezone catalog", &data);
        const auto name = zone.substr(0, slash);
        if (std::find(regions.begin(), regions.end(), name) == regions.end()) regions.push_back(name);
      }
      if (std::find(zones.begin(), zones.end(), selected) == zones.end())
        throw json::type_error::create(302, "Configured timezone absent from catalog", &data);
      lv_dropdown_set_options(region, options(regions).c_str());
      const auto name = selected.substr(0, selected.find('/'));
      lv_dropdown_set_selected(region, std::find(regions.begin(), regions.end(), name) - regions.begin());
      places(selected);
    }
    const auto summary = "Active: " + active + "\nConfigured: " + selected + "\nRestart required: " + (reboot ? "Yes" : "No");
    lv_label_set_text(state, summary.c_str());
    lv_label_set_text(message, saving
      ? "Timezone saved. It takes effect after a restart.\n\nRestart later, when the printer is idle. Core has no safe general restart API yet."
      : "Changes take effect after a restart. UTC and NTP synchronization are unchanged.");
  } catch (const json::exception &) {
    zones.clear();
    lv_label_set_text(state, "");
    lv_label_set_text(message, "Core returned invalid timezone settings. Refresh to retry.");
  }
  controls();
}

void TimezonePanel::event_callback(lv_event_t *event)
{
  auto *self = static_cast<TimezonePanel *>(lv_event_get_user_data(event));
  auto *target = lv_event_get_current_target_obj(event);
  if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
    if (target == self->back) lv_obj_move_to_index(self->cont, 0);
    else if (target == self->refresh) self->start(false);
    else if (target == self->save) self->start(true);
  } else if (lv_event_get_code(event) == LV_EVENT_VALUE_CHANGED && target == self->region) {
    self->places();
  }
}

void TimezonePanel::timer_callback(lv_timer_t *timer)
{
  static_cast<TimezonePanel *>(lv_timer_get_user_data(timer))->poll();
}
