#include "numpad.h"
#include "spdlog/spdlog.h"

#include <string>

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
}

} // namespace

Numpad::Numpad(lv_obj_t *parent)
  : edit_cont(lv_obj_create(parent))
  , header_cont(lv_obj_create(edit_cont))
  , back_btn(lv_btn_create(header_cont))
  , title_label(lv_label_create(header_cont))
  , input(lv_textarea_create(edit_cont))
  , kb(lv_keyboard_create(edit_cont))
  , ready_cb([](double value) {})
{
  spdlog::trace("creating global numpad");

  lv_obj_add_flag(edit_cont, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_size(edit_cont, LV_PCT(100), LV_PCT(100));
  lv_obj_clear_flag(edit_cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(edit_cont, 8, 0);
  lv_obj_set_style_pad_row(edit_cont, 8, 0);
  lv_obj_set_style_border_width(edit_cont, 0, 0);
  lv_obj_set_style_bg_color(edit_cont, lv_color_hex(COLOR_BG), 0);

  static lv_coord_t rows[] = {
    42,
    58,
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  static lv_coord_t cols[] = {
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  lv_obj_set_grid_dsc_array(edit_cont, cols, rows);

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
      &Numpad::_handle_back,
      LV_EVENT_CLICKED,
      this);

  lv_obj_t *back_label = lv_label_create(back_btn);
  lv_label_set_text(back_label, LV_SYMBOL_LEFT);
  lv_obj_set_style_text_font(back_label, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(back_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(back_label);

  lv_label_set_text(title_label, "Enter Value");
  lv_obj_set_style_text_font(title_label, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(title_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(title_label);

  lv_obj_set_grid_cell(
      input,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 1, 1);
  lv_textarea_set_one_line(input, true);
  lv_textarea_set_text(input, "");
  lv_obj_set_style_bg_color(input, lv_color_hex(COLOR_CARD), 0);
  lv_obj_set_style_border_width(input, 1, 0);
  lv_obj_set_style_border_color(input, lv_color_hex(COLOR_BORDER), 0);
  lv_obj_set_style_border_color(
      input,
      lv_color_hex(COLOR_ACCENT),
      LV_STATE_FOCUSED);
  lv_obj_set_style_radius(input, 8, 0);
  lv_obj_set_style_shadow_width(input, 0, 0);
  lv_obj_set_style_text_font(input, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_color(input, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_set_style_text_align(input, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_style_pad_all(input, 12, 0);

  lv_obj_set_grid_cell(
      kb,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 2, 1);

  static const char *kb_map[] = {
    "1", "2", "3", "\n",
    "4", "5", "6", "\n",
    "7", "8", "9", "\n",
    LV_SYMBOL_BACKSPACE, "0", LV_SYMBOL_OK, nullptr
  };
  static const lv_btnmatrix_ctrl_t kb_ctrl[] = {
    1, 1, 1,
    1, 1, 1,
    1, 1, 1,
    1, 1, 1
  };

  lv_keyboard_set_map(
      kb,
      LV_KEYBOARD_MODE_USER_1,
      kb_map,
      kb_ctrl);
  lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_USER_1);
  lv_keyboard_set_textarea(kb, input);

  lv_obj_clear_flag(kb, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(kb, lv_color_hex(COLOR_BG), LV_PART_MAIN);
  lv_obj_set_style_border_width(kb, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(kb, 2, LV_PART_MAIN);
  lv_obj_set_style_pad_row(kb, 5, LV_PART_MAIN);
  lv_obj_set_style_pad_column(kb, 5, LV_PART_MAIN);

  lv_obj_set_style_bg_color(kb, lv_color_hex(COLOR_CARD), LV_PART_ITEMS);
  lv_obj_set_style_border_width(kb, 1, LV_PART_ITEMS);
  lv_obj_set_style_border_color(
      kb,
      lv_color_hex(COLOR_BORDER),
      LV_PART_ITEMS);
  lv_obj_set_style_radius(kb, 8, LV_PART_ITEMS);
  lv_obj_set_style_shadow_width(kb, 0, LV_PART_ITEMS);
  lv_obj_set_style_text_font(
      kb,
      &lv_font_montserrat_20,
      LV_PART_ITEMS);
  lv_obj_set_style_text_color(
      kb,
      lv_color_hex(COLOR_TEXT),
      LV_PART_ITEMS);
  lv_obj_set_style_bg_color(
      kb,
      lv_color_hex(COLOR_ACCENT),
      LV_PART_ITEMS | LV_STATE_PRESSED);
  lv_obj_set_style_text_color(
      kb,
      lv_color_hex(COLOR_BG),
      LV_PART_ITEMS | LV_STATE_PRESSED);

  lv_obj_add_event_cb(
      input,
      &Numpad::_handle_input,
      LV_EVENT_ALL,
      this);
}

Numpad::~Numpad()
{
  if (edit_cont != nullptr) {
    lv_obj_del(edit_cont);
    edit_cont = nullptr;
  }
}

void Numpad::set_callback(std::function<void(double)> cb)
{
  ready_cb = cb;
}

void Numpad::close()
{
  lv_obj_add_flag(edit_cont, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_background(edit_cont);
  lv_textarea_set_text(input, "");
}

void Numpad::handle_back(lv_event_t *event)
{
  if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
    close();
  }
}

void Numpad::handle_input(lv_event_t *event)
{
  const lv_event_code_t code = lv_event_get_code(event);

  if (code == LV_EVENT_CANCEL) {
    close();
    return;
  }

  if (code != LV_EVENT_READY) {
    return;
  }

  std::string value = lv_textarea_get_text(input);
  if (!value.empty()) {
    ready_cb(std::stod(value));
  }

  close();
}

void Numpad::foreground_reset()
{
  spdlog::trace("showing numpad");
  lv_textarea_set_text(input, "");
  lv_keyboard_set_textarea(kb, input);
  lv_obj_clear_flag(edit_cont, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(edit_cont);
  lv_obj_add_state(input, LV_STATE_FOCUSED);
}
