#include "homing_panel.h"

#include "config.h"
#include "state.h"
#include "spdlog/spdlog.h"

LV_IMG_DECLARE(arrow_left);
LV_IMG_DECLARE(arrow_up);
LV_IMG_DECLARE(arrow_right);
LV_IMG_DECLARE(arrow_down);
LV_IMG_DECLARE(home);
LV_IMG_DECLARE(z_closer);
LV_IMG_DECLARE(z_farther);
LV_IMG_DECLARE(emergency);
LV_IMG_DECLARE(motor_off_img);

namespace {

constexpr uint32_t COLOR_BG = 0x080B0D;
constexpr uint32_t COLOR_CARD = 0x11171B;
constexpr uint32_t COLOR_CARD_PRESSED = 0x18242A;
constexpr uint32_t COLOR_BORDER = 0x25323A;
constexpr uint32_t COLOR_ACCENT = 0x00E5FF;
constexpr uint32_t COLOR_TEXT = 0xFFFFFF;
constexpr uint32_t COLOR_MUTED = 0xB8C0C5;
constexpr uint32_t COLOR_DANGER = 0xFF4D4F;

static const char *distance_map[] = {
  ".1", ".5", "1", "5", "10", "25", "50", ""
};

void style_button(lv_obj_t *button, bool danger = false)
{
  lv_obj_clear_flag(button, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(button, lv_color_hex(COLOR_CARD), 0);
  lv_obj_set_style_border_width(button, 1, 0);
  lv_obj_set_style_border_color(
      button,
      lv_color_hex(danger ? COLOR_DANGER : COLOR_BORDER),
      0);
  lv_obj_set_style_radius(button, 8, 0);
  lv_obj_set_style_shadow_width(button, 0, 0);
  lv_obj_set_style_bg_color(button, lv_color_hex(COLOR_CARD_PRESSED), LV_STATE_PRESSED);
  lv_obj_set_style_border_color(
      button,
      lv_color_hex(danger ? COLOR_DANGER : COLOR_ACCENT),
      LV_STATE_PRESSED);
  lv_obj_set_style_opa(button, LV_OPA_40, LV_STATE_DISABLED);
}

lv_obj_t *create_action_button(lv_obj_t *parent,
                               const void *image,
                               const char *text,
                               lv_event_cb_t callback,
                               void *user_data,
                               bool danger = false,
                               lv_obj_t **icon_out = nullptr)
{
  lv_obj_t *button = lv_btn_create(parent);
  style_button(button, danger);
  lv_obj_set_style_pad_all(button, 6, 0);
  lv_obj_set_flex_flow(button, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(
      button,
      LV_FLEX_ALIGN_CENTER,
      LV_FLEX_ALIGN_CENTER,
      LV_FLEX_ALIGN_CENTER);
  lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, user_data);

  lv_obj_t *icon = lv_img_create(button);
  lv_img_set_src(icon, image);
  lv_img_set_zoom(icon, 128);
  lv_obj_set_style_translate_y(icon, -6, 0);
  lv_obj_set_style_img_recolor_opa(icon, LV_OPA_COVER, 0);
  lv_obj_set_style_img_recolor(
      icon,
      lv_color_hex(danger ? COLOR_DANGER : COLOR_TEXT),
      0);

  lv_obj_t *label = lv_label_create(button);
  lv_label_set_text(label, text);
  lv_obj_set_width(label, LV_PCT(100));
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_translate_y(label, -6, 0);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(
      label,
      lv_color_hex(danger ? COLOR_DANGER : COLOR_TEXT),
      0);

  if (icon_out != nullptr) {
    *icon_out = icon;
  }

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

HomingPanel::HomingPanel(KWebSocketClient &websocket_client, std::mutex &lock)
  : NotifyConsumer(lock)
  , ws(websocket_client)
  , homing_cont(lv_obj_create(lv_scr_act()))
  , header_cont(lv_obj_create(homing_cont))
  , back_btn(lv_btn_create(header_cont))
  , title_label(lv_label_create(header_cont))
  , home_all_btn(nullptr)
  , home_xy_btn(nullptr)
  , y_up_btn(nullptr)
  , y_down_btn(nullptr)
  , x_up_btn(nullptr)
  , x_down_btn(nullptr)
  , z_up_btn(nullptr)
  , z_down_btn(nullptr)
  , z_up_icon(nullptr)
  , z_down_icon(nullptr)
  , motoroff_btn(nullptr)
  , emergency_btn(nullptr)
  , distance_btnm(nullptr)
  , distance_idx(2)
  , emergency_prompt(nullptr)
  , emergency_confirm_btn(nullptr)
  , emergency_cancel_btn(nullptr)
{
  lv_obj_set_size(homing_cont, LV_PCT(100), LV_PCT(100));
  lv_obj_clear_flag(homing_cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(homing_cont, 8, 0);
  lv_obj_set_style_pad_row(homing_cont, 8, 0);
  lv_obj_set_style_border_width(homing_cont, 0, 0);
  lv_obj_set_style_bg_color(homing_cont, lv_color_hex(COLOR_BG), 0);

  static lv_coord_t rows[] = {
    42,
    60,
    LV_GRID_FR(1),
    76,
    54,
    LV_GRID_TEMPLATE_LAST
  };
  static lv_coord_t cols[] = {
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  lv_obj_set_grid_dsc_array(homing_cont, cols, rows);

  lv_obj_set_grid_cell(
      header_cont,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 0, 1);
  style_transparent_container(header_cont);

  style_button(back_btn);
  lv_obj_set_size(back_btn, 40, 34);
  lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 0, 0);
  lv_obj_add_event_cb(back_btn, &HomingPanel::_handle_callback, LV_EVENT_CLICKED, this);

  lv_obj_t *back_label = lv_label_create(back_btn);
  lv_label_set_text(back_label, LV_SYMBOL_LEFT);
  lv_obj_set_style_text_font(back_label, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(back_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(back_label);

  lv_label_set_text(title_label, "Homing");
  lv_obj_set_style_text_font(title_label, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(title_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(title_label);

  lv_obj_t *home_cont = lv_obj_create(homing_cont);
  lv_obj_set_grid_cell(
      home_cont,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 1, 1);
  style_transparent_container(home_cont);
  lv_obj_set_style_pad_column(home_cont, 8, 0);

  static lv_coord_t home_rows[] = {
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  static lv_coord_t home_cols[] = {
    LV_GRID_FR(1),
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  lv_obj_set_grid_dsc_array(home_cont, home_cols, home_rows);

  home_all_btn = create_action_button(
      home_cont, &home, "Home All", &HomingPanel::_handle_callback, this);
  home_xy_btn = create_action_button(
      home_cont, &home, "Home XY", &HomingPanel::_handle_callback, this);

  lv_obj_set_grid_cell(
      home_all_btn,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 0, 1);
  lv_obj_set_grid_cell(
      home_xy_btn,
      LV_GRID_ALIGN_STRETCH, 1, 1,
      LV_GRID_ALIGN_STRETCH, 0, 1);

  lv_obj_t *move_cont = lv_obj_create(homing_cont);
  lv_obj_set_grid_cell(
      move_cont,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 2, 1);
  style_transparent_container(move_cont);
  lv_obj_set_style_pad_column(move_cont, 8, 0);

  static lv_coord_t move_rows[] = {
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  static lv_coord_t move_cols[] = {
    LV_GRID_FR(2),
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  lv_obj_set_grid_dsc_array(move_cont, move_cols, move_rows);

  lv_obj_t *xy_cont = lv_obj_create(move_cont);
  lv_obj_set_grid_cell(
      xy_cont,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 0, 1);
  style_transparent_container(xy_cont);
  lv_obj_set_style_pad_all(xy_cont, 4, 0);
  lv_obj_set_style_pad_row(xy_cont, 6, 0);
  lv_obj_set_style_pad_column(xy_cont, 6, 0);

  static lv_coord_t xy_rows[] = {
    LV_GRID_FR(1),
    LV_GRID_FR(1),
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  static lv_coord_t xy_cols[] = {
    LV_GRID_FR(1),
    LV_GRID_FR(1),
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  lv_obj_set_grid_dsc_array(xy_cont, xy_cols, xy_rows);

  y_up_btn = create_action_button(
      xy_cont, &arrow_up, "Y+", &HomingPanel::_handle_callback, this);
  y_down_btn = create_action_button(
      xy_cont, &arrow_down, "Y-", &HomingPanel::_handle_callback, this);
  x_up_btn = create_action_button(
      xy_cont, &arrow_right, "X+", &HomingPanel::_handle_callback, this);
  x_down_btn = create_action_button(
      xy_cont, &arrow_left, "X-", &HomingPanel::_handle_callback, this);

  lv_obj_set_grid_cell(
      y_up_btn,
      LV_GRID_ALIGN_STRETCH, 1, 1,
      LV_GRID_ALIGN_STRETCH, 0, 1);
  lv_obj_set_grid_cell(
      x_down_btn,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 1, 1);
  lv_obj_set_grid_cell(
      x_up_btn,
      LV_GRID_ALIGN_STRETCH, 2, 1,
      LV_GRID_ALIGN_STRETCH, 1, 1);
  lv_obj_set_grid_cell(
      y_down_btn,
      LV_GRID_ALIGN_STRETCH, 1, 1,
      LV_GRID_ALIGN_STRETCH, 2, 1);

  lv_obj_t *xy_label = lv_label_create(xy_cont);
  lv_label_set_text(xy_label, "XY");
  lv_obj_set_style_text_font(xy_label, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(xy_label, lv_color_hex(COLOR_MUTED), 0);
  lv_obj_set_grid_cell(
      xy_label,
      LV_GRID_ALIGN_CENTER, 1, 1,
      LV_GRID_ALIGN_CENTER, 1, 1);

  lv_obj_t *z_cont = lv_obj_create(move_cont);
  lv_obj_set_grid_cell(
      z_cont,
      LV_GRID_ALIGN_STRETCH, 1, 1,
      LV_GRID_ALIGN_STRETCH, 0, 1);
  style_transparent_container(z_cont);
  lv_obj_set_style_pad_row(z_cont, 8, 0);

  static lv_coord_t z_rows[] = {
    LV_GRID_FR(1),
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  static lv_coord_t z_cols[] = {
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  lv_obj_set_grid_dsc_array(z_cont, z_cols, z_rows);

  z_up_btn = create_action_button(
      z_cont, &z_closer, "Z+", &HomingPanel::_handle_callback, this, false, &z_up_icon);
  z_down_btn = create_action_button(
      z_cont, &z_farther, "Z-", &HomingPanel::_handle_callback, this, false, &z_down_icon);

  lv_obj_set_grid_cell(
      z_up_btn,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 0, 1);
  lv_obj_set_grid_cell(
      z_down_btn,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 1, 1);

  lv_obj_t *distance_cont = lv_obj_create(homing_cont);
  lv_obj_set_grid_cell(
      distance_cont,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 3, 1);
  style_transparent_container(distance_cont);
  lv_obj_set_flex_flow(distance_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(
      distance_cont,
      LV_FLEX_ALIGN_CENTER,
      LV_FLEX_ALIGN_CENTER,
      LV_FLEX_ALIGN_CENTER);

  lv_obj_t *distance_label = lv_label_create(distance_cont);
  lv_label_set_text(distance_label, "Move distance (mm)");
  lv_obj_set_width(distance_label, LV_PCT(100));
  lv_obj_set_style_text_align(distance_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(distance_label, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(distance_label, lv_color_hex(COLOR_MUTED), 0);

  distance_btnm = lv_btnmatrix_create(distance_cont);
  lv_obj_set_size(distance_btnm, LV_PCT(100), 46);
  lv_btnmatrix_set_map(distance_btnm, distance_map);
  lv_btnmatrix_set_btn_ctrl_all(distance_btnm, LV_BTNMATRIX_CTRL_CHECKABLE);
  lv_btnmatrix_set_one_checked(distance_btnm, true);
  lv_btnmatrix_set_btn_ctrl(distance_btnm, distance_idx, LV_BTNMATRIX_CTRL_CHECKED);
  lv_obj_add_event_cb(
      distance_btnm,
      &HomingPanel::_handle_distance_cb,
      LV_EVENT_VALUE_CHANGED,
      this);

  lv_obj_set_style_bg_color(distance_btnm, lv_color_hex(COLOR_CARD), LV_PART_MAIN);
  lv_obj_set_style_border_width(distance_btnm, 1, LV_PART_MAIN);
  lv_obj_set_style_border_color(distance_btnm, lv_color_hex(COLOR_BORDER), LV_PART_MAIN);
  lv_obj_set_style_radius(distance_btnm, 8, LV_PART_MAIN);
  lv_obj_set_style_pad_all(distance_btnm, 3, LV_PART_MAIN);
  lv_obj_set_style_bg_color(distance_btnm, lv_color_hex(COLOR_CARD), LV_PART_ITEMS);
  lv_obj_set_style_border_width(distance_btnm, 1, LV_PART_ITEMS);
  lv_obj_set_style_border_color(distance_btnm, lv_color_hex(COLOR_BORDER), LV_PART_ITEMS);
  lv_obj_set_style_text_color(distance_btnm, lv_color_hex(COLOR_TEXT), LV_PART_ITEMS);
  lv_obj_set_style_bg_color(
      distance_btnm,
      lv_color_hex(COLOR_ACCENT),
      LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_set_style_text_color(
      distance_btnm,
      lv_color_hex(COLOR_BG),
      LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_set_style_outline_width(
      distance_btnm,
      0,
      LV_PART_ITEMS | LV_STATE_FOCUS_KEY);

  lv_obj_t *bottom_cont = lv_obj_create(homing_cont);
  lv_obj_set_grid_cell(
      bottom_cont,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 4, 1);
  style_transparent_container(bottom_cont);
  lv_obj_set_style_pad_column(bottom_cont, 8, 0);

  static lv_coord_t bottom_rows[] = {
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  static lv_coord_t bottom_cols[] = {
    LV_GRID_FR(1),
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  lv_obj_set_grid_dsc_array(bottom_cont, bottom_cols, bottom_rows);

  motoroff_btn = create_action_button(
      bottom_cont,
      &motor_off_img,
      "Motor Off",
      &HomingPanel::_handle_callback,
      this);
  emergency_btn = create_action_button(
      bottom_cont,
      &emergency,
      "Stop",
      &HomingPanel::_handle_callback,
      this,
      true);

  lv_obj_set_grid_cell(
      motoroff_btn,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 0, 1);
  lv_obj_set_grid_cell(
      emergency_btn,
      LV_GRID_ALIGN_STRETCH, 1, 1,
      LV_GRID_ALIGN_STRETCH, 0, 1);

  set_button_enabled(x_up_btn, false);
  set_button_enabled(x_down_btn, false);
  set_button_enabled(y_up_btn, false);
  set_button_enabled(y_down_btn, false);
  set_button_enabled(z_up_btn, false);
  set_button_enabled(z_down_btn, false);

  ws.register_notify_update(this);
}

HomingPanel::~HomingPanel()
{
  ws.unregister_notify_update(this);

  if (emergency_prompt != nullptr) {
    lv_obj_del(emergency_prompt);
    emergency_prompt = nullptr;
    emergency_confirm_btn = nullptr;
    emergency_cancel_btn = nullptr;
  }

  if (homing_cont != nullptr) {
    lv_obj_del(homing_cont);
    homing_cont = nullptr;
  }
}

void HomingPanel::set_button_enabled(lv_obj_t *button, bool enabled)
{
  if (enabled) {
    lv_obj_clear_state(button, LV_STATE_DISABLED);
  } else {
    lv_obj_add_state(button, LV_STATE_DISABLED);
  }
}

void HomingPanel::update_axes(const std::string &homed_axes)
{
  const bool x_homed = homed_axes.find('x') != std::string::npos;
  const bool y_homed = homed_axes.find('y') != std::string::npos;
  const bool z_homed = homed_axes.find('z') != std::string::npos;

  set_button_enabled(x_up_btn, x_homed);
  set_button_enabled(x_down_btn, x_homed);
  set_button_enabled(y_up_btn, y_homed);
  set_button_enabled(y_down_btn, y_homed);
  set_button_enabled(z_up_btn, z_homed);
  set_button_enabled(z_down_btn, z_homed);
}

void HomingPanel::update_z_icons()
{
  const auto value = Config::get_instance()->get_json("/invert_z_icon");
  const bool inverted = !value.is_null() && value.template get<bool>();

  if (inverted) {
    lv_img_set_src(z_up_icon, &z_farther);
    lv_img_set_src(z_down_icon, &z_closer);
  } else {
    lv_img_set_src(z_up_icon, &z_closer);
    lv_img_set_src(z_down_icon, &z_farther);
  }
}

void HomingPanel::consume(json &j)
{
  auto value = j["/params/0/toolhead/homed_axes"_json_pointer];
  if (value.is_null()) {
    return;
  }

  std::lock_guard<std::mutex> lock(lv_lock);
  update_axes(value.template get<std::string>());
}

lv_obj_t *HomingPanel::get_container()
{
  return homing_cont;
}

void HomingPanel::foreground()
{
  auto value = State::get_instance()
      ->get_data("/printer_state/toolhead/homed_axes"_json_pointer);

  if (value.is_null()) {
    update_axes("");
  } else {
    update_axes(value.template get<std::string>());
  }

  update_z_icons();
  lv_obj_move_foreground(homing_cont);
}

const char *HomingPanel::selected_distance() const
{
  const char *distance = lv_btnmatrix_get_btn_text(distance_btnm, distance_idx);
  return distance == nullptr ? "1" : distance;
}

void HomingPanel::request_emergency_stop()
{
  const auto value = Config::get_instance()->get_json("/prompt_emergency_stop");
  const bool should_prompt = !value.is_null() && value.template get<bool>();

  if (!should_prompt) {
    spdlog::debug("emergency stop pressed");
    ws.send_jsonrpc("printer.emergency_stop");
    return;
  }

  if (emergency_prompt != nullptr) {
    lv_obj_move_foreground(emergency_prompt);
    return;
  }

  emergency_prompt = lv_obj_create(lv_scr_act());
  lv_obj_set_size(emergency_prompt, LV_PCT(100), LV_PCT(100));
  lv_obj_clear_flag(emergency_prompt, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(emergency_prompt, 0, 0);
  lv_obj_set_style_border_width(emergency_prompt, 0, 0);
  lv_obj_set_style_radius(emergency_prompt, 0, 0);
  lv_obj_set_style_bg_color(emergency_prompt, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(emergency_prompt, LV_OPA_70, 0);

  lv_obj_t *dialog = lv_obj_create(emergency_prompt);
  lv_obj_set_size(dialog, LV_PCT(90), 170);
  lv_obj_center(dialog);
  lv_obj_clear_flag(dialog, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(dialog, 12, 0);
  lv_obj_set_style_bg_color(dialog, lv_color_hex(COLOR_CARD), 0);
  lv_obj_set_style_border_width(dialog, 1, 0);
  lv_obj_set_style_border_color(dialog, lv_color_hex(COLOR_BORDER), 0);
  lv_obj_set_style_radius(dialog, 10, 0);
  lv_obj_set_style_shadow_width(dialog, 0, 0);

  lv_obj_t *message = lv_label_create(dialog);
  lv_label_set_text(message, "Stop the printer immediately?");
  lv_label_set_long_mode(message, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(message, LV_PCT(100));
  lv_obj_set_style_text_align(message, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(message, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(message, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_align(message, LV_ALIGN_TOP_MID, 0, 14);

  emergency_confirm_btn = lv_btn_create(dialog);
  style_button(emergency_confirm_btn, true);
  lv_obj_set_size(emergency_confirm_btn, 100, 46);
  lv_obj_align(emergency_confirm_btn, LV_ALIGN_BOTTOM_LEFT, 0, 0);
  lv_obj_add_event_cb(
      emergency_confirm_btn,
      &HomingPanel::_handle_emergency_prompt,
      LV_EVENT_CLICKED,
      this);

  lv_obj_t *confirm_label = lv_label_create(emergency_confirm_btn);
  lv_label_set_text(confirm_label, "Confirm");
  lv_obj_set_style_text_color(confirm_label, lv_color_hex(COLOR_DANGER), 0);
  lv_obj_center(confirm_label);

  emergency_cancel_btn = lv_btn_create(dialog);
  style_button(emergency_cancel_btn);
  lv_obj_set_size(emergency_cancel_btn, 100, 46);
  lv_obj_align(emergency_cancel_btn, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
  lv_obj_add_event_cb(
      emergency_cancel_btn,
      &HomingPanel::_handle_emergency_prompt,
      LV_EVENT_CLICKED,
      this);

  lv_obj_t *cancel_label = lv_label_create(emergency_cancel_btn);
  lv_label_set_text(cancel_label, "Cancel");
  lv_obj_set_style_text_color(cancel_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(cancel_label);

  lv_obj_move_foreground(emergency_prompt);
}

void HomingPanel::handle_emergency_prompt(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
    return;
  }

  lv_obj_t *button = lv_event_get_current_target(event);
  if (button == emergency_confirm_btn) {
    spdlog::debug("emergency stop confirmed");
    ws.send_jsonrpc("printer.emergency_stop");
  }

  lv_obj_t *overlay = emergency_prompt;
  emergency_prompt = nullptr;
  emergency_confirm_btn = nullptr;
  emergency_cancel_btn = nullptr;

  if (overlay != nullptr) {
    lv_obj_del(overlay);
  }
}

void HomingPanel::handle_callback(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
    return;
  }

  lv_obj_t *button = lv_event_get_current_target(event);
  std::string move_op;

  if (button == back_btn) {
    lv_obj_move_background(homing_cont);
    return;
  }

  if (button == home_all_btn) {
    spdlog::debug("home all pressed");
    ws.gcode_script("G28 X Y Z");
  } else if (button == home_xy_btn) {
    spdlog::debug("home xy pressed");
    ws.gcode_script("G28 X Y");
  } else if (button == y_up_btn) {
    spdlog::debug("y up pressed");
    move_op = fmt::format("G0 Y+{} F1200", selected_distance());
  } else if (button == y_down_btn) {
    spdlog::debug("y down pressed");
    move_op = fmt::format("G0 Y-{} F1200", selected_distance());
  } else if (button == x_up_btn) {
    spdlog::debug("x up pressed");
    move_op = fmt::format("G0 X+{} F1200", selected_distance());
  } else if (button == x_down_btn) {
    spdlog::debug("x down pressed");
    move_op = fmt::format("G0 X-{} F1200", selected_distance());
  } else if (button == z_up_btn) {
    spdlog::debug("z up pressed");
    move_op = fmt::format("G0 Z+{} F1200", selected_distance());
  } else if (button == z_down_btn) {
    spdlog::debug("z down pressed");
    move_op = fmt::format("G0 Z-{} F1200", selected_distance());
  } else if (button == motoroff_btn) {
    spdlog::debug("motor off pressed");
    ws.gcode_script("M84");
  } else if (button == emergency_btn) {
    request_emergency_stop();
  } else {
    spdlog::debug("unknown Homing action button pressed");
  }

  if (!move_op.empty()) {
    ws.gcode_script(fmt::format("G91\n{}", move_op));
  }
}

void HomingPanel::handle_distance_cb(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED) {
    return;
  }

  lv_obj_t *matrix = lv_event_get_current_target(event);
  const uint32_t idx = lv_btnmatrix_get_selected_btn(matrix);

  if (idx < 7) {
    distance_idx = idx;
    spdlog::debug("move distance index {}", idx);
  }
}
