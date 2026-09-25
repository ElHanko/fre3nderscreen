#include "power_panel.h"
#include "spdlog/spdlog.h"

namespace {

constexpr uint32_t COLOR_BG = 0x080B0D;
constexpr uint32_t COLOR_CARD = 0x11171B;
constexpr uint32_t COLOR_CARD_PRESSED = 0x18242A;
constexpr uint32_t COLOR_BORDER = 0x25323A;
constexpr uint32_t COLOR_ACCENT = 0x00E5FF;
constexpr uint32_t COLOR_TEXT = 0xFFFFFF;
constexpr uint32_t COLOR_MUTED = 0xB8C0C5;

void style_transparent(lv_obj_t *obj)
{
  lv_obj_set_style_pad_all(obj, 0, 0);
  lv_obj_set_style_border_width(obj, 0, 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
}

void style_card(lv_obj_t *obj)
{
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(obj, lv_color_hex(COLOR_CARD), 0);
  lv_obj_set_style_border_width(obj, 1, 0);
  lv_obj_set_style_border_color(obj, lv_color_hex(COLOR_BORDER), 0);
  lv_obj_set_style_radius(obj, 8, 0);
  lv_obj_set_style_shadow_width(obj, 0, 0);
}

void style_button(lv_obj_t *button)
{
  lv_obj_clear_flag(button, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(button, lv_color_hex(COLOR_CARD), 0);
  lv_obj_set_style_border_width(button, 1, 0);
  lv_obj_set_style_border_color(button, lv_color_hex(COLOR_BORDER), 0);
  lv_obj_set_style_radius(button, 8, 0);
  lv_obj_set_style_shadow_width(button, 0, 0);
  lv_obj_set_style_bg_color(
      button,
      lv_color_hex(COLOR_CARD_PRESSED),
      LV_STATE_PRESSED);
  lv_obj_set_style_border_color(
      button,
      lv_color_hex(COLOR_ACCENT),
      LV_STATE_PRESSED);
}

void style_switch(lv_obj_t *sw)
{
  lv_obj_set_style_bg_color(
      sw,
      lv_color_hex(COLOR_ACCENT),
      LV_PART_INDICATOR | LV_STATE_CHECKED);
}

} // namespace

PowerPanel::PowerPanel(KWebSocketClient &websocket_client,
                       std::mutex &lock)
  : ws(websocket_client)
  , lv_lock(lock)
  , cont(lv_obj_create(lv_scr_act()))
  , header_cont(lv_obj_create(cont))
  , back_btn(lv_btn_create(header_cont))
  , title_label(lv_label_create(header_cont))
  , devices_cont(lv_obj_create(cont))
  , empty_label(lv_label_create(devices_cont))
{
  lv_obj_move_background(cont);
  lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
  lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(cont, 8, 0);
  lv_obj_set_style_pad_row(cont, 6, 0);
  lv_obj_set_style_border_width(cont, 0, 0);
  lv_obj_set_style_bg_color(cont, lv_color_hex(COLOR_BG), 0);

  static lv_coord_t rows[] = {
    42,
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  static lv_coord_t cols[] = {
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  lv_obj_set_grid_dsc_array(cont, cols, rows);

  lv_obj_set_grid_cell(
      header_cont,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 0, 1);
  style_transparent(header_cont);
  lv_obj_clear_flag(header_cont, LV_OBJ_FLAG_SCROLLABLE);

  style_button(back_btn);
  lv_obj_set_size(back_btn, 40, 34);
  lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 0, 0);
  lv_obj_add_event_cb(
      back_btn,
      &PowerPanel::_handle_callback,
      LV_EVENT_CLICKED,
      this);

  lv_obj_t *back_label = lv_label_create(back_btn);
  lv_label_set_text(back_label, LV_SYMBOL_LEFT);
  lv_obj_set_style_text_font(back_label, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(back_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(back_label);

  lv_label_set_text(title_label, "Power");
  lv_obj_set_style_text_font(title_label, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(title_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(title_label);

  lv_obj_set_grid_cell(
      devices_cont,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 1, 1);
  style_transparent(devices_cont);
  lv_obj_set_style_pad_row(devices_cont, 8, 0);
  lv_obj_set_style_pad_right(devices_cont, 4, 0);
  lv_obj_set_flex_flow(devices_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_scroll_dir(devices_cont, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(devices_cont, LV_SCROLLBAR_MODE_AUTO);

  lv_label_set_text(
      empty_label,
      "No power devices configured.");
  lv_obj_set_width(empty_label, LV_PCT(100));
  lv_obj_set_style_text_align(empty_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(empty_label, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(empty_label, lv_color_hex(COLOR_MUTED), 0);
}

PowerPanel::~PowerPanel()
{
  if (cont != nullptr) {
    lv_obj_del(cont);
    cont = nullptr;
  }

  devices.clear();
}

void PowerPanel::create_device(json &j)
{
  std::string name = j["device"].template get<std::string>();

  lv_obj_t *power_device_toggle;
  auto entry = devices.find(name);

  if (entry == devices.end()) {
    lv_obj_add_flag(empty_label, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *device_cont = lv_obj_create(devices_cont);
    lv_obj_set_width(device_cont, LV_PCT(100));
    lv_obj_set_height(device_cont, 62);
    style_card(device_cont);
    lv_obj_set_style_pad_all(device_cont, 10, 0);

    lv_obj_t *label = lv_label_create(device_cont);
    lv_label_set_text(label, name.c_str());
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(COLOR_TEXT), 0);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);

    power_device_toggle = lv_switch_create(device_cont);
    style_switch(power_device_toggle);
    lv_obj_align(power_device_toggle, LV_ALIGN_RIGHT_MID, 0, 0);

    lv_obj_add_event_cb(
        power_device_toggle,
        &PowerPanel::_handle_callback,
        LV_EVENT_VALUE_CHANGED,
        this);

    devices.insert({name, power_device_toggle});
  } else {
    power_device_toggle = entry->second;
  }

  std::string status = j["status"].template get<std::string>();
  spdlog::debug(
      "Fetched initial status for power device {}: {}",
      name,
      status);

  if (status == "on") {
    lv_obj_add_state(power_device_toggle, LV_STATE_CHECKED);
  } else {
    lv_obj_clear_state(power_device_toggle, LV_STATE_CHECKED);
  }
}

void PowerPanel::create_devices(json &j)
{
  std::lock_guard<std::mutex> lock(lv_lock);

  if (!j.contains("result")) {
    return;
  }

  json result = j["result"];
  if (!result.contains("devices")) {
    return;
  }

  json power_devices = result["devices"];
  for (auto &device : power_devices.items()) {
    create_device(device.value());
  }
}

void PowerPanel::handle_device_callback(json &j)
{
  std::lock_guard<std::mutex> lock(lv_lock);

  if (!j.contains("result")) {
    return;
  }

  json result = j["result"];
  for (auto &device : result.items()) {
    auto entry = devices.find(device.key());
    if (entry == devices.end()) {
      continue;
    }

    std::string new_status = device.value();

    spdlog::debug(
        "Fetched new status for power device {}: {}",
        entry->first,
        new_status);

    if (new_status == "on") {
      lv_obj_add_state(entry->second, LV_STATE_CHECKED);
    } else {
      lv_obj_clear_state(entry->second, LV_STATE_CHECKED);
    }
  }
}

void PowerPanel::foreground()
{
  lv_obj_move_foreground(cont);

  json params;
  for (auto &device : devices) {
    params[device.first] = 0;
  }

  ws.send_jsonrpc(
      "machine.device_power.status",
      params,
      [this](json &j) {
        this->handle_device_callback(j);
      });
}

void PowerPanel::handle_callback(lv_event_t *event)
{
  const lv_event_code_t code = lv_event_get_code(event);

  if (code == LV_EVENT_CLICKED) {
    lv_obj_t *button = lv_event_get_current_target(event);
    if (button == back_btn) {
      lv_obj_move_background(cont);
    }
    return;
  }

  if (code != LV_EVENT_VALUE_CHANGED) {
    return;
  }

  lv_obj_t *obj = lv_event_get_target(event);
  for (auto &device : devices) {
    if (obj != device.second) {
      continue;
    }

    bool turn_on =
        lv_obj_has_state(device.second, LV_STATE_CHECKED);
    std::string status = turn_on ? "on" : "off";

    spdlog::debug(
        "Turning power device {} {}",
        device.first,
        status);

    json params;
    params["device"] = device.first;
    params["action"] = status;

    ws.send_jsonrpc(
        "machine.device_power.post_device",
        params,
        [this](json &j) {
          this->handle_device_callback(j);
        });
    break;
  }
}
