#include "finetune_panel.h"

#include "config.h"
#include "state.h"
#include "spdlog/fmt/fmt.h"
#include "spdlog/spdlog.h"

#include <algorithm>

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

void style_button(lv_obj_t *button, bool accent)
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

  lv_obj_t *label = lv_label_create(button);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(
      label,
      lv_color_hex(accent ? COLOR_ACCENT : COLOR_TEXT),
      0);
  lv_obj_center(label);
}

void style_selector(lv_obj_t *selector)
{
  lv_obj_set_style_bg_color(selector, lv_color_hex(COLOR_CARD_PRESSED), 0);
  lv_obj_set_style_border_width(selector, 1, 0);
  lv_obj_set_style_border_color(selector, lv_color_hex(COLOR_BORDER), 0);
  lv_obj_set_style_radius(selector, 6, 0);
  lv_obj_set_style_pad_all(selector, 2, 0);
  lv_obj_set_style_text_font(selector, &lv_font_montserrat_12, LV_PART_ITEMS);
  lv_obj_set_style_text_color(
      selector,
      lv_color_hex(COLOR_TEXT),
      LV_PART_ITEMS);
  lv_obj_set_style_bg_color(
      selector,
      lv_color_hex(COLOR_ACCENT),
      LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_set_style_text_color(
      selector,
      lv_color_hex(COLOR_BG),
      LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_set_style_outline_width(
      selector,
      0,
      LV_PART_ITEMS | LV_STATE_FOCUS_KEY);
}

lv_obj_t *create_action_card(lv_obj_t *parent,
                             const char *title,
                             lv_obj_t **value_label,
                             lv_obj_t **minus_btn,
                             lv_obj_t **reset_btn,
                             lv_obj_t **plus_btn,
                             lv_event_cb_t callback,
                             void *user_data)
{
  lv_obj_t *card = lv_obj_create(parent);
  style_card(card);
  lv_obj_set_width(card, LV_PCT(100));
  lv_obj_set_height(card, 68);

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
  lv_obj_set_size(controls, LV_PCT(100), 32);
  lv_obj_align(controls, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_pad_column(controls, 5, 0);
  lv_obj_set_flex_flow(controls, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(
      controls,
      LV_FLEX_ALIGN_START,
      LV_FLEX_ALIGN_CENTER,
      LV_FLEX_ALIGN_CENTER);

  *minus_btn = lv_btn_create(controls);
  style_button(*minus_btn, true);
  lv_obj_set_height(*minus_btn, 32);
  lv_obj_set_flex_grow(*minus_btn, 1);
  lv_label_set_text(lv_obj_get_child(*minus_btn, 0), LV_SYMBOL_MINUS);

  *reset_btn = lv_btn_create(controls);
  style_button(*reset_btn, false);
  lv_obj_set_height(*reset_btn, 32);
  lv_obj_set_flex_grow(*reset_btn, 1);
  lv_label_set_text(lv_obj_get_child(*reset_btn, 0), LV_SYMBOL_REFRESH);

  *plus_btn = lv_btn_create(controls);
  style_button(*plus_btn, true);
  lv_obj_set_height(*plus_btn, 32);
  lv_obj_set_flex_grow(*plus_btn, 1);
  lv_label_set_text(lv_obj_get_child(*plus_btn, 0), LV_SYMBOL_PLUS);

  lv_obj_add_event_cb(*minus_btn, callback, LV_EVENT_CLICKED, user_data);
  lv_obj_add_event_cb(*reset_btn, callback, LV_EVENT_CLICKED, user_data);
  lv_obj_add_event_cb(*plus_btn, callback, LV_EVENT_CLICKED, user_data);

  return card;
}

} // namespace

FineTunePanel::FineTunePanel(KWebSocketClient &websocket_client,
                             std::mutex &lock)
  : NotifyConsumer(lock)
  , ws(websocket_client)
  , panel_cont(lv_obj_create(lv_scr_act()))
  , header_cont(lv_obj_create(panel_cont))
  , back_btn(lv_btn_create(header_cont))
  , title_label(lv_label_create(header_cont))
  , content_cont(lv_obj_create(panel_cont))
  , zpa_step_cont(lv_obj_create(content_cont))
  , zoffset_selector(lv_btnmatrix_create(zpa_step_cont))
  , multiplier_step_cont(lv_obj_create(content_cont))
  , multiplier_selector(lv_btnmatrix_create(multiplier_step_cont))
  , z_card(nullptr)
  , z_value(nullptr)
  , zreset_btn(nullptr)
  , zup_btn(nullptr)
  , zup_icon(nullptr)
  , zdown_btn(nullptr)
  , zdown_icon(nullptr)
  , pa_card(nullptr)
  , pa_value(nullptr)
  , pareset_btn(nullptr)
  , paup_btn(nullptr)
  , padown_btn(nullptr)
  , speed_card(nullptr)
  , speed_value(nullptr)
  , speed_reset_btn(nullptr)
  , speed_up_btn(nullptr)
  , speed_down_btn(nullptr)
  , flow_card(nullptr)
  , flow_value(nullptr)
  , flow_reset_btn(nullptr)
  , flow_up_btn(nullptr)
  , flow_down_btn(nullptr)
  , zoffset_selector_idx(0)
  , multiplier_selector_idx(0)
{
  lv_obj_move_background(panel_cont);
  lv_obj_set_size(panel_cont, LV_PCT(100), LV_PCT(100));
  lv_obj_clear_flag(panel_cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(panel_cont, 8, 0);
  lv_obj_set_style_pad_row(panel_cont, 8, 0);
  lv_obj_set_style_border_width(panel_cont, 0, 0);
  lv_obj_set_style_bg_color(panel_cont, lv_color_hex(COLOR_BG), 0);

  static lv_coord_t root_rows[] = {
    42,
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  static lv_coord_t root_cols[] = {
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  lv_obj_set_grid_dsc_array(panel_cont, root_cols, root_rows);

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
      &FineTunePanel::_handle_callback,
      LV_EVENT_CLICKED,
      this);

  lv_obj_t *back_label = lv_label_create(back_btn);
  lv_label_set_text(back_label, LV_SYMBOL_LEFT);
  lv_obj_set_style_text_font(back_label, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(back_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(back_label);

  lv_label_set_text(title_label, "Fine Tune");
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

  style_card(zpa_step_cont);
  lv_obj_set_size(zpa_step_cont, LV_PCT(100), 46);

  lv_obj_t *zpa_label = lv_label_create(zpa_step_cont);
  lv_label_set_text(zpa_label, "Z / PA Step");
  lv_obj_set_style_text_font(zpa_label, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(zpa_label, lv_color_hex(COLOR_MUTED), 0);
  lv_obj_align(zpa_label, LV_ALIGN_LEFT_MID, 0, 0);

  static const char *zpa_map[] = {
    "0.01", "0.05", "0.10", ""
  };
  lv_btnmatrix_set_map(zoffset_selector, zpa_map);
  style_selector(zoffset_selector);
  lv_obj_set_size(zoffset_selector, 150, 30);
  lv_obj_align(zoffset_selector, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_btnmatrix_set_btn_ctrl_all(
      zoffset_selector,
      LV_BTNMATRIX_CTRL_CHECKABLE);
  lv_btnmatrix_set_one_checked(zoffset_selector, true);
  lv_btnmatrix_set_btn_ctrl(
      zoffset_selector,
      zoffset_selector_idx,
      LV_BTNMATRIX_CTRL_CHECKED);
  lv_obj_add_event_cb(
      zoffset_selector,
      &FineTunePanel::_handle_callback,
      LV_EVENT_VALUE_CHANGED,
      this);

  style_card(multiplier_step_cont);
  lv_obj_set_size(multiplier_step_cont, LV_PCT(100), 46);

  lv_obj_t *multiplier_label = lv_label_create(multiplier_step_cont);
  lv_label_set_text(multiplier_label, "Speed / Flow");
  lv_obj_set_style_text_font(multiplier_label, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(multiplier_label, lv_color_hex(COLOR_MUTED), 0);
  lv_obj_align(multiplier_label, LV_ALIGN_LEFT_MID, 0, 0);

  static const char *multiplier_map[] = {
    "1", "5", "10", "25", ""
  };
  lv_btnmatrix_set_map(multiplier_selector, multiplier_map);
  style_selector(multiplier_selector);
  lv_obj_set_size(multiplier_selector, 150, 30);
  lv_obj_align(multiplier_selector, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_btnmatrix_set_btn_ctrl_all(
      multiplier_selector,
      LV_BTNMATRIX_CTRL_CHECKABLE);
  lv_btnmatrix_set_one_checked(multiplier_selector, true);
  lv_btnmatrix_set_btn_ctrl(
      multiplier_selector,
      multiplier_selector_idx,
      LV_BTNMATRIX_CTRL_CHECKED);
  lv_obj_add_event_cb(
      multiplier_selector,
      &FineTunePanel::_handle_callback,
      LV_EVENT_VALUE_CHANGED,
      this);

  z_card = create_action_card(
      content_cont,
      "Z Offset",
      &z_value,
      &zdown_btn,
      &zreset_btn,
      &zup_btn,
      &FineTunePanel::_handle_zoffset,
      this);
  zdown_icon = lv_obj_get_child(zdown_btn, 0);
  zup_icon = lv_obj_get_child(zup_btn, 0);

  pa_card = create_action_card(
      content_cont,
      "Pressure Advance",
      &pa_value,
      &padown_btn,
      &pareset_btn,
      &paup_btn,
      &FineTunePanel::_handle_pa,
      this);

  speed_card = create_action_card(
      content_cont,
      "Speed",
      &speed_value,
      &speed_down_btn,
      &speed_reset_btn,
      &speed_up_btn,
      &FineTunePanel::_handle_speed,
      this);

  flow_card = create_action_card(
      content_cont,
      "Flow",
      &flow_value,
      &flow_down_btn,
      &flow_reset_btn,
      &flow_up_btn,
      &FineTunePanel::_handle_flow,
      this);

  ws.register_notify_update(this);
}

FineTunePanel::~FineTunePanel()
{
  if (panel_cont != nullptr) {
    lv_obj_del(panel_cont);
    panel_cont = nullptr;
  }

  ws.unregister_notify_update(this);
}

void FineTunePanel::foreground()
{
  auto value = State::get_instance()->get_data(
      "/printer_state/gcode_move/homing_origin/2"_json_pointer);
  if (!value.is_null()) {
    lv_label_set_text(
        z_value,
        fmt::format("{:.5} mm", value.template get<double>()).c_str());
  }

  value = State::get_instance()->get_data(
      "/printer_state/extruder/pressure_advance"_json_pointer);
  if (!value.is_null()) {
    lv_label_set_text(
        pa_value,
        fmt::format("{:.5} mm/s", value.template get<double>()).c_str());
  }

  value = State::get_instance()->get_data(
      "/printer_state/gcode_move/speed_factor"_json_pointer);
  if (!value.is_null()) {
    lv_label_set_text(
        speed_value,
        fmt::format(
            "{}%",
            static_cast<int>(value.template get<double>() * 100)).c_str());
  }

  value = State::get_instance()->get_data(
      "/printer_state/gcode_move/extrude_factor"_json_pointer);
  if (!value.is_null()) {
    lv_label_set_text(
        flow_value,
        fmt::format(
            "{}%",
            static_cast<int>(value.template get<double>() * 100)).c_str());
  }

  value = Config::get_instance()->get_json("/invert_z_icon");
  const bool inverted =
      !value.is_null() && value.template get<bool>();

  lv_label_set_text(
      zup_icon,
      inverted ? LV_SYMBOL_UP : LV_SYMBOL_DOWN);
  lv_label_set_text(
      zdown_icon,
      inverted ? LV_SYMBOL_DOWN : LV_SYMBOL_UP);

  lv_obj_move_foreground(panel_cont);
}

void FineTunePanel::consume(json &j)
{
  std::lock_guard<std::mutex> lock(lv_lock);

  auto value = j["/params/0/gcode_move/homing_origin/2"_json_pointer];
  if (!value.is_null()) {
    lv_label_set_text(
        z_value,
        fmt::format("{:.5} mm", value.template get<double>()).c_str());
  }

  value = j["/params/0/extruder/pressure_advance"_json_pointer];
  if (!value.is_null()) {
    lv_label_set_text(
        pa_value,
        fmt::format("{:.5} mm/s", value.template get<double>()).c_str());
  }

  value = j["/params/0/gcode_move/speed_factor"_json_pointer];
  if (!value.is_null()) {
    lv_label_set_text(
        speed_value,
        fmt::format(
            "{}%",
            static_cast<int>(value.template get<double>() * 100)).c_str());
  }

  value = j["/params/0/gcode_move/extrude_factor"_json_pointer];
  if (!value.is_null()) {
    lv_label_set_text(
        flow_value,
        fmt::format(
            "{}%",
            static_cast<int>(value.template get<double>() * 100)).c_str());
  }
}

void FineTunePanel::handle_callback(lv_event_t *event)
{
  const lv_event_code_t code = lv_event_get_code(event);
  lv_obj_t *target = lv_event_get_current_target(event);

  if (code == LV_EVENT_CLICKED && target == back_btn) {
    lv_obj_move_background(panel_cont);
    return;
  }

  if (code != LV_EVENT_VALUE_CHANGED) {
    return;
  }

  if (target == zoffset_selector) {
    zoffset_selector_idx =
        lv_btnmatrix_get_selected_btn(zoffset_selector);
  } else if (target == multiplier_selector) {
    multiplier_selector_idx =
        lv_btnmatrix_get_selected_btn(multiplier_selector);
  }
}

void FineTunePanel::handle_zoffset(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
    return;
  }

  lv_obj_t *button = lv_event_get_current_target(event);

  if (button == zreset_btn) {
    spdlog::trace("clicked zoffset reset");
    ws.gcode_script("SET_GCODE_OFFSET Z=0 MOVE=1");
    return;
  }

  const char *step = lv_btnmatrix_get_btn_text(
      zoffset_selector,
      zoffset_selector_idx);

  spdlog::trace("clicked z {}", step);

  ws.gcode_script(fmt::format(
      "SET_GCODE_OFFSET Z_ADJUST={}{} MOVE=1",
      button == zup_btn ? "+" : "-",
      step));
}

void FineTunePanel::handle_pa(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
    return;
  }

  lv_obj_t *button = lv_event_get_current_target(event);

  if (button == pareset_btn) {
    spdlog::trace("clicked pa reset");

    auto value = State::get_instance()->get_data(
        "/printer_state/configfile/settings/extruder/pressure_advance"_json_pointer);

    if (!value.is_null()) {
      ws.gcode_script(fmt::format(
          "SET_PRESSURE_ADVANCE ADVANCE={}",
          value.template get<double>()));
    }
    return;
  }

  auto current_pa = State::get_instance()->get_data(
      "/printer_state/extruder/pressure_advance"_json_pointer);

  if (current_pa.is_null()) {
    return;
  }

  const char *step = lv_btnmatrix_get_btn_text(
      zoffset_selector,
      zoffset_selector_idx);

  const double direction =
      button == paup_btn ? std::stod(step) : -std::stod(step);

  double new_pa =
      current_pa.template get<double>() + direction;
  new_pa = std::max(new_pa, 0.0);

  ws.gcode_script(fmt::format(
      "SET_PRESSURE_ADVANCE ADVANCE={}",
      new_pa));
}

void FineTunePanel::handle_speed(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
    return;
  }

  lv_obj_t *button = lv_event_get_current_target(event);

  if (button == speed_reset_btn) {
    spdlog::trace("speed reset");
    ws.gcode_script("M220 S100");
    return;
  }

  auto speed_factor = State::get_instance()->get_data(
      "/printer_state/gcode_move/speed_factor"_json_pointer);

  if (speed_factor.is_null()) {
    return;
  }

  const char *step = lv_btnmatrix_get_btn_text(
      multiplier_selector,
      multiplier_selector_idx);

  const int32_t direction =
      button == speed_up_btn ? std::stoi(step) : -std::stoi(step);

  int32_t new_speed =
      static_cast<int32_t>(
          speed_factor.template get<double>() * 100 + direction);
  new_speed = std::max(new_speed, 1);

  spdlog::trace("speed step {}, {}", direction, new_speed);
  ws.gcode_script(fmt::format("M220 S{}", new_speed));
}

void FineTunePanel::handle_flow(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
    return;
  }

  lv_obj_t *button = lv_event_get_current_target(event);

  if (button == flow_reset_btn) {
    spdlog::trace("flow reset");
    ws.gcode_script("M221 S100");
    return;
  }

  auto extrude_factor = State::get_instance()->get_data(
      "/printer_state/gcode_move/extrude_factor"_json_pointer);

  if (extrude_factor.is_null()) {
    return;
  }

  const char *step = lv_btnmatrix_get_btn_text(
      multiplier_selector,
      multiplier_selector_idx);

  const int32_t direction =
      button == flow_up_btn ? std::stoi(step) : -std::stoi(step);

  int32_t new_flow =
      static_cast<int32_t>(
          extrude_factor.template get<double>() * 100 + direction);
  new_flow = std::max(new_flow, 1);

  spdlog::trace("flow step {}, {}", direction, new_flow);
  ws.gcode_script(fmt::format("M221 S{}", new_flow));
}
