#include "limits_panel.h"

#include "state.h"
#include "spdlog/fmt/fmt.h"
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
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(obj, 0, 0);
  lv_obj_set_style_border_width(obj, 0, 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
}

void style_button(lv_obj_t *button)
{
  lv_obj_clear_flag(button, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(button, lv_color_hex(COLOR_CARD_PRESSED), 0);
  lv_obj_set_style_border_width(button, 1, 0);
  lv_obj_set_style_border_color(button, lv_color_hex(COLOR_BORDER), 0);
  lv_obj_set_style_radius(button, 6, 0);
  lv_obj_set_style_shadow_width(button, 0, 0);
  lv_obj_set_style_border_color(
      button,
      lv_color_hex(COLOR_ACCENT),
      LV_STATE_PRESSED);
}

void style_card(lv_obj_t *card)
{
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(card, lv_color_hex(COLOR_CARD), 0);
  lv_obj_set_style_border_width(card, 1, 0);
  lv_obj_set_style_border_color(card, lv_color_hex(COLOR_BORDER), 0);
  lv_obj_set_style_radius(card, 8, 0);
  lv_obj_set_style_shadow_width(card, 0, 0);
  lv_obj_set_style_pad_all(card, 8, 0);
}

void style_slider(lv_obj_t *slider)
{
  lv_obj_set_style_bg_color(
      slider,
      lv_color_hex(COLOR_CARD_PRESSED),
      LV_PART_MAIN);
  lv_obj_set_style_bg_color(
      slider,
      lv_color_hex(COLOR_ACCENT),
      LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(
      slider,
      lv_color_hex(COLOR_ACCENT),
      LV_PART_KNOB);
  lv_obj_set_style_radius(slider, 4, LV_PART_MAIN);
  lv_obj_set_style_radius(slider, 4, LV_PART_INDICATOR);
  lv_obj_set_style_radius(slider, 8, LV_PART_KNOB);
}

lv_obj_t *create_limit_card(lv_obj_t *parent,
                            const char *title,
                            lv_obj_t **value_label,
                            lv_obj_t **slider,
                            lv_obj_t **reset_btn,
                            lv_event_cb_t callback,
                            void *user_data)
{
  lv_obj_t *card = lv_obj_create(parent);
  style_card(card);
  lv_obj_set_width(card, LV_PCT(100));
  lv_obj_set_height(card, 82);

  lv_obj_t *title_label = lv_label_create(card);
  lv_label_set_text(title_label, title);
  lv_obj_set_style_text_font(title_label, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(title_label, lv_color_hex(COLOR_MUTED), 0);
  lv_obj_align(title_label, LV_ALIGN_TOP_LEFT, 0, 0);

  *value_label = lv_label_create(card);
  lv_label_set_text(*value_label, "--");
  lv_obj_set_style_text_font(*value_label, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(*value_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_align(*value_label, LV_ALIGN_TOP_RIGHT, 0, 0);

  lv_obj_t *controls = lv_obj_create(card);
  style_transparent(controls);
  lv_obj_set_size(controls, LV_PCT(100), 38);
  lv_obj_align(controls, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_pad_column(controls, 8, 0);
  lv_obj_set_flex_flow(controls, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(
      controls,
      LV_FLEX_ALIGN_START,
      LV_FLEX_ALIGN_CENTER,
      LV_FLEX_ALIGN_CENTER);

  *reset_btn = lv_btn_create(controls);
  style_button(*reset_btn);
  lv_obj_set_size(*reset_btn, 58, 32);
  lv_obj_add_event_cb(
      *reset_btn,
      callback,
      LV_EVENT_CLICKED,
      user_data);

  lv_obj_t *reset_label = lv_label_create(*reset_btn);
  lv_label_set_text(reset_label, LV_SYMBOL_REFRESH);
  lv_obj_set_style_text_font(reset_label, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(reset_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(reset_label);

  *slider = lv_slider_create(controls);
  lv_obj_set_flex_grow(*slider, 1);
  lv_obj_set_height(*slider, 18);
  style_slider(*slider);
  lv_obj_add_event_cb(
      *slider,
      callback,
      LV_EVENT_VALUE_CHANGED,
      user_data);
  lv_obj_add_event_cb(
      *slider,
      callback,
      LV_EVENT_RELEASED,
      user_data);

  return card;
}

void set_value(lv_obj_t *label,
               lv_obj_t *slider,
               int value,
               const char *unit)
{
  lv_label_set_text(
      label,
      fmt::format("{} {}", value, unit).c_str());
  lv_slider_set_value(slider, value, LV_ANIM_OFF);
}

} // namespace

LimitsPanel::LimitsPanel(KWebSocketClient &client,
                         std::mutex &lock)
  : NotifyConsumer(lock)
  , ws(client)
  , cont(lv_obj_create(lv_scr_act()))
  , header_cont(lv_obj_create(cont))
  , back_btn(lv_btn_create(header_cont))
  , title_label(lv_label_create(header_cont))
  , content_cont(lv_obj_create(cont))
  , velocity_card(nullptr)
  , velocity_value(nullptr)
  , velocity_slider(nullptr)
  , velocity_reset(nullptr)
  , acceleration_card(nullptr)
  , acceleration_value(nullptr)
  , acceleration_slider(nullptr)
  , acceleration_reset(nullptr)
  , accel_to_decel_card(nullptr)
  , accel_to_decel_value(nullptr)
  , accel_to_decel_slider(nullptr)
  , accel_to_decel_reset(nullptr)
  , square_corner_card(nullptr)
  , square_corner_value(nullptr)
  , square_corner_slider(nullptr)
  , square_corner_reset(nullptr)
  , max_velocity_default(1000)
  , max_accel_default(20000)
  , max_accel_to_decel_default(10000)
  , square_corner_default(5)
{
  lv_obj_move_background(cont);
  lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
  lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(cont, 8, 0);
  lv_obj_set_style_pad_row(cont, 8, 0);
  lv_obj_set_style_border_width(cont, 0, 0);
  lv_obj_set_style_bg_color(cont, lv_color_hex(COLOR_BG), 0);

  static lv_coord_t root_rows[] = {
    42,
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  static lv_coord_t root_cols[] = {
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  lv_obj_set_grid_dsc_array(cont, root_cols, root_rows);

  lv_obj_set_grid_cell(
      header_cont,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 0, 1);
  style_transparent(header_cont);

  lv_obj_set_style_bg_color(back_btn, lv_color_hex(COLOR_CARD), 0);
  lv_obj_set_style_border_width(back_btn, 1, 0);
  lv_obj_set_style_border_color(back_btn, lv_color_hex(COLOR_BORDER), 0);
  lv_obj_set_style_radius(back_btn, 8, 0);
  lv_obj_set_style_shadow_width(back_btn, 0, 0);
  lv_obj_set_size(back_btn, 40, 34);
  lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 0, 0);
  lv_obj_add_event_cb(
      back_btn,
      &LimitsPanel::_handle_callback,
      LV_EVENT_CLICKED,
      this);

  lv_obj_t *back_label = lv_label_create(back_btn);
  lv_label_set_text(back_label, LV_SYMBOL_LEFT);
  lv_obj_set_style_text_font(back_label, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(back_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(back_label);

  lv_label_set_text(title_label, "Limits");
  lv_obj_set_style_text_font(title_label, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(title_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(title_label);

  lv_obj_set_grid_cell(
      content_cont,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 1, 1);
  style_transparent(content_cont);
  lv_obj_set_style_pad_row(content_cont, 6, 0);
  lv_obj_set_flex_flow(content_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(
      content_cont,
      LV_FLEX_ALIGN_START,
      LV_FLEX_ALIGN_START,
      LV_FLEX_ALIGN_START);

  velocity_card = create_limit_card(
      content_cont,
      "Velocity",
      &velocity_value,
      &velocity_slider,
      &velocity_reset,
      &LimitsPanel::_handle_callback,
      this);

  acceleration_card = create_limit_card(
      content_cont,
      "Acceleration",
      &acceleration_value,
      &acceleration_slider,
      &acceleration_reset,
      &LimitsPanel::_handle_callback,
      this);

  accel_to_decel_card = create_limit_card(
      content_cont,
      "Accel to Decel",
      &accel_to_decel_value,
      &accel_to_decel_slider,
      &accel_to_decel_reset,
      &LimitsPanel::_handle_callback,
      this);

  square_corner_card = create_limit_card(
      content_cont,
      "Square Corner Velocity",
      &square_corner_value,
      &square_corner_slider,
      &square_corner_reset,
      &LimitsPanel::_handle_callback,
      this);

  ws.register_notify_update(this);
}

LimitsPanel::~LimitsPanel()
{
  if (cont != nullptr) {
    lv_obj_del(cont);
    cont = nullptr;
  }
}

void LimitsPanel::init(json &j)
{
  State *state = State::get_instance();
  auto settings = state->get_data(
      "/printer_state/configfile/settings/printer"_json_pointer);

  if (!settings.is_null()) {
    if (settings.contains("max_velocity")) {
      max_velocity_default =
          settings["max_velocity"].template get<int>();
    }

    if (settings.contains("max_accel")) {
      max_accel_default =
          settings["max_accel"].template get<int>();
    }

    if (settings.contains("max_accel_to_decel")) {
      max_accel_to_decel_default =
          settings["max_accel_to_decel"].template get<int>();
    }

    if (settings.contains("square_corner_velocity")) {
      square_corner_default =
          settings["square_corner_velocity"].template get<int>();
    }
  }

  lv_slider_set_range(velocity_slider, 1, max_velocity_default);
  lv_slider_set_range(acceleration_slider, 1, max_accel_default);
  lv_slider_set_range(
      accel_to_decel_slider,
      1,
      max_accel_to_decel_default);
  lv_slider_set_range(
      square_corner_slider,
      0,
      square_corner_default);

  auto value = j["/result/status/toolhead/max_velocity"_json_pointer];
  if (!value.is_null()) {
    set_value(
        velocity_value,
        velocity_slider,
        value.template get<int>(),
        "mm/s");
  }

  value = j["/result/status/toolhead/max_accel"_json_pointer];
  if (!value.is_null()) {
    set_value(
        acceleration_value,
        acceleration_slider,
        value.template get<int>(),
        "mm/s2");
  }

  value = j["/result/status/toolhead/max_accel_to_decel"_json_pointer];
  if (!value.is_null()) {
    set_value(
        accel_to_decel_value,
        accel_to_decel_slider,
        value.template get<int>(),
        "mm/s2");
  }

  value = j[
      "/result/status/toolhead/square_corner_velocity"_json_pointer];
  if (!value.is_null()) {
    set_value(
        square_corner_value,
        square_corner_slider,
        value.template get<int>(),
        "mm/s");
  }
}

void LimitsPanel::foreground()
{
  lv_obj_move_foreground(cont);
}

void LimitsPanel::consume(json &j)
{
  std::lock_guard<std::mutex> lock(lv_lock);

  auto value = j["/params/0/toolhead/max_velocity"_json_pointer];
  if (!value.is_null()) {
    set_value(
        velocity_value,
        velocity_slider,
        value.template get<int>(),
        "mm/s");
  }

  value = j["/params/0/toolhead/max_accel"_json_pointer];
  if (!value.is_null()) {
    set_value(
        acceleration_value,
        acceleration_slider,
        value.template get<int>(),
        "mm/s2");
  }

  value = j[
      "/params/0/toolhead/max_accel_to_decel"_json_pointer];
  if (!value.is_null()) {
    set_value(
        accel_to_decel_value,
        accel_to_decel_slider,
        value.template get<int>(),
        "mm/s2");
  }

  value = j[
      "/params/0/toolhead/square_corner_velocity"_json_pointer];
  if (!value.is_null()) {
    set_value(
        square_corner_value,
        square_corner_slider,
        value.template get<int>(),
        "mm/s");
  }
}

void LimitsPanel::handle_callback(lv_event_t *event)
{
  const lv_event_code_t code = lv_event_get_code(event);
  lv_obj_t *target = lv_event_get_current_target(event);

  if (code == LV_EVENT_CLICKED && target == back_btn) {
    lv_obj_move_background(cont);
    return;
  }

  if (code == LV_EVENT_VALUE_CHANGED) {
    if (target == velocity_slider) {
      set_value(
          velocity_value,
          velocity_slider,
          lv_slider_get_value(velocity_slider),
          "mm/s");
    } else if (target == acceleration_slider) {
      set_value(
          acceleration_value,
          acceleration_slider,
          lv_slider_get_value(acceleration_slider),
          "mm/s2");
    } else if (target == accel_to_decel_slider) {
      set_value(
          accel_to_decel_value,
          accel_to_decel_slider,
          lv_slider_get_value(accel_to_decel_slider),
          "mm/s2");
    } else if (target == square_corner_slider) {
      set_value(
          square_corner_value,
          square_corner_slider,
          lv_slider_get_value(square_corner_slider),
          "mm/s");
    }
    return;
  }

  if (code == LV_EVENT_RELEASED) {
    if (target == velocity_slider) {
      ws.gcode_script(fmt::format(
          "SET_VELOCITY_LIMIT VELOCITY={}",
          lv_slider_get_value(velocity_slider)));
    } else if (target == acceleration_slider) {
      ws.gcode_script(fmt::format(
          "SET_VELOCITY_LIMIT ACCEL={}",
          lv_slider_get_value(acceleration_slider)));
    } else if (target == accel_to_decel_slider) {
      ws.gcode_script(fmt::format(
          "SET_VELOCITY_LIMIT ACCEL_TO_DECEL={}",
          lv_slider_get_value(accel_to_decel_slider)));
    } else if (target == square_corner_slider) {
      ws.gcode_script(fmt::format(
          "SET_VELOCITY_LIMIT SQUARE_CORNER_VELOCITY={}",
          lv_slider_get_value(square_corner_slider)));
    }
    return;
  }

  if (code != LV_EVENT_CLICKED) {
    return;
  }

  if (target == velocity_reset) {
    ws.gcode_script(fmt::format(
        "SET_VELOCITY_LIMIT VELOCITY={}",
        max_velocity_default));
  } else if (target == acceleration_reset) {
    ws.gcode_script(fmt::format(
        "SET_VELOCITY_LIMIT ACCEL={}",
        max_accel_default));
  } else if (target == accel_to_decel_reset) {
    ws.gcode_script(fmt::format(
        "SET_VELOCITY_LIMIT ACCEL_TO_DECEL={}",
        max_accel_to_decel_default));
  } else if (target == square_corner_reset) {
    ws.gcode_script(fmt::format(
        "SET_VELOCITY_LIMIT SQUARE_CORNER_VELOCITY={}",
        square_corner_default));
  }
}
