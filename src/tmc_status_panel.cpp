#include "tmc_status_panel.h"
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

TmcStatusPanel::TmcStatusPanel(KWebSocketClient &client,
                               std::mutex &lock)
  : NotifyConsumer(lock)
  , ws(client)
  , cont(lv_obj_create(lv_scr_act()))
  , header_cont(lv_obj_create(cont))
  , back_btn(lv_btn_create(header_cont))
  , title_label(lv_label_create(header_cont))
  , top(lv_obj_create(cont))
  , toggle(lv_switch_create(top))
  , metrics_cont(lv_obj_create(cont))
  , empty_label(lv_label_create(metrics_cont))
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
    96,
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
      [](lv_event_t *event) {
        auto *panel = static_cast<TmcStatusPanel *>(event->user_data);
        panel->background();
      },
      LV_EVENT_CLICKED,
      this);

  lv_obj_t *back_label = lv_label_create(back_btn);
  lv_label_set_text(back_label, LV_SYMBOL_LEFT);
  lv_obj_set_style_text_font(back_label, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(back_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(back_label);

  lv_label_set_text(title_label, "TMC Status");
  lv_obj_set_style_text_font(title_label, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(title_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(title_label);

  style_card(top);
  lv_obj_set_grid_cell(
      top,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 1, 1);
  lv_obj_set_style_pad_all(top, 10, 0);

  lv_obj_t *info = lv_label_create(top);
  lv_label_set_text(
      info,
      "Experimental TMC metrics\nEnable only when needed.");
  lv_obj_set_width(info, LV_PCT(72));
  lv_label_set_long_mode(info, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_font(info, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(info, lv_color_hex(COLOR_MUTED), 0);
  lv_obj_align(info, LV_ALIGN_LEFT_MID, 0, 0);

  style_switch(toggle);
  lv_obj_clear_state(toggle, LV_STATE_CHECKED);
  lv_obj_align(toggle, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_obj_add_event_cb(
      toggle,
      [](lv_event_t *event) {
        auto *panel = static_cast<TmcStatusPanel *>(event->user_data);
        lv_obj_t *sw = lv_event_get_target(event);

        if (lv_obj_has_state(sw, LV_STATE_CHECKED)) {
          panel->ws.gcode_script(
              "_GUPPY_LOAD_MODULE SECTION=tmcstatus");
        } else {
          panel->ws.gcode_script(
              "_GUPPY_UNLOAD_MODULE SECTION=tmcstatus");
        }
      },
      LV_EVENT_VALUE_CHANGED,
      this);

  lv_obj_set_grid_cell(
      metrics_cont,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 2, 1);
  style_transparent(metrics_cont);
  lv_obj_set_style_pad_row(metrics_cont, 8, 0);
  lv_obj_set_style_pad_right(metrics_cont, 4, 0);
  lv_obj_set_flex_flow(metrics_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_scroll_dir(metrics_cont, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(metrics_cont, LV_SCROLLBAR_MODE_AUTO);

  lv_label_set_text(
      empty_label,
      "No TMC metrics available.\nEnable metrics to start monitoring.");
  lv_obj_set_width(empty_label, LV_PCT(100));
  lv_obj_set_style_text_align(empty_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(empty_label, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(empty_label, lv_color_hex(COLOR_MUTED), 0);

  ws.register_notify_update(this);
}

TmcStatusPanel::~TmcStatusPanel()
{
  if (cont != nullptr) {
    lv_obj_del(cont);
    cont = nullptr;
  }
}

void TmcStatusPanel::foreground()
{
  lv_obj_move_foreground(cont);
}

void TmcStatusPanel::background()
{
  lv_obj_move_background(cont);
}

void TmcStatusPanel::init(json &j)
{
  auto tmc_status = j["/result/status/tmcstatus"_json_pointer];
  if (tmc_status.is_null()) {
    return;
  }

  if (!tmc_status.empty()) {
    lv_obj_add_state(toggle, LV_STATE_CHECKED);
    lv_obj_add_flag(empty_label, LV_OBJ_FLAG_HIDDEN);
  }

  for (auto &el : tmc_status.items()) {
    const auto &existing = metrics.find(el.key());
    if (existing != metrics.end()) {
      existing->second->update(el.value());
      continue;
    }

    spdlog::debug("tmc stepper created {}", el.key());
    auto status = std::make_shared<TmcStatusContainer>(
        ws,
        metrics_cont,
        el.key());
    metrics.insert({el.key(), status});
    status->update(el.value());
  }
}

void TmcStatusPanel::consume(json &j)
{
  std::lock_guard<std::mutex> lock(lv_lock);

  auto tmc_status = j["/params/0/tmcstatus"_json_pointer];
  if (tmc_status.is_null()) {
    return;
  }

  if (!tmc_status.empty()) {
    lv_obj_add_flag(empty_label, LV_OBJ_FLAG_HIDDEN);
  }

  for (auto &el : tmc_status.items()) {
    const auto &existing = metrics.find(el.key());
    if (existing != metrics.end()) {
      existing->second->update(el.value());
      continue;
    }

    spdlog::debug("tmc stepper created {}", el.key());
    auto status = std::make_shared<TmcStatusContainer>(
        ws,
        metrics_cont,
        el.key());
    metrics.insert({el.key(), status});
    status->update(el.value());
  }
}
