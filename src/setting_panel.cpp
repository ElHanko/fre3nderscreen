#include "setting_panel.h"

#include "spdlog/spdlog.h"

LV_IMG_DECLARE(network_img);
LV_IMG_DECLARE(sysinfo_img);

namespace {

constexpr uint32_t COLOR_BG = 0x080B0D;
constexpr uint32_t COLOR_CARD = 0x11171B;
constexpr uint32_t COLOR_CARD_PRESSED = 0x18242A;
constexpr uint32_t COLOR_BORDER = 0x25323A;
constexpr uint32_t COLOR_ACCENT = 0x00E5FF;
constexpr uint32_t COLOR_TEXT = 0xFFFFFF;

lv_obj_t *create_setting_card(lv_obj_t *parent,
                              const void *image,
                              const char *text,
                              lv_event_cb_t callback,
                              void *user_data)
{
  lv_obj_t *button = lv_btn_create(parent);
  lv_obj_clear_flag(button, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(button, 12, 0);
  lv_obj_set_style_bg_color(button, lv_color_hex(COLOR_CARD), 0);
  lv_obj_set_style_border_width(button, 1, 0);
  lv_obj_set_style_border_color(button, lv_color_hex(COLOR_BORDER), 0);
  lv_obj_set_style_radius(button, 8, 0);
  lv_obj_set_style_shadow_width(button, 0, 0);
  lv_obj_set_style_bg_color(button, lv_color_hex(COLOR_CARD_PRESSED), LV_STATE_PRESSED);
  lv_obj_set_style_border_color(button, lv_color_hex(COLOR_ACCENT), LV_STATE_PRESSED);
  lv_obj_set_flex_flow(button, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(
      button,
      LV_FLEX_ALIGN_CENTER,
      LV_FLEX_ALIGN_CENTER,
      LV_FLEX_ALIGN_CENTER);
  lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, user_data);

  lv_obj_t *icon = lv_img_create(button);
  lv_img_set_src(icon, image);
  lv_obj_set_style_img_recolor_opa(icon, LV_OPA_COVER, 0);
  lv_obj_set_style_img_recolor(icon, lv_color_hex(COLOR_TEXT), 0);

  lv_obj_t *label = lv_label_create(button);
  lv_label_set_text(label, text);
  lv_obj_set_width(label, LV_PCT(100));
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(COLOR_TEXT), 0);

  return button;
}

} // namespace

SettingPanel::SettingPanel(std::mutex &lock, lv_obj_t *parent)
  : cont(lv_obj_create(parent))
  , fre3nder_panel(lock)
  , fre3nderscreen_panel()
  , fre3nder_btn(nullptr)
  , fre3nderscreen_btn(nullptr)
{
  lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
  lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(cont, 12, 0);
  lv_obj_set_style_pad_row(cont, 10, 0);
  lv_obj_set_style_border_width(cont, 0, 0);
  lv_obj_set_style_bg_color(cont, lv_color_hex(COLOR_BG), 0);

  static lv_coord_t rows[] = {
    LV_GRID_FR(1),
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  static lv_coord_t cols[] = {
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  lv_obj_set_grid_dsc_array(cont, cols, rows);

  fre3nder_btn = create_setting_card(
      cont,
      &network_img,
      "Fre3nder",
      &SettingPanel::_handle_callback,
      this);

  fre3nderscreen_btn = create_setting_card(
      cont,
      &sysinfo_img,
      "Fre3nderScreen",
      &SettingPanel::_handle_callback,
      this);

  lv_obj_set_grid_cell(
      fre3nder_btn,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 0, 1);

  lv_obj_set_grid_cell(
      fre3nderscreen_btn,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 1, 1);
}

SettingPanel::~SettingPanel()
{
  // cont belongs to the Settings tab and is deleted with the tabview.
}

lv_obj_t *SettingPanel::get_container()
{
  return cont;
}

void SettingPanel::handle_callback(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
    return;
  }

  lv_obj_t *button = lv_event_get_current_target(event);

  if (button == fre3nder_btn) {
    spdlog::trace("Fre3nder settings pressed");
    fre3nder_panel.foreground();
  } else if (button == fre3nderscreen_btn) {
    spdlog::trace("Fre3nderScreen settings pressed");
    fre3nderscreen_panel.foreground();
  }
}
