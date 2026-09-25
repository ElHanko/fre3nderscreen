#include "printertune_panel.h"

#include "state.h"
#include "spdlog/spdlog.h"

#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

namespace {

constexpr uint32_t COLOR_BG = 0x080B0D;
constexpr uint32_t COLOR_CARD = 0x11171B;
constexpr uint32_t COLOR_CARD_PRESSED = 0x18242A;
constexpr uint32_t COLOR_BORDER = 0x25323A;
constexpr uint32_t COLOR_ACCENT = 0x00E5FF;
constexpr uint32_t COLOR_TEXT = 0xFFFFFF;

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
  lv_obj_set_style_opa(button, LV_OPA_50, LV_STATE_DISABLED);
}

} // namespace

PrinterTunePanel::PrinterTunePanel(KWebSocketClient &client,
                                   std::mutex &lock,
                                   FineTunePanel &finetune)
  : cont(lv_obj_create(lv_scr_act()))
  , header_cont(lv_obj_create(cont))
  , back_btn(lv_btn_create(header_cont))
  , title_label(lv_label_create(header_cont))
  , cards_cont(lv_obj_create(cont))
  , bedmesh_panel(client, lock)
  , finetune_panel(finetune)
  , limits_panel(client, lock)
  , inputshaper_panel(client, lock)
  , belts_calibration_panel(client, lock)
  , tmc_tune_panel(client)
  , tmc_status_panel(client, lock)
  , power_panel(client, lock)
  , bedmesh_btn(lv_btn_create(cards_cont))
  , finetune_btn(lv_btn_create(cards_cont))
  , inputshaper_btn(lv_btn_create(cards_cont))
  , belts_calibration_btn(lv_btn_create(cards_cont))
  , limits_btn(lv_btn_create(cards_cont))
  , tmc_tune_btn(lv_btn_create(cards_cont))
  , tmc_status_btn(lv_btn_create(cards_cont))
  , power_devices_btn(lv_btn_create(cards_cont))
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

  style_button(back_btn);
  lv_obj_set_size(back_btn, 40, 34);
  lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 0, 0);
  lv_obj_add_event_cb(
      back_btn,
      &PrinterTunePanel::_handle_callback,
      LV_EVENT_CLICKED,
      this);

  lv_obj_t *back_label = lv_label_create(back_btn);
  lv_label_set_text(back_label, LV_SYMBOL_LEFT);
  lv_obj_set_style_text_font(back_label, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(back_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(back_label);

  lv_label_set_text(title_label, "Tune");
  lv_obj_set_style_text_font(title_label, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(title_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(title_label);

  lv_obj_set_grid_cell(
      cards_cont,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 1, 1);
  style_transparent(cards_cont);
  lv_obj_set_style_pad_row(cards_cont, 8, 0);
  lv_obj_set_style_pad_column(cards_cont, 8, 0);

  static lv_coord_t rows[] = {
    LV_GRID_FR(1),
    LV_GRID_FR(1),
    LV_GRID_FR(1),
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  static lv_coord_t cols[] = {
    LV_GRID_FR(1),
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  lv_obj_set_grid_dsc_array(cards_cont, cols, rows);

  struct TuneDef {
    lv_obj_t *button;
    const char *icon;
    const char *text;
    uint8_t col;
    uint8_t row;
  };

  TuneDef buttons[] = {
    {finetune_btn, LV_SYMBOL_EDIT, "Fine Tune", 0, 0},
    {limits_btn, LV_SYMBOL_SETTINGS, "Limits", 1, 0},
    {bedmesh_btn, LV_SYMBOL_IMAGE, "Bed Mesh", 0, 1},
    {inputshaper_btn, LV_SYMBOL_LOOP, "Input Shaper", 1, 1},
    {belts_calibration_btn, LV_SYMBOL_REFRESH, "Belts Calibration", 0, 2},
    {tmc_tune_btn, LV_SYMBOL_CHARGE, "TMC Tune", 1, 2},
    {tmc_status_btn, LV_SYMBOL_LIST, "TMC Status", 0, 3},
    {power_devices_btn, LV_SYMBOL_POWER, "Power", 1, 3},
  };

  for (const auto &entry : buttons) {
    style_button(entry.button);
    lv_obj_set_style_pad_all(entry.button, 6, 0);
    lv_obj_set_style_pad_row(entry.button, 3, 0);
    lv_obj_set_flex_flow(entry.button, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(
        entry.button,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER);
    lv_obj_set_grid_cell(
        entry.button,
        LV_GRID_ALIGN_STRETCH, entry.col, 1,
        LV_GRID_ALIGN_STRETCH, entry.row, 1);
    lv_obj_add_event_cb(
        entry.button,
        &PrinterTunePanel::_handle_callback,
        LV_EVENT_CLICKED,
        this);

    lv_obj_t *icon = lv_label_create(entry.button);
    lv_label_set_text(icon, entry.icon);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(icon, lv_color_hex(COLOR_ACCENT), 0);

    lv_obj_t *label = lv_label_create(entry.button);
    lv_label_set_text(label, entry.text);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label, LV_PCT(100));
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(COLOR_TEXT), 0);
  }

  lv_obj_add_state(tmc_tune_btn, LV_STATE_DISABLED);
}

PrinterTunePanel::~PrinterTunePanel()
{
  if (cont != nullptr) {
    lv_obj_del(cont);
    cont = nullptr;
  }
}

lv_obj_t *PrinterTunePanel::get_container()
{
  return cont;
}

BedMeshPanel &PrinterTunePanel::get_bedmesh_panel()
{
  return bedmesh_panel;
}

PowerPanel &PrinterTunePanel::get_power_panel()
{
  return power_panel;
}

void PrinterTunePanel::foreground()
{
  lv_obj_move_foreground(cont);
}

void PrinterTunePanel::init(json &j)
{
  limits_panel.init(j);
  tmc_status_panel.init(j);

  // Existing host-path capability check; platform/API cleanup is separate.
  State *state = State::get_instance();
  auto klipper_path =
      state->get_data("/printer_info/klipper_path"_json_pointer);

  if (klipper_path.is_null()) {
    lv_obj_add_state(tmc_tune_btn, LV_STATE_DISABLED);
    return;
  }

  auto motor_db =
      fs::path(klipper_path.template get<std::string>()) /
      "klippy/extras/motor_database.cfg";

  if (fs::exists(motor_db)) {
    lv_obj_clear_state(tmc_tune_btn, LV_STATE_DISABLED);
    tmc_tune_panel.init(j, motor_db);
  } else {
    lv_obj_add_state(tmc_tune_btn, LV_STATE_DISABLED);
  }
}

void PrinterTunePanel::handle_callback(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
    return;
  }

  lv_obj_t *button = lv_event_get_current_target(event);

  if (button == back_btn) {
    lv_obj_move_background(cont);
  } else if (button == finetune_btn) {
    spdlog::trace("tune finetune pressed");
    finetune_panel.foreground();
  } else if (button == limits_btn) {
    spdlog::trace("limits pressed");
    limits_panel.foreground();
  } else if (button == bedmesh_btn) {
    spdlog::trace("tune bedmesh pressed");
    bedmesh_panel.foreground();
  } else if (button == inputshaper_btn) {
    spdlog::trace("tune inputshaper pressed");
    inputshaper_panel.foreground();
  } else if (button == belts_calibration_btn) {
    spdlog::trace("tune belts pressed");
    belts_calibration_panel.foreground();
  } else if (button == tmc_tune_btn) {
    spdlog::trace("tmc auto tune pressed");
    tmc_tune_panel.foreground();
  } else if (button == tmc_status_btn) {
    spdlog::trace("tmc metrics pressed");
    tmc_status_panel.foreground();
  } else if (button == power_devices_btn) {
    spdlog::trace("power devices pressed");
    power_panel.foreground();
  }
}
