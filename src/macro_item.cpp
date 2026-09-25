#include "macro_item.h"
#include "spdlog/spdlog.h"

namespace {

constexpr uint32_t COLOR_CARD = 0x11171B;
constexpr uint32_t COLOR_CARD_PRESSED = 0x18242A;
constexpr uint32_t COLOR_BORDER = 0x25323A;
constexpr uint32_t COLOR_ACCENT = 0x00E5FF;
constexpr uint32_t COLOR_TEXT = 0xFFFFFF;
constexpr uint32_t COLOR_MUTED = 0xB8C0C5;

void style_transparent(lv_obj_t *obj)
{
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(obj, 0, 0);
  lv_obj_set_style_border_width(obj, 0, 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
}

void style_icon_button(lv_obj_t *button)
{
  lv_obj_clear_flag(button, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(button, 0, 0);
  lv_obj_set_style_bg_color(button, lv_color_hex(COLOR_CARD_PRESSED), 0);
  lv_obj_set_style_border_width(button, 1, 0);
  lv_obj_set_style_border_color(button, lv_color_hex(COLOR_BORDER), 0);
  lv_obj_set_style_radius(button, 6, 0);
  lv_obj_set_style_shadow_width(button, 0, 0);
  lv_obj_set_style_border_color(button, lv_color_hex(COLOR_ACCENT), LV_STATE_PRESSED);
}

} // namespace

MacroItem::MacroItem(KWebSocketClient &client,
                     lv_obj_t *parent,
                     std::string macro_name,
                     const std::map<std::string, std::string> &macro_params,
                     lv_obj_t *keyboard,
                     bool hide)
  : ws(client)
  , cont(lv_obj_create(parent))
  , top_cont(lv_obj_create(cont))
  , macro_label(lv_label_create(top_cont))
  , hide_show_cont(lv_btn_create(top_cont))
  , hide_show(lv_label_create(hide_show_cont))
  , kb(keyboard)
  , hidden(hide)
  , always_visible(false)
{
  if (hidden) {
    lv_obj_add_flag(cont, LV_OBJ_FLAG_HIDDEN);
  }

  lv_obj_set_width(cont, LV_PCT(100));
  lv_obj_set_height(cont, LV_SIZE_CONTENT);
  lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(cont, 8, 0);
  lv_obj_set_style_pad_row(cont, 6, 0);
  lv_obj_set_style_bg_color(cont, lv_color_hex(COLOR_CARD), 0);
  lv_obj_set_style_border_width(cont, 1, 0);
  lv_obj_set_style_border_color(cont, lv_color_hex(COLOR_BORDER), 0);
  lv_obj_set_style_radius(cont, 8, 0);
  lv_obj_set_style_shadow_width(cont, 0, 0);
  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(
      cont,
      LV_FLEX_ALIGN_START,
      LV_FLEX_ALIGN_START,
      LV_FLEX_ALIGN_START);

  style_transparent(top_cont);
  lv_obj_set_width(top_cont, LV_PCT(100));
  lv_obj_set_height(top_cont, 40);

  static lv_coord_t cols[] = {
    36,
    LV_GRID_FR(1),
    40,
    LV_GRID_TEMPLATE_LAST
  };
  static lv_coord_t rows[] = {
    40,
    LV_GRID_TEMPLATE_LAST
  };
  lv_obj_set_grid_dsc_array(top_cont, cols, rows);

  style_icon_button(hide_show_cont);
  lv_obj_set_grid_cell(
      hide_show_cont,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_CENTER, 0, 1);
  lv_obj_add_event_cb(
      hide_show_cont,
      &MacroItem::_handle_hide_show,
      LV_EVENT_CLICKED,
      this);

  lv_label_set_text(
      hide_show,
      hidden ? LV_SYMBOL_EYE_OPEN : LV_SYMBOL_EYE_CLOSE);
  lv_obj_set_style_text_color(
      hide_show,
      hidden ? lv_color_hex(COLOR_ACCENT) : lv_color_hex(COLOR_TEXT),
      0);
  lv_obj_center(hide_show);

  lv_label_set_text(macro_label, macro_name.c_str());
  lv_label_set_long_mode(macro_label, LV_LABEL_LONG_DOT);
  lv_obj_set_width(macro_label, LV_PCT(100));
  lv_obj_set_style_text_font(macro_label, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(macro_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_set_grid_cell(
      macro_label,
      LV_GRID_ALIGN_STRETCH, 1, 1,
      LV_GRID_ALIGN_CENTER, 0, 1);

  lv_obj_t *run_btn = lv_btn_create(top_cont);
  style_icon_button(run_btn);
  lv_obj_set_grid_cell(
      run_btn,
      LV_GRID_ALIGN_STRETCH, 2, 1,
      LV_GRID_ALIGN_CENTER, 0, 1);
  lv_obj_add_event_cb(
      run_btn,
      &MacroItem::_handle_send_macro,
      LV_EVENT_CLICKED,
      this);

  lv_obj_t *run_label = lv_label_create(run_btn);
  lv_label_set_text(run_label, LV_SYMBOL_PLAY);
  lv_obj_set_style_text_color(run_label, lv_color_hex(COLOR_ACCENT), 0);
  lv_obj_center(run_label);

  if (!macro_params.empty()) {
    lv_obj_t *params_cont = lv_obj_create(cont);
    style_transparent(params_cont);
    lv_obj_set_width(params_cont, LV_PCT(100));
    lv_obj_set_height(params_cont, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_row(params_cont, 6, 0);
    lv_obj_set_flex_flow(params_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(
        params_cont,
        LV_FLEX_ALIGN_START,
        LV_FLEX_ALIGN_START,
        LV_FLEX_ALIGN_START);

    for (const auto &[name, value] : macro_params) {
      lv_obj_t *row = lv_obj_create(params_cont);
      style_transparent(row);
      lv_obj_set_width(row, LV_PCT(100));
      lv_obj_set_height(row, 36);

      lv_obj_t *param_name = lv_label_create(row);
      lv_label_set_text(param_name, name.c_str());
      lv_obj_set_width(param_name, LV_PCT(38));
      lv_label_set_long_mode(param_name, LV_LABEL_LONG_DOT);
      lv_obj_set_style_text_font(param_name, &lv_font_montserrat_12, 0);
      lv_obj_set_style_text_color(param_name, lv_color_hex(COLOR_MUTED), 0);
      lv_obj_align(param_name, LV_ALIGN_LEFT_MID, 0, 0);

      lv_obj_t *param_value = lv_textarea_create(row);
      lv_obj_set_size(param_value, LV_PCT(58), 32);
      lv_obj_align(param_value, LV_ALIGN_RIGHT_MID, 0, 0);
      lv_textarea_set_one_line(param_value, true);
      lv_textarea_set_text(param_value, value.c_str());
      lv_obj_set_style_bg_color(param_value, lv_color_hex(COLOR_CARD_PRESSED), 0);
      lv_obj_set_style_border_width(param_value, 1, 0);
      lv_obj_set_style_border_color(param_value, lv_color_hex(COLOR_BORDER), 0);
      lv_obj_set_style_radius(param_value, 6, 0);
      lv_obj_set_style_text_color(param_value, lv_color_hex(COLOR_TEXT), 0);
      lv_obj_set_style_text_font(param_value, &lv_font_montserrat_12, 0);
      lv_obj_add_event_cb(
          param_value,
          &MacroItem::_handle_kb_input,
          LV_EVENT_ALL,
          this);

      params.push_back({param_name, param_value});
    }
  }
}

MacroItem::~MacroItem()
{
  if (cont != nullptr) {
    lv_obj_del(cont);
    cont = nullptr;
  }
}

void MacroItem::handle_kb_input(lv_event_t *event)
{
  const lv_event_code_t code = lv_event_get_code(event);
  lv_obj_t *obj = lv_event_get_target(event);

  if (code == LV_EVENT_FOCUSED) {
    lv_keyboard_set_textarea(kb, obj);
    lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
  } else if (code == LV_EVENT_DEFOCUSED) {
    lv_keyboard_set_textarea(kb, nullptr);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
  } else if (code == LV_EVENT_CANCEL) {
    lv_keyboard_set_textarea(kb, nullptr);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
  }
}

void MacroItem::handle_send_macro(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
    return;
  }

  std::vector<std::string> parts;
  parts.push_back(lv_label_get_text(macro_label));

  for (const auto &param : params) {
    const char *value = lv_textarea_get_text(param.second);
    if (value == nullptr || value[0] == '\0') {
      continue;
    }

    parts.push_back(fmt::format(
        "{}={}",
        lv_label_get_text(param.first),
        value));
  }

  const auto command = fmt::format("{}", fmt::join(parts, " "));
  spdlog::trace("sending macro: {}", command);
  ws.gcode_script(command);
}

void MacroItem::handle_hide_show(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
    return;
  }

  const std::string key =
      fmt::format("macros.settings.{}", lv_label_get_text(macro_label));

  json request = {
    {"namespace", "fre3nderscreen"},
    {"key", key},
    {"value", {
      {"hidden", !hidden}
    }}
  };

  ws.send_jsonrpc("server.database.post_item", request);

  hidden = !hidden;

  if (hidden) {
    if (!always_visible) {
      lv_obj_add_flag(cont, LV_OBJ_FLAG_HIDDEN);
    }
    lv_label_set_text(hide_show, LV_SYMBOL_EYE_OPEN);
    lv_obj_set_style_text_color(
        hide_show,
        lv_color_hex(COLOR_ACCENT),
        0);
  } else {
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(hide_show, LV_SYMBOL_EYE_CLOSE);
    lv_obj_set_style_text_color(
        hide_show,
        lv_color_hex(COLOR_TEXT),
        0);
  }
}

void MacroItem::hide_if_hidden()
{
  if (hidden) {
    lv_obj_add_flag(cont, LV_OBJ_FLAG_HIDDEN);
  }
  always_visible = false;
}

void MacroItem::show()
{
  lv_obj_clear_flag(cont, LV_OBJ_FLAG_HIDDEN);
  always_visible = true;
}
