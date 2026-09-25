#include "print_status_panel.h"

#include "config.h"
#include "state.h"
#include "utils.h"
#include "spdlog/fmt/fmt.h"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace {

constexpr uint32_t COLOR_BG = 0x080B0D;
constexpr uint32_t COLOR_CARD = 0x11171B;
constexpr uint32_t COLOR_CARD_PRESSED = 0x18242A;
constexpr uint32_t COLOR_BORDER = 0x25323A;
constexpr uint32_t COLOR_ACCENT = 0x00E5FF;
constexpr uint32_t COLOR_TEXT = 0xFFFFFF;
constexpr uint32_t COLOR_MUTED = 0xB8C0C5;
constexpr uint32_t COLOR_DANGER = 0xFF4D5A;

double pi()
{
  return std::atan(1) * 4;
}

void style_transparent(lv_obj_t *obj)
{
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
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

void style_button(lv_obj_t *button, bool danger = false)
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
      lv_color_hex(danger ? COLOR_DANGER : COLOR_ACCENT),
      LV_STATE_PRESSED);
  lv_obj_set_style_opa(button, LV_OPA_40, LV_STATE_DISABLED);
}

lv_obj_t *create_metric_card(lv_obj_t *parent,
                             const char *title,
                             lv_obj_t **value_out)
{
  lv_obj_t *card = lv_obj_create(parent);
  style_card(card);
  lv_obj_set_style_pad_all(card, 7, 0);

  lv_obj_t *title_label = lv_label_create(card);
  lv_label_set_text(title_label, title);
  lv_obj_set_style_text_font(title_label, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(
      title_label,
      lv_color_hex(COLOR_MUTED),
      0);
  lv_obj_align(title_label, LV_ALIGN_TOP_LEFT, 0, 0);

  lv_obj_t *value = lv_label_create(card);
  lv_label_set_text(value, "--");
  lv_label_set_long_mode(value, LV_LABEL_LONG_DOT);
  lv_obj_set_width(value, LV_PCT(100));
  lv_obj_set_style_text_align(value, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_style_text_font(value, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(value, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_align(value, LV_ALIGN_BOTTOM_LEFT, 0, 0);

  *value_out = value;
  return card;
}

lv_obj_t *create_action_button(lv_obj_t *parent,
                               const char *icon_text,
                               const char *label_text,
                               bool danger = false)
{
  lv_obj_t *button = lv_btn_create(parent);
  style_button(button, danger);
  lv_obj_set_style_pad_all(button, 5, 0);
  lv_obj_set_style_pad_row(button, 3, 0);
  lv_obj_set_flex_flow(button, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(
      button,
      LV_FLEX_ALIGN_CENTER,
      LV_FLEX_ALIGN_CENTER,
      LV_FLEX_ALIGN_CENTER);

  lv_obj_t *icon = lv_label_create(button);
  lv_label_set_text(icon, icon_text);
  lv_obj_set_style_text_font(icon, &lv_font_montserrat_18, 0);
  lv_obj_set_style_text_color(
      icon,
      lv_color_hex(danger ? COLOR_DANGER : COLOR_ACCENT),
      0);

  lv_obj_t *label = lv_label_create(button);
  lv_label_set_text(label, label_text);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(COLOR_TEXT), 0);

  return button;
}

std::string display_filename(const std::string &path)
{
  const size_t slash = path.find_last_of("/\\");
  return slash == std::string::npos ? path : path.substr(slash + 1);
}

} // namespace

PrintStatusPanel::PrintStatusPanel(KWebSocketClient &websocket_client,
                                   std::mutex &lock,
                                   lv_obj_t *mini_parent)
  : NotifyConsumer(lock)
  , ws(websocket_client)
  , finetune_panel(websocket_client, lock)
  , mini_print_status(
        mini_parent,
        &PrintStatusPanel::_handle_callback,
        this)
  , status_cont(lv_obj_create(lv_scr_act()))
  , header_cont(lv_obj_create(status_cont))
  , back_btn(lv_btn_create(header_cont))
  , title_label(lv_label_create(header_cont))
  , thumbnail_cont(lv_obj_create(status_cont))
  , thumbnail(lv_img_create(thumbnail_cont))
  , pbar_cont(lv_obj_create(thumbnail_cont))
  , filename_label(lv_label_create(pbar_cont))
  , progress_bar(lv_bar_create(pbar_cont))
  , progress_label(lv_label_create(pbar_cont))
  , detail_cont(lv_obj_create(status_cont))
  , extruder_temp(nullptr)
  , bed_temp(nullptr)
  , print_speed(nullptr)
  , z_offset(nullptr)
  , flow_rate(nullptr)
  , layers(nullptr)
  , fan0(nullptr)
  , elapsed(nullptr)
  , time_left(nullptr)
  , buttons_cont(lv_obj_create(status_cont))
  , finetune_btn(nullptr)
  , pause_btn(nullptr)
  , resume_btn(nullptr)
  , cancel_btn(nullptr)
  , emergency_btn(nullptr)
  , confirm_overlay(lv_obj_create(lv_scr_act()))
  , confirm_dialog(lv_obj_create(confirm_overlay))
  , confirm_message(lv_label_create(confirm_dialog))
  , confirm_yes_btn(lv_btn_create(confirm_dialog))
  , confirm_no_btn(lv_btn_create(confirm_dialog))
  , pending_confirmation(ConfirmAction::None)
  , estimated_time_s(0)
  , filament_diameter(1.75)
  , extruder_target(-1)
  , heater_bed_target(-1)
{
  lv_obj_move_background(status_cont);
  lv_obj_set_size(status_cont, LV_PCT(100), LV_PCT(100));
  lv_obj_clear_flag(status_cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(status_cont, 8, 0);
  lv_obj_set_style_pad_row(status_cont, 6, 0);
  lv_obj_set_style_border_width(status_cont, 0, 0);
  lv_obj_set_style_bg_color(status_cont, lv_color_hex(COLOR_BG), 0);

  static lv_coord_t root_rows[] = {
    42,
    96,
    LV_GRID_FR(1),
    104,
    LV_GRID_TEMPLATE_LAST
  };
  static lv_coord_t root_cols[] = {
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  lv_obj_set_grid_dsc_array(status_cont, root_cols, root_rows);

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
      &PrintStatusPanel::_handle_callback,
      LV_EVENT_CLICKED,
      this);

  lv_obj_t *back_label = lv_label_create(back_btn);
  lv_label_set_text(back_label, LV_SYMBOL_LEFT);
  lv_obj_set_style_text_font(back_label, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(back_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(back_label);

  lv_label_set_text(title_label, "Print Status");
  lv_obj_set_style_text_font(title_label, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(title_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(title_label);

  style_card(thumbnail_cont);
  lv_obj_set_grid_cell(
      thumbnail_cont,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 1, 1);
  lv_obj_set_style_pad_all(thumbnail_cont, 8, 0);

  static lv_coord_t summary_rows[] = {
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  static lv_coord_t summary_cols[] = {
    76,
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  lv_obj_set_grid_dsc_array(
      thumbnail_cont,
      summary_cols,
      summary_rows);

  lv_obj_set_grid_cell(
      thumbnail,
      LV_GRID_ALIGN_CENTER, 0, 1,
      LV_GRID_ALIGN_CENTER, 0, 1);
  lv_img_set_size_mode(thumbnail, LV_IMG_SIZE_MODE_REAL);

  lv_obj_set_grid_cell(
      pbar_cont,
      LV_GRID_ALIGN_STRETCH, 1, 1,
      LV_GRID_ALIGN_STRETCH, 0, 1);
  style_transparent(pbar_cont);
  lv_obj_set_style_pad_left(pbar_cont, 8, 0);
  lv_obj_set_style_pad_row(pbar_cont, 7, 0);
  lv_obj_set_flex_flow(pbar_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(
      pbar_cont,
      LV_FLEX_ALIGN_CENTER,
      LV_FLEX_ALIGN_START,
      LV_FLEX_ALIGN_CENTER);

  lv_label_set_text(filename_label, "No active print");
  lv_label_set_long_mode(filename_label, LV_LABEL_LONG_DOT);
  lv_obj_set_width(filename_label, LV_PCT(100));
  lv_obj_set_style_text_font(filename_label, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(filename_label, lv_color_hex(COLOR_TEXT), 0);

  lv_obj_set_size(progress_bar, LV_PCT(100), 14);
  lv_bar_set_range(progress_bar, 0, 100);
  lv_bar_set_value(progress_bar, 0, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(
      progress_bar,
      lv_color_hex(COLOR_CARD_PRESSED),
      LV_PART_MAIN);
  lv_obj_set_style_bg_color(
      progress_bar,
      lv_color_hex(COLOR_ACCENT),
      LV_PART_INDICATOR);

  lv_label_set_text(progress_label, "0%");
  lv_obj_set_style_text_font(progress_label, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(progress_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_set_style_text_align(progress_label, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_width(progress_label, LV_PCT(100));

  lv_obj_set_grid_cell(
      detail_cont,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 2, 1);
  style_transparent(detail_cont);
  lv_obj_set_style_pad_row(detail_cont, 6, 0);
  lv_obj_set_style_pad_column(detail_cont, 6, 0);

  static lv_coord_t detail_rows[] = {
    LV_GRID_FR(1),
    LV_GRID_FR(1),
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  static lv_coord_t detail_cols[] = {
    LV_GRID_FR(1),
    LV_GRID_FR(1),
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  lv_obj_set_grid_dsc_array(detail_cont, detail_cols, detail_rows);

  struct MetricDef {
    const char *title;
    lv_obj_t **value;
    uint8_t col;
    uint8_t row;
  };

  MetricDef metrics[] = {
    {"Extruder", &extruder_temp, 0, 0},
    {"Bed", &bed_temp, 1, 0},
    {"Speed", &print_speed, 2, 0},
    {"Z Offset", &z_offset, 0, 1},
    {"Flow", &flow_rate, 1, 1},
    {"Layers", &layers, 2, 1},
    {"Fan", &fan0, 0, 2},
    {"Elapsed", &elapsed, 1, 2},
    {"ETA", &time_left, 2, 2},
  };

  for (auto &metric : metrics) {
    lv_obj_t *card = create_metric_card(
        detail_cont,
        metric.title,
        metric.value);
    lv_obj_set_grid_cell(
        card,
        LV_GRID_ALIGN_STRETCH, metric.col, 1,
        LV_GRID_ALIGN_STRETCH, metric.row, 1);
  }

  lv_obj_set_grid_cell(
      buttons_cont,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 3, 1);
  style_transparent(buttons_cont);
  lv_obj_set_style_pad_row(buttons_cont, 6, 0);
  lv_obj_set_style_pad_column(buttons_cont, 6, 0);

  static lv_coord_t action_rows[] = {
    LV_GRID_FR(1),
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  static lv_coord_t action_cols[] = {
    LV_GRID_FR(1),
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  lv_obj_set_grid_dsc_array(
      buttons_cont,
      action_cols,
      action_rows);

  finetune_btn = create_action_button(
      buttons_cont,
      LV_SYMBOL_SETTINGS,
      "Fine Tune");
  pause_btn = create_action_button(
      buttons_cont,
      LV_SYMBOL_PAUSE,
      "Pause");
  resume_btn = create_action_button(
      buttons_cont,
      LV_SYMBOL_PLAY,
      "Resume");
  cancel_btn = create_action_button(
      buttons_cont,
      LV_SYMBOL_CLOSE,
      "Cancel",
      true);
  emergency_btn = create_action_button(
      buttons_cont,
      LV_SYMBOL_WARNING,
      "Stop",
      true);

  lv_obj_set_grid_cell(
      finetune_btn,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 0, 1);
  lv_obj_set_grid_cell(
      pause_btn,
      LV_GRID_ALIGN_STRETCH, 1, 1,
      LV_GRID_ALIGN_STRETCH, 0, 1);
  lv_obj_set_grid_cell(
      resume_btn,
      LV_GRID_ALIGN_STRETCH, 1, 1,
      LV_GRID_ALIGN_STRETCH, 0, 1);
  lv_obj_set_grid_cell(
      cancel_btn,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 1, 1);
  lv_obj_set_grid_cell(
      emergency_btn,
      LV_GRID_ALIGN_STRETCH, 1, 1,
      LV_GRID_ALIGN_STRETCH, 1, 1);

  lv_obj_add_flag(resume_btn, LV_OBJ_FLAG_HIDDEN);
  set_button_enabled(resume_btn, false);

  lv_obj_t *actions[] = {
    finetune_btn,
    pause_btn,
    resume_btn,
    cancel_btn,
    emergency_btn,
  };
  for (lv_obj_t *button : actions) {
    lv_obj_add_event_cb(
        button,
        &PrintStatusPanel::_handle_callback,
        LV_EVENT_CLICKED,
        this);
  }

  lv_obj_set_size(confirm_overlay, LV_PCT(100), LV_PCT(100));
  lv_obj_clear_flag(confirm_overlay, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(confirm_overlay, 0, 0);
  lv_obj_set_style_border_width(confirm_overlay, 0, 0);
  lv_obj_set_style_radius(confirm_overlay, 0, 0);
  lv_obj_set_style_bg_color(
      confirm_overlay,
      lv_color_hex(0x000000),
      0);
  lv_obj_set_style_bg_opa(confirm_overlay, LV_OPA_70, 0);
  lv_obj_add_flag(confirm_overlay, LV_OBJ_FLAG_HIDDEN);

  lv_obj_set_size(confirm_dialog, LV_PCT(90), 170);
  lv_obj_center(confirm_dialog);
  style_card(confirm_dialog);
  lv_obj_set_style_pad_all(confirm_dialog, 12, 0);

  lv_obj_set_width(confirm_message, LV_PCT(100));
  lv_label_set_long_mode(confirm_message, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(
      confirm_message,
      LV_TEXT_ALIGN_CENTER,
      0);
  lv_obj_set_style_text_font(
      confirm_message,
      &lv_font_montserrat_16,
      0);
  lv_obj_set_style_text_color(
      confirm_message,
      lv_color_hex(COLOR_TEXT),
      0);
  lv_obj_align(confirm_message, LV_ALIGN_TOP_MID, 0, 14);

  style_button(confirm_yes_btn, true);
  lv_obj_set_size(confirm_yes_btn, 100, 46);
  lv_obj_align(confirm_yes_btn, LV_ALIGN_BOTTOM_LEFT, 0, 0);
  lv_obj_t *confirm_yes_label = lv_label_create(confirm_yes_btn);
  lv_label_set_text(confirm_yes_label, "Confirm");
  lv_obj_set_style_text_color(
      confirm_yes_label,
      lv_color_hex(COLOR_DANGER),
      0);
  lv_obj_center(confirm_yes_label);

  style_button(confirm_no_btn);
  lv_obj_set_size(confirm_no_btn, 100, 46);
  lv_obj_align(confirm_no_btn, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
  lv_obj_t *confirm_no_label = lv_label_create(confirm_no_btn);
  lv_label_set_text(confirm_no_label, "Cancel");
  lv_obj_set_style_text_color(
      confirm_no_label,
      lv_color_hex(COLOR_TEXT),
      0);
  lv_obj_center(confirm_no_label);

  lv_obj_add_event_cb(
      confirm_yes_btn,
      &PrintStatusPanel::_handle_callback,
      LV_EVENT_CLICKED,
      this);
  lv_obj_add_event_cb(
      confirm_no_btn,
      &PrintStatusPanel::_handle_callback,
      LV_EVENT_CLICKED,
      this);

  ws.register_notify_update(this);
}

PrintStatusPanel::~PrintStatusPanel()
{
  if (confirm_overlay != nullptr) {
    lv_obj_del(confirm_overlay);
    confirm_overlay = nullptr;
  }

  if (status_cont != nullptr) {
    lv_obj_del(status_cont);
    status_cont = nullptr;
  }

  ws.unregister_notify_update(this);
}

void PrintStatusPanel::set_button_enabled(lv_obj_t *button, bool enabled)
{
  if (enabled) {
    lv_obj_clear_state(button, LV_STATE_DISABLED);
  } else {
    lv_obj_add_state(button, LV_STATE_DISABLED);
  }
}

void PrintStatusPanel::foreground()
{
  lv_obj_move_foreground(status_cont);
}

void PrintStatusPanel::background()
{
  close_confirmation();
  lv_obj_move_background(status_cont);
}

void PrintStatusPanel::close_confirmation()
{
  pending_confirmation = ConfirmAction::None;
  lv_obj_add_flag(confirm_overlay, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_background(confirm_overlay);
}

void PrintStatusPanel::execute_confirmation()
{
  switch (pending_confirmation) {
    case ConfirmAction::CancelPrint:
      ws.send_jsonrpc("printer.print.cancel");
      break;
    case ConfirmAction::EmergencyStop:
      ws.send_jsonrpc("printer.emergency_stop");
      break;
    case ConfirmAction::None:
      break;
  }
}

void PrintStatusPanel::request_confirmation(ConfirmAction action,
                                            const char *message)
{
  Config *config = Config::get_instance();
  auto value = config->get_json("/prompt_emergency_stop");
  const bool prompt =
      !value.is_null() && value.template get<bool>();

  if (!prompt) {
    pending_confirmation = action;
    execute_confirmation();
    pending_confirmation = ConfirmAction::None;
    return;
  }

  pending_confirmation = action;
  lv_label_set_text(confirm_message, message);
  lv_obj_clear_flag(confirm_overlay, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(confirm_overlay);
}

void PrintStatusPanel::reset()
{
  lv_bar_set_value(progress_bar, 0, LV_ANIM_OFF);
  lv_label_set_text(progress_label, "0%");
  lv_label_set_text(filename_label, "No active print");
  lv_label_set_text(extruder_temp, "--");
  lv_label_set_text(bed_temp, "--");
  lv_label_set_text(print_speed, "0 mm/s");
  lv_label_set_text(z_offset, "--");
  lv_label_set_text(flow_rate, "0.0 mm3/s");
  lv_label_set_text(layers, "...");
  lv_label_set_text(fan0, "--");
  lv_label_set_text(elapsed, "0s");
  lv_label_set_text(time_left, "...");

  estimated_time_s = 0;

  auto value = State::get_instance()->get_data(
      "/printer_state/configfile/config/extruder/filament_diameter"_json_pointer);
  filament_diameter =
      value.is_null()
          ? 1.750
          : std::stod(value.template get<std::string>());

  extruder_target = -1;
  heater_bed_target = -1;

  lv_img_set_src(thumbnail, nullptr);
  mini_print_status.reset();
}

void PrintStatusPanel::init(json &fans)
{
  fan_speeds.clear();
  std::vector<std::string> values;

  for (auto &entry : fans.items()) {
    std::string fan_name = entry.key();

    auto fan_value = State::get_instance()->get_data(
        json::json_pointer(
            fmt::format("/printer_state/{}/value", fan_name)));
    if (!fan_value.is_null()) {
      int speed =
          static_cast<int>(fan_value.template get<double>() * 100);
      fan_speeds.insert({fan_name, speed});
      values.push_back(fmt::format("{}%", speed));
    }

    fan_value = State::get_instance()->get_data(
        json::json_pointer(
            fmt::format("/printer_state/{}/speed", fan_name)));
    if (!fan_value.is_null()) {
      int speed =
          static_cast<int>(fan_value.template get<double>() * 100);
      fan_speeds.insert({fan_name, speed});
      values.push_back(fmt::format("{}%", speed));
    }
  }

  lv_label_set_text(
      fan0,
      values.empty() ? "--" : fmt::format("{}", fmt::join(values, ", ")).c_str());

  reset();
  populate();

  json &state = State::get_instance()->get_data(
      "/printer_state/print_stats/state"_json_pointer);
  if (!state.is_null()) {
    auto print_status = state.template get<std::string>();
    if (print_status != "printing" && print_status != "paused") {
      mini_print_status.hide();
    }
    mini_print_status.update_status(print_status);
  } else {
    mini_print_status.show();
  }
}

void PrintStatusPanel::populate()
{
  State *state = State::get_instance();

  json &printfile = state->get_data(
      "/printer_state/print_stats/filename"_json_pointer);
  if (!printfile.is_null()) {
    const std::string filename = printfile.template get<std::string>();
    if (!filename.empty()) {
      lv_label_set_text(
          filename_label,
          display_filename(filename).c_str());

      json input = {{"filename", filename}};
      ws.send_jsonrpc(
          "server.files.metadata",
          input,
          [filename, this](json &data) {
            this->handle_metadata(filename, data);
          });

      mini_print_status.show();
    }
  }

  auto &print_state = state->get_data(
      "/printer_state/print_stats/state"_json_pointer);
  const bool paused =
      !print_state.is_null() &&
      print_state.template get<std::string>() == "paused";

  if (paused) {
    lv_obj_clear_flag(resume_btn, LV_OBJ_FLAG_HIDDEN);
    set_button_enabled(resume_btn, true);
    lv_obj_add_flag(pause_btn, LV_OBJ_FLAG_HIDDEN);
    set_button_enabled(pause_btn, false);
  } else {
    lv_obj_add_flag(resume_btn, LV_OBJ_FLAG_HIDDEN);
    set_button_enabled(resume_btn, false);
    lv_obj_clear_flag(pause_btn, LV_OBJ_FLAG_HIDDEN);
    set_button_enabled(pause_btn, true);
  }

  auto value = state->get_data(
      "/printer_state/virtual_sdcard/progress"_json_pointer);
  if (!value.is_null()) {
    int progress =
        static_cast<int>(value.template get<double>() * 100);
    lv_bar_set_value(progress_bar, progress, LV_ANIM_ON);
    lv_label_set_text(
        progress_label,
        fmt::format("{}%", progress).c_str());
    mini_print_status.update_progress(progress);
  }

  value = state->get_data(
      "/printer_state/gcode_move/homing_origin/2"_json_pointer);
  if (!value.is_null()) {
    lv_label_set_text(
        z_offset,
        fmt::format("{:.5} mm",
                    value.template get<double>()).c_str());
  }
}

void PrintStatusPanel::handle_metadata(const std::string &gcode_file,
                                       json &j)
{
  auto eta = j["/result/estimated_time"_json_pointer];
  if (!eta.is_null()) {
    estimated_time_s =
        static_cast<uint32_t>(eta.template get<float>());

    json &value = State::get_instance()->get_data(
        "/printer_state/print_stats/print_duration"_json_pointer);
    if (!value.is_null()) {
      uint32_t passed =
          static_cast<uint32_t>(value.template get<float>());
      std::lock_guard<std::mutex> lock(lv_lock);
      update_time_progress(passed);
    }
  }

  current_file = j["/result"_json_pointer];

  auto thumb_detail = KUtils::get_thumbnail(gcode_file, j, 1.0);
  std::string fullpath = thumb_detail.first;

  if (!fullpath.empty() && thumb_detail.second > 0) {
    std::lock_guard<std::mutex> lock(lv_lock);
    const std::string image_path = "A:" + fullpath;

    const uint32_t zoom =
        static_cast<uint32_t>(
            (70.0 / static_cast<double>(thumb_detail.second)) * 256.0);

    lv_img_set_src(thumbnail, image_path.c_str());
    lv_img_set_zoom(thumbnail, zoom);
    mini_print_status.update_img(image_path, thumb_detail.second);
  }
}

void PrintStatusPanel::consume(json &j)
{
  std::lock_guard<std::mutex> lock(lv_lock);

  auto printfile = j["/params/0/print_stats/filename"_json_pointer];
  if (!printfile.is_null()) {
    reset();

    const std::string filename =
        printfile.template get<std::string>();
    if (!filename.empty()) {
      lv_label_set_text(
          filename_label,
          display_filename(filename).c_str());
    }

    populate();
    foreground();
  }

  auto &print_state = j["/params/0/print_stats/state"_json_pointer];
  if (!print_state.is_null()) {
    auto status = print_state.template get<std::string>();
    if (status != "printing" && status != "paused") {
      mini_print_status.hide();
    } else {
      mini_print_status.show();
    }
    mini_print_status.update_status(status);
  }

  auto value = j["/params/0/extruder/target"_json_pointer];
  if (!value.is_null()) {
    extruder_target = value.template get<int>();
  }

  value = j["/params/0/heater_bed/target"_json_pointer];
  if (!value.is_null()) {
    heater_bed_target = value.template get<int>();
  }

  value = j["/params/0/extruder/temperature"_json_pointer];
  if (!value.is_null()) {
    if (extruder_target > 0) {
      lv_label_set_text(
          extruder_temp,
          fmt::format("{} / {}",
                      value.template get<int>(),
                      extruder_target).c_str());
    } else {
      lv_label_set_text(
          extruder_temp,
          fmt::format("{}",
                      value.template get<int>()).c_str());
    }
  }

  value = j["/params/0/heater_bed/temperature"_json_pointer];
  if (!value.is_null()) {
    if (heater_bed_target > 0) {
      lv_label_set_text(
          bed_temp,
          fmt::format("{} / {}",
                      value.template get<int>(),
                      heater_bed_target).c_str());
    } else {
      lv_label_set_text(
          bed_temp,
          fmt::format("{}",
                      value.template get<int>()).c_str());
    }
  }

  auto speed = j["/params/0/motion_report/live_velocity"_json_pointer];
  if (!speed.is_null()) {
    lv_label_set_text(
        print_speed,
        fmt::format("{} mm/s",
                    static_cast<int>(
                        speed.template get<double>())).c_str());
  }

  value = j["/params/0/gcode_move/homing_origin/2"_json_pointer];
  if (!value.is_null()) {
    lv_label_set_text(
        z_offset,
        fmt::format("{:.5} mm",
                    value.template get<double>()).c_str());
  }

  std::vector<std::string> fan_values;
  for (auto &entry : fan_speeds) {
    const std::string fan_name = entry.first;
    int speed_value = entry.second;

    auto fan_value = j[json::json_pointer(
        fmt::format("/params/0/{}/value", fan_name))];
    if (!fan_value.is_null()) {
      speed_value =
          static_cast<int>(
              fan_value.template get<double>() * 100);
      entry.second = speed_value;
    }

    fan_value = j[json::json_pointer(
        fmt::format("/params/0/{}/speed", fan_name))];
    if (!fan_value.is_null()) {
      speed_value =
          static_cast<int>(
              fan_value.template get<double>() * 100);
      entry.second = speed_value;
    }

    fan_values.push_back(
        fmt::format("{}%", speed_value));
  }

  lv_label_set_text(
      fan0,
      fan_values.empty()
          ? "--"
          : fmt::format("{}", fmt::join(fan_values, ", ")).c_str());

  value = j["/params/0/print_stats/print_duration"_json_pointer];
  if (!value.is_null()) {
    uint32_t passed =
        static_cast<uint32_t>(value.template get<float>());
    update_time_progress(passed);
  }

  value = j["/params/0/virtual_sdcard/progress"_json_pointer];
  if (!value.is_null()) {
    int progress =
        static_cast<int>(value.template get<double>() * 100);
    lv_bar_set_value(progress_bar, progress, LV_ANIM_ON);
    lv_label_set_text(
        progress_label,
        fmt::format("{}%", progress).c_str());
    mini_print_status.update_progress(progress);
  }

  value = j[
      "/params/0/motion_report/live_extruder_velocity"_json_pointer];
  if (!value.is_null()) {
    double flow_value =
        pi() / 4 *
        std::pow(filament_diameter, 2) *
        value.template get<double>();

    lv_label_set_text(
        flow_rate,
        fmt::format(
            "{:.1f} mm3/s",
            flow_value > 0.0 ? flow_value : 0.0).c_str());
  }

  value = j["/params/0/pause_resume/is_paused"_json_pointer];
  if (!value.is_null()) {
    const bool paused = value.template get<bool>();

    if (paused) {
      lv_obj_clear_flag(resume_btn, LV_OBJ_FLAG_HIDDEN);
      set_button_enabled(resume_btn, true);
      lv_obj_add_flag(pause_btn, LV_OBJ_FLAG_HIDDEN);
      set_button_enabled(pause_btn, false);
    } else {
      lv_obj_clear_flag(pause_btn, LV_OBJ_FLAG_HIDDEN);
      set_button_enabled(pause_btn, true);
      lv_obj_add_flag(resume_btn, LV_OBJ_FLAG_HIDDEN);
      set_button_enabled(resume_btn, false);
    }
  }

  value = j["/params/0/print_stats/info"_json_pointer];
  update_layers(value);
}

void PrintStatusPanel::handle_callback(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
    return;
  }

  lv_obj_t *button = lv_event_get_current_target(event);

  if (button == confirm_yes_btn) {
    execute_confirmation();
    close_confirmation();
    return;
  }

  if (button == confirm_no_btn) {
    close_confirmation();
    return;
  }

  if (button == back_btn) {
    background();
  } else if (button == emergency_btn) {
    request_confirmation(
        ConfirmAction::EmergencyStop,
        "Do you want to emergency stop?");
  } else if (button == pause_btn) {
    ws.send_jsonrpc("printer.print.pause");
    set_button_enabled(pause_btn, false);
  } else if (button == resume_btn) {
    ws.send_jsonrpc("printer.print.resume");
    set_button_enabled(resume_btn, false);
  } else if (button == cancel_btn) {
    request_confirmation(
        ConfirmAction::CancelPrint,
        "Do you want to cancel the print?");
  } else if (button == finetune_btn) {
    finetune_panel.foreground();
  } else if (button == mini_print_status.get_container()) {
    foreground();
  }
}

void PrintStatusPanel::update_time_progress(uint32_t time_passed)
{
  int32_t remaining =
      static_cast<int32_t>(estimated_time_s) -
      static_cast<int32_t>(time_passed);

  if (remaining < 0) {
    lv_label_set_text(time_left, "...");
  } else {
    auto eta = KUtils::eta_string(remaining);
    lv_label_set_text(time_left, eta.c_str());
    mini_print_status.update_eta(eta);
  }

  lv_label_set_text(
      elapsed,
      KUtils::eta_string(time_passed).c_str());
}

void PrintStatusPanel::update_layers(json &info)
{
  lv_label_set_text(
      layers,
      fmt::format(
          "{} / {}",
          current_layer(info),
          max_layer(info)).c_str());
}

int PrintStatusPanel::max_layer(json &info)
{
  if (!info.is_null()) {
    auto value = info["/total_layer"_json_pointer];
    if (!value.is_null()) {
      return value.template get<int>();
    }
  }

  if (!current_file.is_null()) {
    auto value = current_file["/layer_count"_json_pointer];
    if (!value.is_null()) {
      return value.template get<int>();
    }

    auto first_layer_height =
        current_file["/first_layer_height"_json_pointer];
    auto layer_height =
        current_file["/layer_height"_json_pointer];
    auto object_height =
        current_file["/object_height"_json_pointer];

    if (!first_layer_height.is_null() &&
        !layer_height.is_null() &&
        !object_height.is_null()) {
      auto layer = static_cast<int>(
          std::ceil(
              (object_height.template get<double>() -
               first_layer_height.template get<double>()) /
                  layer_height.template get<double>() +
              1));
      return layer > 0 ? layer : 0;
    }
  }

  return 0;
}

int PrintStatusPanel::current_layer(json &info)
{
  if (!info.is_null()) {
    auto value = info["/current_layer"_json_pointer];
    if (!value.is_null()) {
      return value.template get<int>();
    }
  }

  if (!current_file.is_null()) {
    State *state = State::get_instance();

    auto print_duration = state->get_data(
        "/printer_state/print_stats/print_duration"_json_pointer);
    auto z_position = state->get_data(
        "/printer_state/gcode_move/gcode_position/2"_json_pointer);

    auto first_layer_height =
        current_file["/first_layer_height"_json_pointer];
    auto layer_height =
        current_file["/layer_height"_json_pointer];

    if (!print_duration.is_null() &&
        print_duration.template get<int>() > 0 &&
        !z_position.is_null() &&
        !first_layer_height.is_null() &&
        !layer_height.is_null()) {
      auto layer = static_cast<int>(
          std::ceil(
              (z_position.template get<double>() -
               first_layer_height.template get<double>()) /
                  layer_height.template get<double>() +
              1));

      auto total = max_layer(info);
      if (layer > total) {
        return total;
      }

      if (layer > 0) {
        return layer;
      }
    }
  }

  return 0;
}

FineTunePanel &PrintStatusPanel::get_finetune_panel()
{
  return finetune_panel;
}
