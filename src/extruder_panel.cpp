#include "extruder_panel.h"

#include "config.h"
#include "state.h"
#include "spdlog/spdlog.h"

LV_IMG_DECLARE(spoolman_img);
LV_IMG_DECLARE(extrude_img);
LV_IMG_DECLARE(retract_img);
LV_IMG_DECLARE(unload_filament_img);
LV_IMG_DECLARE(load_filament_img);
LV_IMG_DECLARE(extruder);
LV_IMG_DECLARE(cooldown_img);

namespace {

constexpr uint32_t COLOR_BG = 0x080B0D;
constexpr uint32_t COLOR_CARD = 0x11171B;
constexpr uint32_t COLOR_CARD_PRESSED = 0x18242A;
constexpr uint32_t COLOR_BORDER = 0x25323A;
constexpr uint32_t COLOR_ACCENT = 0x00E5FF;
constexpr uint32_t COLOR_TEXT = 0xFFFFFF;
constexpr uint32_t COLOR_MUTED = 0xB8C0C5;

static const char *temp_map[] = {
  "180", "190", "200", "210", "220", "230", "240", ""
};
static const char *length_map[] = {
  "5", "10", "15", "20", "25", "30", "35", ""
};
static const char *speed_map[] = {
  "1", "2", "5", "10", "25", "35", "50", ""
};

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
  lv_obj_set_style_opa(button, LV_OPA_40, LV_STATE_DISABLED);
}

void style_transparent_container(lv_obj_t *cont)
{
  lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(cont, 0, 0);
  lv_obj_set_style_border_width(cont, 0, 0);
  lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
}

lv_obj_t *create_action_button(lv_obj_t *parent,
                               const void *image,
                               const char *text,
                               lv_event_cb_t callback,
                               void *user_data)
{
  lv_obj_t *button = lv_btn_create(parent);
  style_button(button);
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
  lv_obj_set_style_img_recolor(icon, lv_color_hex(COLOR_TEXT), 0);

  lv_obj_t *label = lv_label_create(button);
  lv_label_set_text(label, text);
  lv_obj_set_width(label, LV_PCT(100));
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_translate_y(label, -6, 0);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(COLOR_TEXT), 0);

  return button;
}

lv_obj_t *create_selector(lv_obj_t *parent,
                          const char *label_text,
                          const char **map,
                          uint32_t selected,
                          lv_event_cb_t callback,
                          void *user_data)
{
  lv_obj_t *cont = lv_obj_create(parent);
  lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
  style_transparent_container(cont);
  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(
      cont,
      LV_FLEX_ALIGN_CENTER,
      LV_FLEX_ALIGN_CENTER,
      LV_FLEX_ALIGN_CENTER);

  lv_obj_t *label = lv_label_create(cont);
  lv_label_set_text(label, label_text);
  lv_obj_set_width(label, LV_PCT(100));
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(COLOR_MUTED), 0);

  lv_obj_t *matrix = lv_btnmatrix_create(cont);
  lv_obj_set_size(matrix, LV_PCT(100), 38);
  lv_btnmatrix_set_map(matrix, map);
  lv_btnmatrix_set_btn_ctrl_all(matrix, LV_BTNMATRIX_CTRL_CHECKABLE);
  lv_btnmatrix_set_one_checked(matrix, true);
  lv_btnmatrix_set_btn_ctrl(matrix, selected, LV_BTNMATRIX_CTRL_CHECKED);
  lv_obj_add_event_cb(matrix, callback, LV_EVENT_VALUE_CHANGED, user_data);

  lv_obj_set_style_bg_color(matrix, lv_color_hex(COLOR_CARD), LV_PART_MAIN);
  lv_obj_set_style_border_width(matrix, 1, LV_PART_MAIN);
  lv_obj_set_style_border_color(matrix, lv_color_hex(COLOR_BORDER), LV_PART_MAIN);
  lv_obj_set_style_radius(matrix, 8, LV_PART_MAIN);
  lv_obj_set_style_pad_all(matrix, 2, LV_PART_MAIN);
  lv_obj_set_style_bg_color(matrix, lv_color_hex(COLOR_CARD), LV_PART_ITEMS);
  lv_obj_set_style_border_width(matrix, 1, LV_PART_ITEMS);
  lv_obj_set_style_border_color(matrix, lv_color_hex(COLOR_BORDER), LV_PART_ITEMS);
  lv_obj_set_style_text_color(matrix, lv_color_hex(COLOR_TEXT), LV_PART_ITEMS);
  lv_obj_set_style_text_font(matrix, &lv_font_montserrat_12, LV_PART_ITEMS);
  lv_obj_set_style_bg_color(
      matrix,
      lv_color_hex(COLOR_ACCENT),
      LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_set_style_text_color(
      matrix,
      lv_color_hex(COLOR_BG),
      LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_set_style_outline_width(
      matrix,
      0,
      LV_PART_ITEMS | LV_STATE_FOCUS_KEY);

  return matrix;
}

} // namespace

ExtruderPanel::ExtruderPanel(KWebSocketClient &websocket_client,
                             std::mutex &lock,
                             Numpad &numpad,
                             SpoolmanPanel &sm)
  : NotifyConsumer(lock)
  , ws(websocket_client)
  , panel_cont(lv_obj_create(lv_scr_act()))
  , spoolman_panel(sm)
  , extruder_temp(
        ws,
        panel_cont,
        &extruder,
        128,
        "Extruder",
        lv_color_hex(COLOR_ACCENT),
        false,
        true,
        numpad,
        "extruder",
        nullptr,
        nullptr)
  , header_cont(lv_obj_create(panel_cont))
  , back_btn(lv_btn_create(header_cont))
  , title_label(lv_label_create(header_cont))
  , temp_selector(nullptr)
  , length_selector(nullptr)
  , speed_selector(nullptr)
  , temp_idx(6)
  , length_idx(1)
  , speed_idx(2)
  , load_btn(nullptr)
  , unload_btn(nullptr)
  , cooldown_btn(nullptr)
  , spoolman_btn(nullptr)
  , extrude_btn(nullptr)
  , retract_btn(nullptr)
  , load_filament_macro("LOAD_FILAMENT")
  , unload_filament_macro("UNLOAD_FILAMENT")
  , cooldown_macro("SET_HEATER_TEMPERATURE HEATER=extruder TARGET=0")
{
  Config *conf = Config::get_instance();

  auto value = conf->get_json("/default_macros/load_filament");
  if (!value.is_null()) {
    load_filament_macro = value.template get<std::string>();
  }

  value = conf->get_json("/default_macros/unload_filament");
  if (!value.is_null()) {
    unload_filament_macro = value.template get<std::string>();
  }

  value = conf->get_json("/default_macros/cooldown");
  if (!value.is_null()) {
    cooldown_macro = value.template get<std::string>();
  }

  lv_obj_move_background(panel_cont);
  lv_obj_set_size(panel_cont, LV_PCT(100), LV_PCT(100));
  lv_obj_clear_flag(panel_cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(panel_cont, 8, 0);
  lv_obj_set_style_pad_row(panel_cont, 6, 0);
  lv_obj_set_style_border_width(panel_cont, 0, 0);
  lv_obj_set_style_bg_color(panel_cont, lv_color_hex(COLOR_BG), 0);

  static lv_coord_t rows[] = {
    42,
    40,
    58,
    58,
    58,
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  static lv_coord_t cols[] = {
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  lv_obj_set_grid_dsc_array(panel_cont, cols, rows);

  lv_obj_set_grid_cell(
      header_cont,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 0, 1);
  style_transparent_container(header_cont);

  style_button(back_btn);
  lv_obj_set_size(back_btn, 40, 34);
  lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 0, 0);
  lv_obj_add_event_cb(back_btn, &ExtruderPanel::_handle_callback, LV_EVENT_CLICKED, this);

  lv_obj_t *back_label = lv_label_create(back_btn);
  lv_label_set_text(back_label, LV_SYMBOL_LEFT);
  lv_obj_set_style_text_font(back_label, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(back_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(back_label);

  lv_label_set_text(title_label, "Extrude");
  lv_obj_set_style_text_font(title_label, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(title_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(title_label);

  extruder_temp.use_compact_layout();
  lv_obj_t *temp_card = extruder_temp.get_sensor();
  lv_obj_set_grid_cell(
      temp_card,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 1, 1);
  lv_obj_set_style_radius(temp_card, 8, LV_PART_MAIN);
  lv_obj_set_style_border_width(temp_card, 1, LV_PART_MAIN);
  lv_obj_set_style_border_side(temp_card, LV_BORDER_SIDE_FULL, LV_PART_MAIN);
  lv_obj_set_style_border_color(temp_card, lv_color_hex(COLOR_BORDER), LV_PART_MAIN);
  lv_obj_set_style_bg_color(temp_card, lv_color_hex(COLOR_CARD), LV_PART_MAIN);

  lv_obj_t *temp_cont = lv_obj_create(panel_cont);
  lv_obj_set_grid_cell(
      temp_cont,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 2, 1);
  style_transparent_container(temp_cont);
  temp_selector = create_selector(
      temp_cont,
      "Temperature (C)",
      temp_map,
      temp_idx,
      &ExtruderPanel::_handle_selector_cb,
      this);

  lv_obj_t *length_cont = lv_obj_create(panel_cont);
  lv_obj_set_grid_cell(
      length_cont,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 3, 1);
  style_transparent_container(length_cont);
  length_selector = create_selector(
      length_cont,
      "Length (mm)",
      length_map,
      length_idx,
      &ExtruderPanel::_handle_selector_cb,
      this);

  lv_obj_t *speed_cont = lv_obj_create(panel_cont);
  lv_obj_set_grid_cell(
      speed_cont,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 4, 1);
  style_transparent_container(speed_cont);
  speed_selector = create_selector(
      speed_cont,
      "Speed (mm/s)",
      speed_map,
      speed_idx,
      &ExtruderPanel::_handle_selector_cb,
      this);

  lv_obj_t *actions_cont = lv_obj_create(panel_cont);
  lv_obj_set_grid_cell(
      actions_cont,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 5, 1);
  style_transparent_container(actions_cont);
  lv_obj_set_style_pad_row(actions_cont, 6, 0);
  lv_obj_set_style_pad_column(actions_cont, 6, 0);

  static lv_coord_t action_rows[] = {
    LV_GRID_FR(1),
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  static lv_coord_t action_cols[] = {
    LV_GRID_FR(1),
    LV_GRID_FR(1),
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  lv_obj_set_grid_dsc_array(actions_cont, action_cols, action_rows);

  load_btn = create_action_button(
      actions_cont, &load_filament_img, "Load", &ExtruderPanel::_handle_callback, this);
  unload_btn = create_action_button(
      actions_cont, &unload_filament_img, "Unload", &ExtruderPanel::_handle_callback, this);
  cooldown_btn = create_action_button(
      actions_cont, &cooldown_img, "Cooldown", &ExtruderPanel::_handle_callback, this);
  extrude_btn = create_action_button(
      actions_cont, &extrude_img, "Extrude", &ExtruderPanel::_handle_callback, this);
  retract_btn = create_action_button(
      actions_cont, &retract_img, "Retract", &ExtruderPanel::_handle_callback, this);
  spoolman_btn = create_action_button(
      actions_cont, &spoolman_img, "Spoolman", &ExtruderPanel::_handle_callback, this);

  lv_obj_t *action_buttons[] = {
    load_btn,
    unload_btn,
    cooldown_btn,
    extrude_btn,
    retract_btn,
    spoolman_btn,
  };

  for (uint32_t index = 0; index < 6; ++index) {
    lv_obj_set_grid_cell(
        action_buttons[index],
        LV_GRID_ALIGN_STRETCH, index % 3, 1,
        LV_GRID_ALIGN_STRETCH, index / 3, 1);
  }

  set_button_enabled(spoolman_btn, false);
  ws.register_notify_update(this);
}

ExtruderPanel::~ExtruderPanel()
{
  ws.unregister_notify_update(this);

  /*
   * SensorContainer owns and deletes its own LVGL child. Do not delete
   * panel_cont here before member destruction, otherwise that child would
   * already be gone when SensorContainer's destructor runs.
   */
}

void ExtruderPanel::set_button_enabled(lv_obj_t *button, bool enabled)
{
  if (enabled) {
    lv_obj_clear_state(button, LV_STATE_DISABLED);
  } else {
    lv_obj_add_state(button, LV_STATE_DISABLED);
  }
}

const char *ExtruderPanel::selected_value(lv_obj_t *selector, uint32_t index) const
{
  const char *value = lv_btnmatrix_get_btn_text(selector, index);
  return value == nullptr ? "" : value;
}

void ExtruderPanel::foreground()
{
  lv_obj_move_foreground(panel_cont);
}

void ExtruderPanel::enable_spoolman()
{
  set_button_enabled(spoolman_btn, true);
}

void ExtruderPanel::consume(json &j)
{
  std::lock_guard<std::mutex> lock(lv_lock);

  auto target_value = j["/params/0/extruder/target"_json_pointer];
  if (!target_value.is_null()) {
    extruder_temp.update_target(target_value.template get<int>());
  }

  auto temp_value = j["/params/0/extruder/temperature"_json_pointer];
  if (!temp_value.is_null()) {
    extruder_temp.update_value(temp_value.template get<int>());
  }
}

void ExtruderPanel::handle_selector_cb(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED) {
    return;
  }

  lv_obj_t *selector = lv_event_get_current_target(event);
  const uint32_t idx = lv_btnmatrix_get_selected_btn(selector);

  if (idx >= 7) {
    return;
  }

  if (selector == temp_selector) {
    temp_idx = idx;
  } else if (selector == length_selector) {
    length_idx = idx;
  } else if (selector == speed_selector) {
    speed_idx = idx;
  }
}

void ExtruderPanel::handle_callback(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
    return;
  }

  lv_obj_t *button = lv_event_get_current_target(event);

  if (button == back_btn) {
    lv_obj_move_background(panel_cont);
    return;
  }

  if (button == extrude_btn || button == retract_btn) {
    const char *temp = selected_value(temp_selector, temp_idx);
    const char *length = selected_value(length_selector, length_idx);
    const char *speed = selected_value(speed_selector, speed_idx);

    const std::string extrusion =
        button == retract_btn ? fmt::format("-{}", length) : std::string(length);

    ws.gcode_script(fmt::format(
        "M109 S{}\nM83\nG1 E{} F{}",
        temp,
        extrusion,
        std::stoi(speed) * 60));
    return;
  }

  if (button == unload_btn) {
    /*
     * Preserve the old compatibility behavior for configured legacy macros.
     * It can be removed only after the Fre3nder configuration audit proves
     * these names are no longer used by any supported configuration.
     */
    if (unload_filament_macro == "_GUPPY_QUIT_MATERIAL") {
      ws.gcode_script(fmt::format(
          "{} EXTRUDER_TEMP={}",
          unload_filament_macro,
          selected_value(temp_selector, temp_idx)));
    } else {
      ws.gcode_script(unload_filament_macro);
    }
    return;
  }

  if (button == load_btn) {
    if (load_filament_macro == "_GUPPY_LOAD_MATERIAL") {
      ws.gcode_script(fmt::format(
          "{} EXTRUDER_TEMP={} EXTRUDE_LEN={}",
          load_filament_macro,
          selected_value(temp_selector, temp_idx),
          selected_value(length_selector, length_idx)));
    } else {
      ws.gcode_script(load_filament_macro);
    }
    return;
  }

  if (button == cooldown_btn) {
    ws.gcode_script(cooldown_macro);
    return;
  }

  if (button == spoolman_btn) {
    spoolman_panel.foreground();
  }
}
