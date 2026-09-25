#include "led_panel.h"

#include "state.h"
#include "utils.h"
#include "spdlog/fmt/fmt.h"

namespace {

constexpr uint32_t COLOR_BG = 0x080B0D;
constexpr uint32_t COLOR_CARD = 0x11171B;
constexpr uint32_t COLOR_CARD_PRESSED = 0x18242A;
constexpr uint32_t COLOR_BORDER = 0x25323A;
constexpr uint32_t COLOR_ACCENT = 0x00E5FF;
constexpr uint32_t COLOR_TEXT = 0xFFFFFF;
constexpr uint32_t COLOR_MUTED = 0xB8C0C5;

void style_button(lv_obj_t *button)
{
  lv_obj_clear_flag(button, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(button, lv_color_hex(COLOR_CARD), 0);
  lv_obj_set_style_border_width(button, 1, 0);
  lv_obj_set_style_border_color(button, lv_color_hex(COLOR_BORDER), 0);
  lv_obj_set_style_radius(button, 8, 0);
  lv_obj_set_style_shadow_width(button, 0, 0);
  lv_obj_set_style_bg_color(button, lv_color_hex(COLOR_CARD_PRESSED), LV_STATE_PRESSED);
  lv_obj_set_style_border_color(button, lv_color_hex(COLOR_ACCENT), LV_STATE_PRESSED);
}

lv_obj_t *create_text_button(lv_obj_t *parent,
                             const char *text,
                             lv_event_cb_t callback,
                             void *user_data)
{
  lv_obj_t *button = lv_btn_create(parent);
  style_button(button);
  lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, user_data);

  lv_obj_t *label = lv_label_create(button);
  lv_label_set_text(label, text);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(label);

  return button;
}

void style_transparent_container(lv_obj_t *cont)
{
  lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(cont, 0, 0);
  lv_obj_set_style_border_width(cont, 0, 0);
  lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
}

} // namespace

LedPanel::LedPanel(KWebSocketClient &websocket_client, std::mutex &lock)
  : NotifyConsumer(lock)
  , ws(websocket_client)
  , ledpanel_cont(lv_obj_create(lv_scr_act()))
  , header_cont(lv_obj_create(ledpanel_cont))
  , back_btn(lv_btn_create(header_cont))
  , title_label(lv_label_create(header_cont))
  , leds_cont(lv_obj_create(ledpanel_cont))
{
  lv_obj_move_background(ledpanel_cont);
  lv_obj_set_size(ledpanel_cont, LV_PCT(100), LV_PCT(100));
  lv_obj_clear_flag(ledpanel_cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(ledpanel_cont, 8, 0);
  lv_obj_set_style_pad_row(ledpanel_cont, 8, 0);
  lv_obj_set_style_border_width(ledpanel_cont, 0, 0);
  lv_obj_set_style_bg_color(ledpanel_cont, lv_color_hex(COLOR_BG), 0);

  static lv_coord_t rows[] = {
    42,
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  static lv_coord_t cols[] = {
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  lv_obj_set_grid_dsc_array(ledpanel_cont, cols, rows);

  lv_obj_set_grid_cell(
      header_cont,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 0, 1);
  style_transparent_container(header_cont);

  style_button(back_btn);
  lv_obj_set_size(back_btn, 40, 34);
  lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 0, 0);
  lv_obj_add_event_cb(back_btn, &LedPanel::_handle_callback, LV_EVENT_CLICKED, this);

  lv_obj_t *back_label = lv_label_create(back_btn);
  lv_label_set_text(back_label, LV_SYMBOL_LEFT);
  lv_obj_set_style_text_font(back_label, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(back_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(back_label);

  lv_label_set_text(title_label, "LED");
  lv_obj_set_style_text_font(title_label, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(title_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(title_label);

  lv_obj_set_grid_cell(
      leds_cont,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 1, 1);
  lv_obj_set_style_pad_all(leds_cont, 0, 0);
  lv_obj_set_style_pad_row(leds_cont, 8, 0);
  lv_obj_set_style_border_width(leds_cont, 0, 0);
  lv_obj_set_style_bg_opa(leds_cont, LV_OPA_TRANSP, 0);
  lv_obj_set_flex_flow(leds_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_scroll_dir(leds_cont, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(leds_cont, LV_SCROLLBAR_MODE_AUTO);

  ws.register_notify_update(this);
}

LedPanel::~LedPanel()
{
  ws.unregister_notify_update(this);

  if (ledpanel_cont != nullptr) {
    lv_obj_del(ledpanel_cont);
    ledpanel_cont = nullptr;
  }
}

LedPanel::LedControl LedPanel::create_led_control(
    const std::string &display_name,
    lv_event_cb_t callback)
{
  LedControl control{};

  control.card = lv_obj_create(leds_cont);
  lv_obj_set_size(control.card, LV_PCT(100), 116);
  lv_obj_clear_flag(control.card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(control.card, 8, 0);
  lv_obj_set_style_pad_row(control.card, 6, 0);
  lv_obj_set_style_pad_column(control.card, 8, 0);
  lv_obj_set_style_radius(control.card, 8, 0);
  lv_obj_set_style_border_width(control.card, 1, 0);
  lv_obj_set_style_border_color(control.card, lv_color_hex(COLOR_BORDER), 0);
  lv_obj_set_style_bg_color(control.card, lv_color_hex(COLOR_CARD), 0);

  static lv_coord_t rows[] = {
    22,
    28,
    38,
    LV_GRID_TEMPLATE_LAST
  };
  static lv_coord_t cols[] = {
    LV_GRID_FR(1),
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  lv_obj_set_grid_dsc_array(control.card, cols, rows);

  lv_obj_t *name_label = lv_label_create(control.card);
  lv_label_set_text(name_label, display_name.c_str());
  lv_obj_set_style_text_font(name_label, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(name_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_set_grid_cell(
      name_label,
      LV_GRID_ALIGN_START, 0, 1,
      LV_GRID_ALIGN_CENTER, 0, 1);

  control.value_label = lv_label_create(control.card);
  lv_label_set_text(control.value_label, "0%");
  lv_obj_set_style_text_font(control.value_label, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(control.value_label, lv_color_hex(COLOR_MUTED), 0);
  lv_obj_set_grid_cell(
      control.value_label,
      LV_GRID_ALIGN_END, 1, 1,
      LV_GRID_ALIGN_CENTER, 0, 1);

  control.slider = lv_slider_create(control.card);
  lv_slider_set_range(control.slider, 0, 100);
  lv_obj_set_grid_cell(
      control.slider,
      LV_GRID_ALIGN_STRETCH, 0, 2,
      LV_GRID_ALIGN_CENTER, 1, 1);
  lv_obj_set_style_bg_color(control.slider, lv_color_hex(COLOR_BORDER), LV_PART_MAIN);
  lv_obj_set_style_bg_color(control.slider, lv_color_hex(COLOR_ACCENT), LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(control.slider, lv_color_hex(COLOR_TEXT), LV_PART_KNOB);
  lv_obj_add_event_cb(control.slider, callback, LV_EVENT_VALUE_CHANGED, this);
  lv_obj_add_event_cb(control.slider, callback, LV_EVENT_RELEASED, this);

  control.off_btn = create_text_button(control.card, "Off", callback, this);
  control.max_btn = create_text_button(control.card, "Max", callback, this);

  lv_obj_set_grid_cell(
      control.off_btn,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 2, 1);
  lv_obj_set_grid_cell(
      control.max_btn,
      LV_GRID_ALIGN_STRETCH, 1, 1,
      LV_GRID_ALIGN_STRETCH, 2, 1);

  return control;
}

void LedPanel::update_value(LedControl &control, int value)
{
  const int clamped = value < 0 ? 0 : (value > 100 ? 100 : value);
  lv_slider_set_value(control.slider, clamped, LV_ANIM_OFF);
  lv_label_set_text(control.value_label, fmt::format("{}%", clamped).c_str());
}

void LedPanel::sync_value_event(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED) {
    return;
  }

  lv_obj_t *object = lv_event_get_target(event);
  for (auto &entry : leds) {
    if (object == entry.second.slider) {
      update_value(entry.second, lv_slider_get_value(object));
      return;
    }
  }
}

void LedPanel::consume(json &j)
{
  std::lock_guard<std::mutex> lock(lv_lock);

  for (auto &entry : leds) {
    auto value = j[json::json_pointer(
        fmt::format("/params/0/{}/value", entry.first))];

    if (!value.is_null()) {
      update_value(
          entry.second,
          static_cast<int>(value.template get<double>() * 100));
    }

    value = j[json::json_pointer(
        fmt::format("/params/0/{}/color_data", entry.first))];

    if (!value.is_null() && !value.empty()) {
      const auto color = value.at(0);
      if (color.size() == 4) {
        update_value(
            entry.second,
            static_cast<int>(color.at(3).template get<double>() * 100));
      }
    }
  }
}

void LedPanel::init(json &display_leds)
{
  std::lock_guard<std::mutex> lock(lv_lock);

  lv_obj_clean(leds_cont);
  leds.clear();

  for (auto &led : display_leds.items()) {
    const std::string key = led.key();
    const std::string display_name =
        led.value()["display_name"].template get<std::string>();

    lv_event_cb_t callback = &LedPanel::_handle_led_update;

    if (key.rfind("output_pin ", 0) != 0) {
      callback = &LedPanel::_handle_led_update_generic;
    }

    leds.emplace(key, create_led_control(display_name, callback));
  }
}

lv_obj_t *LedPanel::get_container()
{
  return ledpanel_cont;
}

void LedPanel::foreground()
{
  for (auto &entry : leds) {
    auto value = State::get_instance()->get_data(
        json::json_pointer(
            fmt::format("/printer_state/{}/value", entry.first)));

    if (!value.is_null()) {
      update_value(
          entry.second,
          static_cast<int>(value.template get<double>() * 100));
    }

    value = State::get_instance()->get_data(
        json::json_pointer(
            fmt::format("/printer_state/{}/color_data", entry.first)));

    if (!value.is_null() && !value.empty()) {
      const auto color = value.at(0);
      if (color.size() == 4) {
        update_value(
            entry.second,
            static_cast<int>(color.at(3).template get<double>() * 100));
      }
    }
  }

  lv_obj_move_foreground(ledpanel_cont);
}

void LedPanel::handle_callback(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
    return;
  }

  if (lv_event_get_current_target(event) == back_btn) {
    lv_obj_move_background(ledpanel_cont);
  }
}

void LedPanel::handle_led_update(lv_event_t *event)
{
  sync_value_event(event);

  const lv_event_code_t code = lv_event_get_code(event);
  if (code != LV_EVENT_RELEASED && code != LV_EVENT_CLICKED) {
    return;
  }

  lv_obj_t *object = lv_event_get_target(event);

  for (auto &entry : leds) {
    LedControl &control = entry.second;
    const std::string led_name = KUtils::get_obj_name(entry.first);

    if (code == LV_EVENT_RELEASED && object == control.slider) {
      const double value =
          static_cast<double>(lv_slider_get_value(object)) / 100.0;
      ws.gcode_script(fmt::format(
          "SET_PIN PIN={} VALUE={}",
          led_name,
          value));
      return;
    }

    object = lv_event_get_current_target(event);

    if (code == LV_EVENT_CLICKED && object == control.off_btn) {
      ws.gcode_script(fmt::format(
          "SET_PIN PIN={} VALUE=0",
          led_name));
      update_value(control, 0);
      return;
    }

    if (code == LV_EVENT_CLICKED && object == control.max_btn) {
      ws.gcode_script(fmt::format(
          "SET_PIN PIN={} VALUE=1",
          led_name));
      update_value(control, 100);
      return;
    }
  }
}

void LedPanel::handle_led_update_generic(lv_event_t *event)
{
  sync_value_event(event);

  const lv_event_code_t code = lv_event_get_code(event);
  if (code != LV_EVENT_RELEASED && code != LV_EVENT_CLICKED) {
    return;
  }

  lv_obj_t *object = lv_event_get_target(event);

  for (auto &entry : leds) {
    LedControl &control = entry.second;
    const std::string led_name = KUtils::get_obj_name(entry.first);

    if (code == LV_EVENT_RELEASED && object == control.slider) {
      const double value =
          static_cast<double>(lv_slider_get_value(object)) / 100.0;
      ws.gcode_script(fmt::format(
          "SET_LED LED={} WHITE={}",
          led_name,
          value));
      return;
    }

    object = lv_event_get_current_target(event);

    if (code == LV_EVENT_CLICKED && object == control.off_btn) {
      ws.gcode_script(fmt::format(
          "SET_LED LED={} WHITE=0",
          led_name));
      update_value(control, 0);
      return;
    }

    if (code == LV_EVENT_CLICKED && object == control.max_btn) {
      ws.gcode_script(fmt::format(
          "SET_LED LED={} WHITE=1",
          led_name));
      update_value(control, 100);
      return;
    }
  }
}
