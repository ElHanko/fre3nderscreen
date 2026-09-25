#include "wifi_panel.h"

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

void style_transparent_container(lv_obj_t *cont)
{
  lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(cont, 0, 0);
  lv_obj_set_style_border_width(cont, 0, 0);
  lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
}

} // namespace

WifiPanel::WifiPanel(std::mutex &)
  : cont(lv_obj_create(lv_scr_act()))
  , header_cont(lv_obj_create(cont))
  , back_btn(lv_btn_create(header_cont))
  , title_label(lv_label_create(header_cont))
  , placeholder_card(lv_obj_create(cont))
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
  style_transparent_container(header_cont);

  style_button(back_btn);
  lv_obj_set_size(back_btn, 40, 34);
  lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 0, 0);
  lv_obj_add_event_cb(
      back_btn,
      &WifiPanel::_handle_back_btn,
      LV_EVENT_CLICKED,
      this);

  lv_obj_t *back_label = lv_label_create(back_btn);
  lv_label_set_text(back_label, LV_SYMBOL_LEFT);
  lv_obj_set_style_text_font(back_label, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(back_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(back_label);

  lv_label_set_text(title_label, "WiFi");
  lv_obj_set_style_text_font(title_label, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(title_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(title_label);

  lv_obj_set_grid_cell(
      placeholder_card,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_START, 1, 1);
  lv_obj_set_height(placeholder_card, 132);
  lv_obj_clear_flag(placeholder_card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(placeholder_card, 14, 0);
  lv_obj_set_style_pad_row(placeholder_card, 8, 0);
  lv_obj_set_style_radius(placeholder_card, 8, 0);
  lv_obj_set_style_border_width(placeholder_card, 1, 0);
  lv_obj_set_style_border_color(
      placeholder_card,
      lv_color_hex(COLOR_BORDER),
      0);
  lv_obj_set_style_bg_color(
      placeholder_card,
      lv_color_hex(COLOR_CARD),
      0);
  lv_obj_set_flex_flow(placeholder_card, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(
      placeholder_card,
      LV_FLEX_ALIGN_CENTER,
      LV_FLEX_ALIGN_CENTER,
      LV_FLEX_ALIGN_CENTER);

  lv_obj_t *wifi_icon = lv_label_create(placeholder_card);
  lv_label_set_text(wifi_icon, LV_SYMBOL_WIFI);
  lv_obj_set_style_text_font(wifi_icon, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(wifi_icon, lv_color_hex(COLOR_ACCENT), 0);

  lv_obj_t *heading = lv_label_create(placeholder_card);
  lv_label_set_text(heading, "WiFi management");
  lv_obj_set_width(heading, LV_PCT(100));
  lv_obj_set_style_text_align(heading, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(heading, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(heading, lv_color_hex(COLOR_TEXT), 0);

  lv_obj_t *description = lv_label_create(placeholder_card);
  lv_label_set_text(description, "Fre3nder API integration pending");
  lv_obj_set_width(description, LV_PCT(100));
  lv_label_set_long_mode(description, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(description, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(description, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(description, lv_color_hex(COLOR_MUTED), 0);
}

WifiPanel::~WifiPanel()
{
  if (cont != nullptr) {
    lv_obj_del(cont);
    cont = nullptr;
  }
}

void WifiPanel::foreground()
{
  lv_obj_move_foreground(cont);
}

void WifiPanel::handle_back_btn(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
    return;
  }

  if (lv_event_get_current_target(event) == back_btn) {
    lv_obj_move_background(cont);
  }
}
