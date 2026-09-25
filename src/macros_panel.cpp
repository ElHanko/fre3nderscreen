#include "macros_panel.h"

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

void style_card(lv_obj_t *card)
{
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(card, lv_color_hex(COLOR_CARD), 0);
  lv_obj_set_style_border_width(card, 1, 0);
  lv_obj_set_style_border_color(card, lv_color_hex(COLOR_BORDER), 0);
  lv_obj_set_style_radius(card, 8, 0);
  lv_obj_set_style_shadow_width(card, 0, 0);
}

void style_transparent(lv_obj_t *obj)
{
  lv_obj_set_style_pad_all(obj, 0, 0);
  lv_obj_set_style_border_width(obj, 0, 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
}

} // namespace

MacrosPanel::MacrosPanel(KWebSocketClient &client)
  : ws(client)
  , cont(lv_obj_create(lv_scr_act()))
  , header_cont(lv_obj_create(cont))
  , back_btn(lv_btn_create(header_cont))
  , title_label(lv_label_create(header_cont))
  , content_cont(lv_obj_create(cont))
  , controls_card(lv_obj_create(content_cont))
  , show_hide_switch(lv_switch_create(controls_card))
  , top_cont(lv_obj_create(content_cont))
  , kb(lv_keyboard_create(content_cont))
{
  lv_obj_move_background(cont);
  lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
  lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(cont, 8, 0);
  lv_obj_set_style_pad_row(cont, 8, 0);
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
      &MacrosPanel::_handle_callback,
      LV_EVENT_CLICKED,
      this);

  lv_obj_t *back_label = lv_label_create(back_btn);
  lv_label_set_text(back_label, LV_SYMBOL_LEFT);
  lv_obj_set_style_text_font(back_label, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(back_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(back_label);

  lv_label_set_text(title_label, "Macros");
  lv_obj_set_style_text_font(title_label, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(title_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(title_label);

  lv_obj_set_grid_cell(
      content_cont,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 1, 1);
  style_transparent(content_cont);
  lv_obj_clear_flag(content_cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_row(content_cont, 8, 0);
  lv_obj_set_flex_flow(content_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(
      content_cont,
      LV_FLEX_ALIGN_START,
      LV_FLEX_ALIGN_START,
      LV_FLEX_ALIGN_START);

  style_card(controls_card);
  lv_obj_set_width(controls_card, LV_PCT(100));
  lv_obj_set_height(controls_card, 50);
  lv_obj_set_style_pad_all(controls_card, 10, 0);

  lv_obj_t *show_hidden_label = lv_label_create(controls_card);
  lv_label_set_text(show_hidden_label, "Show Hidden");
  lv_obj_set_style_text_font(show_hidden_label, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(show_hidden_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_align(show_hidden_label, LV_ALIGN_LEFT_MID, 0, 0);

  lv_obj_align(show_hide_switch, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_obj_set_style_bg_color(
      show_hide_switch,
      lv_color_hex(COLOR_ACCENT),
      LV_PART_INDICATOR | LV_STATE_CHECKED);
  lv_obj_add_event_cb(
      show_hide_switch,
      &MacrosPanel::_handle_callback,
      LV_EVENT_VALUE_CHANGED,
      this);

  style_transparent(top_cont);
  lv_obj_set_width(top_cont, LV_PCT(100));
  lv_obj_set_flex_grow(top_cont, 1);
  lv_obj_set_style_pad_row(top_cont, 8, 0);
  lv_obj_set_flex_flow(top_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(
      top_cont,
      LV_FLEX_ALIGN_START,
      LV_FLEX_ALIGN_START,
      LV_FLEX_ALIGN_START);
  lv_obj_set_scroll_dir(top_cont, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(top_cont, LV_SCROLLBAR_MODE_AUTO);

  lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_width(kb, LV_PCT(100));
  lv_obj_set_height(kb, 180);
  lv_obj_set_style_text_font(kb, &lv_font_montserrat_16, LV_STATE_DEFAULT);
}

MacrosPanel::~MacrosPanel()
{
  if (cont != nullptr) {
    lv_obj_del(cont);
    cont = nullptr;
  }
}

void MacrosPanel::foreground()
{
  lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
  lv_keyboard_set_textarea(kb, nullptr);
  lv_obj_move_foreground(cont);
}

void MacrosPanel::populate()
{
  macro_items.clear();

  auto &config_json = State::get_instance()
      ->get_data("/printer_state/configfile/config"_json_pointer);

  auto &macro_settings = State::get_instance()
      ->get_data("/guppysettings/macros/settings"_json_pointer);

  if (config_json.is_null()) {
    return;
  }

  const auto macros = KUtils::parse_macros(config_json);

  for (const auto &[name, params] : macros) {
    const auto hidden_json =
        macro_settings[json::json_pointer(fmt::format("/{}/hidden", name))];
    const bool hidden =
        !hidden_json.is_null() ? hidden_json.template get<bool>() : false;

    macro_items.push_back(std::make_shared<MacroItem>(
        ws,
        top_cont,
        name,
        params,
        kb,
        hidden));
  }
}

void MacrosPanel::handle_callback(lv_event_t *event)
{
  const lv_event_code_t code = lv_event_get_code(event);
  lv_obj_t *obj = lv_event_get_current_target(event);

  if (code == LV_EVENT_CLICKED && obj == back_btn) {
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    lv_keyboard_set_textarea(kb, nullptr);
    lv_obj_move_background(cont);
    return;
  }

  if (code != LV_EVENT_VALUE_CHANGED || obj != show_hide_switch) {
    return;
  }

  const bool show_hidden =
      lv_obj_has_state(show_hide_switch, LV_STATE_CHECKED);

  for (const auto &macro : macro_items) {
    if (show_hidden) {
      macro->show();
    } else {
      macro->hide_if_hidden();
    }
  }
}
