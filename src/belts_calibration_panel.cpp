#include "belts_calibration_panel.h"
#include "utils.h"
#include "config.h"
#include "spdlog/spdlog.h"
#include "spdlog/fmt/fmt.h"

#define BELTS_PNG "belts_calibration.png"

std::vector<std::string> BeltsCalibrationPanel::axes = {
  "x",
  "y",
  "a",
  "b"
};

namespace {

constexpr uint32_t COLOR_BG = 0x080B0D;
constexpr uint32_t COLOR_CARD = 0x11171B;
constexpr uint32_t COLOR_CARD_PRESSED = 0x18242A;
constexpr uint32_t COLOR_BORDER = 0x25323A;
constexpr uint32_t COLOR_ACCENT = 0x00E5FF;
constexpr uint32_t COLOR_TEXT = 0xFFFFFF;
constexpr uint32_t COLOR_MUTED = 0xB8C0C5;
constexpr uint32_t COLOR_DANGER = 0xFF4D5A;

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
}

void style_dropdown(lv_obj_t *dropdown)
{
  lv_obj_set_style_bg_color(dropdown, lv_color_hex(COLOR_CARD_PRESSED), 0);
  lv_obj_set_style_border_width(dropdown, 1, 0);
  lv_obj_set_style_border_color(dropdown, lv_color_hex(COLOR_BORDER), 0);
  lv_obj_set_style_radius(dropdown, 6, 0);
  lv_obj_set_style_text_font(dropdown, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(dropdown, lv_color_hex(COLOR_TEXT), 0);
}

lv_obj_t *create_action_button(lv_obj_t *parent,
                               const char *icon_text,
                               const char *label_text,
                               bool danger = false)
{
  lv_obj_t *button = lv_btn_create(parent);
  style_button(button, danger);
  lv_obj_set_height(button, 68);
  lv_obj_set_flex_grow(button, 1);
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

} // namespace

BeltsCalibrationPanel::BeltsCalibrationPanel(KWebSocketClient &client,
                                             std::mutex &lock)
  : ws(client)
  , lv_lock(lock)
  , cont(lv_obj_create(lv_scr_act()))
  , header_cont(lv_obj_create(cont))
  , back_btn(lv_btn_create(header_cont))
  , title_label(lv_label_create(header_cont))
  , graph_cont(lv_obj_create(cont))
  , graph(lv_img_create(graph_cont))
  , graph_label(lv_label_create(graph_cont))
  , spinner(lv_spinner_create(cont, 1000, 60))
  , excite_control(lv_obj_create(cont))
  , excite_title(lv_label_create(excite_control))
  , excite_slider(lv_slider_create(excite_control))
  , excite_label(lv_label_create(excite_control))
  , excite_dd(lv_dropdown_create(excite_control))
  , button_cont(lv_obj_create(cont))
  , calibrate_btn(create_action_button(
        button_cont, LV_SYMBOL_REFRESH, "Shake Belts"))
  , excite_btn(create_action_button(
        button_cont, LV_SYMBOL_PLAY, "Excite"))
  , emergency_btn(create_action_button(
        button_cont, LV_SYMBOL_WARNING, "Stop", true))
  , image_fullsized(false)
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
    92,
    74,
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

  style_button(back_btn);
  lv_obj_set_size(back_btn, 40, 34);
  lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 0, 0);
  lv_obj_add_event_cb(
      back_btn,
      &BeltsCalibrationPanel::_handle_callback,
      LV_EVENT_CLICKED,
      this);

  lv_obj_t *back_label = lv_label_create(back_btn);
  lv_label_set_text(back_label, LV_SYMBOL_LEFT);
  lv_obj_set_style_text_font(back_label, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(back_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(back_label);

  lv_label_set_text(title_label, "Belts Calibration");
  lv_obj_set_style_text_font(title_label, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(title_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(title_label);

  style_card(graph_cont);
  lv_obj_set_grid_cell(
      graph_cont,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 1, 1);
  lv_obj_add_flag(graph_cont, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(
      graph_cont,
      &BeltsCalibrationPanel::_handle_image_clicked,
      LV_EVENT_CLICKED,
      this);

  lv_img_set_zoom(graph, 100);
  lv_obj_center(graph);

  lv_label_set_text(graph_label, "Belt Response");
  lv_obj_set_style_text_font(graph_label, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(graph_label, lv_color_hex(COLOR_MUTED), 0);
  lv_obj_align(graph_label, LV_ALIGN_BOTTOM_MID, 0, -4);

  lv_obj_add_flag(spinner, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_size(spinner, 64, 64);
  lv_obj_set_grid_cell(
      spinner,
      LV_GRID_ALIGN_CENTER, 0, 1,
      LV_GRID_ALIGN_CENTER, 1, 1);

  style_card(excite_control);
  lv_obj_set_grid_cell(
      excite_control,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 2, 1);
  lv_obj_set_style_pad_all(excite_control, 8, 0);

  static lv_coord_t excite_rows[] = {
    30,
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  static lv_coord_t excite_cols[] = {
    LV_GRID_FR(1),
    112,
    LV_GRID_TEMPLATE_LAST
  };
  lv_obj_set_grid_dsc_array(
      excite_control,
      excite_cols,
      excite_rows);

  lv_label_set_text(excite_title, "Excite Frequency");
  lv_obj_set_style_text_font(excite_title, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(excite_title, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_set_grid_cell(
      excite_title,
      LV_GRID_ALIGN_START, 0, 1,
      LV_GRID_ALIGN_CENTER, 0, 1);

  style_dropdown(excite_dd);
  lv_dropdown_set_options(
      excite_dd,
      fmt::format("{}", fmt::join(axes, "\n")).c_str());
  lv_obj_set_grid_cell(
      excite_dd,
      LV_GRID_ALIGN_STRETCH, 1, 1,
      LV_GRID_ALIGN_CENTER, 0, 1);

  style_slider(excite_slider);
  lv_slider_set_range(excite_slider, 10, 1400);
  lv_obj_set_grid_cell(
      excite_slider,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_CENTER, 1, 1);
  lv_obj_add_event_cb(
      excite_slider,
      &BeltsCalibrationPanel::_handle_update_slider,
      LV_EVENT_VALUE_CHANGED,
      this);

  lv_label_set_text(excite_label, "1 Hz");
  lv_obj_set_style_text_font(excite_label, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(excite_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_set_style_text_align(excite_label, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_grid_cell(
      excite_label,
      LV_GRID_ALIGN_END, 1, 1,
      LV_GRID_ALIGN_CENTER, 1, 1);

  style_transparent(button_cont);
  lv_obj_set_grid_cell(
      button_cont,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 3, 1);
  lv_obj_set_style_pad_column(button_cont, 6, 0);
  lv_obj_set_flex_flow(button_cont, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(
      button_cont,
      LV_FLEX_ALIGN_START,
      LV_FLEX_ALIGN_CENTER,
      LV_FLEX_ALIGN_CENTER);

  lv_obj_add_event_cb(
      calibrate_btn,
      &BeltsCalibrationPanel::_handle_callback,
      LV_EVENT_CLICKED,
      this);
  lv_obj_add_event_cb(
      excite_btn,
      &BeltsCalibrationPanel::_handle_callback,
      LV_EVENT_CLICKED,
      this);
  lv_obj_add_event_cb(
      emergency_btn,
      &BeltsCalibrationPanel::_handle_callback,
      LV_EVENT_CLICKED,
      this);

  ws.register_method_callback(
      "notify_gcode_response",
      "BeltsCalibrationPanel",
      [this](json &data) { this->handle_macro_response(data); });
}

BeltsCalibrationPanel::~BeltsCalibrationPanel()
{
  if (cont != nullptr) {
    lv_obj_del(cont);
    cont = nullptr;
  }
}

void BeltsCalibrationPanel::foreground()
{
  lv_obj_move_foreground(cont);
}

void BeltsCalibrationPanel::handle_callback(lv_event_t *event)
{
  lv_obj_t *btn = lv_event_get_current_target(event);

  if (btn == calibrate_btn) {
    auto config_root = KUtils::get_root_path("config");
    auto png_path = fmt::format(
        "{}/{}",
        config_root.length() > 0 ? config_root : "/tmp",
        BELTS_PNG);

    if (!KUtils::is_homed()) {
      ws.gcode_script("G28");
    }

    auto screen_width =
        static_cast<double>(lv_disp_get_physical_hor_res(nullptr)) / 100.0;
    auto screen_height =
        static_cast<double>(lv_disp_get_physical_ver_res(nullptr)) / 100.0;

    ws.gcode_script(fmt::format(
        "GUPPY_BELTS_SHAPER_CALIBRATION "
        "PNG_OUT_PATH={} PNG_WIDTH={} PNG_HEIGHT={}",
        png_path,
        screen_width,
        screen_height));

    lv_obj_add_flag(graph, LV_OBJ_FLAG_HIDDEN);
    lv_img_set_src(graph, nullptr);
    lv_obj_invalidate(graph);
    lv_obj_clear_flag(spinner, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(spinner);

  } else if (btn == excite_btn) {
    double excite_hz =
        static_cast<double>(lv_slider_get_value(excite_slider)) / 10.0 + 1;

    char excite_buf[10];
    lv_dropdown_get_selected_str(
        excite_dd,
        excite_buf,
        sizeof(excite_buf));

    if (!KUtils::is_homed()) {
      ws.gcode_script("G28");
    }

    ws.gcode_script(fmt::format(
        "GUPPY_EXCITATE_AXIS_AT_FREQ FREQUENCY={} AXIS={}",
        excite_hz,
        excite_buf));

  } else if (btn == back_btn) {
    lv_obj_move_background(cont);

  } else if (btn == emergency_btn) {
    auto value =
        Config::get_instance()->get_json("/prompt_emergency_stop");
    const bool prompt =
        !value.is_null() && value.template get<bool>();

    if (!prompt) {
      ws.send_jsonrpc("printer.emergency_stop");
      return;
    }

    static const char *buttons[] = {"Confirm", "Cancel", ""};
    lv_obj_t *msgbox = lv_msgbox_create(
        nullptr,
        nullptr,
        "Do you want to emergency stop?",
        buttons,
        false);

    lv_obj_t *message = ((lv_msgbox_t *)msgbox)->text;
    lv_obj_set_width(message, LV_PCT(100));
    lv_obj_set_style_text_align(message, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t *button_matrix = lv_msgbox_get_btns(msgbox);
    lv_btnmatrix_set_btn_ctrl(
        button_matrix, 0, LV_BTNMATRIX_CTRL_CHECKED);
    lv_btnmatrix_set_btn_ctrl(
        button_matrix, 1, LV_BTNMATRIX_CTRL_CHECKED);

    lv_obj_add_event_cb(
        msgbox,
        [](lv_event_t *e) {
          lv_obj_t *box =
              lv_obj_get_parent(lv_event_get_target(e));
          const uint32_t clicked =
              lv_msgbox_get_active_btn(box);

          if (clicked == 0) {
            auto *panel = static_cast<BeltsCalibrationPanel *>(
                e->user_data);
            panel->ws.send_jsonrpc("printer.emergency_stop");
          }

          lv_msgbox_close(box);
        },
        LV_EVENT_VALUE_CHANGED,
        this);

    lv_obj_center(msgbox);
  }
}

void BeltsCalibrationPanel::handle_image_clicked(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
    return;
  }

  lv_obj_t *clicked = lv_event_get_target(event);
  if (clicked != graph_cont) {
    return;
  }

  if (image_fullsized) {
    lv_img_set_zoom(graph, 100);
    lv_obj_set_size(graph_cont, LV_PCT(100), LV_PCT(100));
    lv_obj_clear_flag(graph_cont, LV_OBJ_FLAG_FLOATING);
  } else {
    lv_img_set_zoom(graph, LV_IMG_ZOOM_NONE);
    lv_obj_set_size(graph_cont, LV_PCT(100), LV_PCT(100));
    lv_obj_add_flag(graph_cont, LV_OBJ_FLAG_FLOATING);
  }

  lv_obj_move_foreground(graph_cont);
  image_fullsized = !image_fullsized;
}

void BeltsCalibrationPanel::handle_macro_response(json &j)
{
  spdlog::trace("belts calibration macro response: {}", j.dump());

  auto &value = j["/params/0"_json_pointer];
  if (value.is_null()) {
    return;
  }

  std::string response = value.template get<std::string>();
  std::lock_guard<std::mutex> lock(lv_lock);

  if (response.rfind(
          "// Command {guppy_belts_calibration} finished",
          0) != 0) {
    return;
  }

  spdlog::trace("belts calibration finished");

  auto config_root = KUtils::get_root_path("config");
  auto png_path = fmt::format(
      "{}/{}",
      config_root.length() > 0 ? config_root : "/tmp",
      BELTS_PNG);

  png_path = fmt::format(
      "A:{}",
      KUtils::is_running_local()
          ? png_path
          : KUtils::download_file(
                "config",
                BELTS_PNG,
                Config::get_instance()->get_thumbnail_path()));

  lv_img_set_src(graph, png_path.c_str());
  lv_obj_clear_flag(graph, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(spinner, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_background(spinner);
}

void BeltsCalibrationPanel::handle_update_slider(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED) {
    return;
  }

  double hz = static_cast<double>(lv_slider_get_value(excite_slider));
  lv_slider_set_value(excite_slider, hz, LV_ANIM_OFF);
  lv_label_set_text(
      excite_label,
      fmt::format("{} Hz", hz / 10.0).c_str());
}
