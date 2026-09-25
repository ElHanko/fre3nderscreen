#include "console_panel.h"

#include "state.h"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <cctype>

LV_FONT_DECLARE(dejavusans_mono_14);

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

void style_card(lv_obj_t *card)
{
  lv_obj_set_style_bg_color(card, lv_color_hex(COLOR_CARD), 0);
  lv_obj_set_style_border_width(card, 1, 0);
  lv_obj_set_style_border_color(card, lv_color_hex(COLOR_BORDER), 0);
  lv_obj_set_style_radius(card, 8, 0);
  lv_obj_set_style_shadow_width(card, 0, 0);
}

} // namespace

ConsolePanel::ConsolePanel(KWebSocketClient &websocket_client,
                           std::mutex &lock)
  : ws(websocket_client)
  , lv_lock(lock)
  , console_cont(lv_obj_create(lv_scr_act()))
  , header_cont(lv_obj_create(console_cont))
  , back_btn(lv_btn_create(header_cont))
  , title_label(lv_label_create(header_cont))
  , content_cont(lv_obj_create(console_cont))
  , output_label(lv_label_create(content_cont))
  , output(lv_textarea_create(content_cont))
  , history_label(lv_label_create(content_cont))
  , macro_list(lv_table_create(content_cont))
  , input_cont(lv_obj_create(content_cont))
  , input(lv_textarea_create(input_cont))
  , clear_btn(lv_btn_create(input_cont))
  , send_btn(lv_btn_create(input_cont))
  , kb(lv_keyboard_create(content_cont))
{
  lv_obj_move_background(console_cont);
  lv_obj_set_size(console_cont, LV_PCT(100), LV_PCT(100));
  lv_obj_clear_flag(console_cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(console_cont, 8, 0);
  lv_obj_set_style_pad_row(console_cont, 8, 0);
  lv_obj_set_style_border_width(console_cont, 0, 0);
  lv_obj_set_style_bg_color(console_cont, lv_color_hex(COLOR_BG), 0);

  static lv_coord_t root_rows[] = {
    42,
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  static lv_coord_t root_cols[] = {
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  lv_obj_set_grid_dsc_array(console_cont, root_cols, root_rows);

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
      &ConsolePanel::_handle_back,
      LV_EVENT_CLICKED,
      this);

  lv_obj_t *back_label = lv_label_create(back_btn);
  lv_label_set_text(back_label, LV_SYMBOL_LEFT);
  lv_obj_set_style_text_font(back_label, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(back_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(back_label);

  lv_label_set_text(title_label, "Console");
  lv_obj_set_style_text_font(title_label, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(title_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(title_label);

  lv_obj_set_grid_cell(
      content_cont,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 1, 1);
  style_transparent(content_cont);
  lv_obj_set_style_pad_row(content_cont, 6, 0);
  lv_obj_set_flex_flow(content_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(
      content_cont,
      LV_FLEX_ALIGN_START,
      LV_FLEX_ALIGN_START,
      LV_FLEX_ALIGN_START);

  lv_label_set_text(output_label, "Output");
  lv_obj_set_style_text_font(output_label, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(output_label, lv_color_hex(COLOR_MUTED), 0);

  style_card(output);
  lv_obj_set_width(output, LV_PCT(100));
  lv_obj_set_flex_grow(output, 1);
  lv_obj_set_style_text_font(output, &dejavusans_mono_14, 0);
  lv_obj_set_style_text_color(output, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_set_style_bg_color(output, lv_color_hex(COLOR_CARD), 0);
  lv_obj_set_style_pad_all(output, 8, 0);
  lv_obj_set_style_border_color(
      output,
      lv_color_hex(COLOR_BORDER),
      LV_STATE_FOCUSED | LV_PART_CURSOR);
  lv_textarea_set_cursor_click_pos(output, false);

  lv_label_set_text(history_label, "History & Commands");
  lv_obj_set_style_text_font(history_label, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(history_label, lv_color_hex(COLOR_MUTED), 0);

  style_card(macro_list);
  lv_obj_set_size(macro_list, LV_PCT(100), 112);
  lv_obj_set_style_text_font(macro_list, &dejavusans_mono_14, 0);
  lv_obj_set_style_text_color(macro_list, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_set_style_bg_color(
      macro_list,
      lv_color_hex(COLOR_CARD),
      LV_PART_ITEMS);
  lv_obj_set_style_border_width(
      macro_list,
      1,
      LV_PART_ITEMS);
  lv_obj_set_style_border_color(
      macro_list,
      lv_color_hex(COLOR_BORDER),
      LV_PART_ITEMS);
  lv_table_set_col_width(macro_list, 0, 240);
  lv_obj_add_event_cb(
      macro_list,
      &ConsolePanel::_handle_select_macro,
      LV_EVENT_ALL,
      this);
  lv_obj_set_scroll_dir(macro_list, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(macro_list, LV_SCROLLBAR_MODE_AUTO);

  style_transparent(input_cont);
  lv_obj_set_size(input_cont, LV_PCT(100), 42);
  lv_obj_set_style_pad_column(input_cont, 6, 0);
  lv_obj_set_flex_flow(input_cont, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(
      input_cont,
      LV_FLEX_ALIGN_START,
      LV_FLEX_ALIGN_CENTER,
      LV_FLEX_ALIGN_CENTER);

  lv_obj_set_flex_grow(input, 1);
  lv_obj_set_height(input, 42);
  lv_textarea_set_one_line(input, true);
  lv_obj_set_style_text_font(input, &dejavusans_mono_14, 0);
  lv_obj_set_style_text_color(input, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_set_style_bg_color(input, lv_color_hex(COLOR_CARD), 0);
  lv_obj_set_style_border_width(input, 1, 0);
  lv_obj_set_style_border_color(input, lv_color_hex(COLOR_BORDER), 0);
  lv_obj_set_style_border_color(input, lv_color_hex(COLOR_ACCENT), LV_STATE_FOCUSED);
  lv_obj_set_style_radius(input, 8, 0);
  lv_obj_add_event_cb(
      input,
      &ConsolePanel::_handle_kb_input,
      LV_EVENT_ALL,
      this);

  style_button(clear_btn);
  lv_obj_set_size(clear_btn, 42, 42);
  lv_obj_add_event_cb(
      clear_btn,
      &ConsolePanel::_handle_clear_input,
      LV_EVENT_CLICKED,
      this);

  lv_obj_t *clear_label = lv_label_create(clear_btn);
  lv_label_set_text(clear_label, LV_SYMBOL_CLOSE);
  lv_obj_set_style_text_color(clear_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(clear_label);

  style_button(send_btn);
  lv_obj_set_size(send_btn, 42, 42);
  lv_obj_add_event_cb(
      send_btn,
      &ConsolePanel::_handle_send_macro,
      LV_EVENT_CLICKED,
      this);

  lv_obj_t *send_label = lv_label_create(send_btn);
  lv_label_set_text(send_label, LV_SYMBOL_NEW_LINE);
  lv_obj_set_style_text_color(send_label, lv_color_hex(COLOR_ACCENT), 0);
  lv_obj_center(send_label);

  lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_width(kb, LV_PCT(100));
  lv_obj_set_height(kb, 180);
  lv_obj_set_style_text_font(kb, &lv_font_montserrat_16, LV_STATE_DEFAULT);

  ws.register_method_callback(
      "notify_gcode_response",
      "ConsolePanel",
      [this](json &data) { this->handle_macro_response(data); });
}

ConsolePanel::~ConsolePanel()
{
  if (console_cont != nullptr) {
    lv_obj_del(console_cont);
    console_cont = nullptr;
  }
}

lv_obj_t *ConsolePanel::get_container()
{
  return console_cont;
}

void ConsolePanel::foreground()
{
  lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
  lv_keyboard_set_textarea(kb, nullptr);
  lv_obj_move_foreground(console_cont);
}

void ConsolePanel::handle_back(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
    return;
  }

  lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
  lv_keyboard_set_textarea(kb, nullptr);
  lv_obj_move_background(console_cont);
}

void ConsolePanel::handle_kb_input(lv_event_t *event)
{
  const lv_event_code_t code = lv_event_get_code(event);

  if (code == LV_EVENT_FOCUSED) {
    lv_keyboard_set_textarea(kb, input);
    lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
  }

  if (code == LV_EVENT_DEFOCUSED) {
    lv_keyboard_set_textarea(kb, nullptr);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
  }

  if (code == LV_EVENT_VALUE_CHANGED) {
    lv_obj_scroll_to_y(macro_list, 0, LV_ANIM_OFF);

    const std::string cmd = lv_textarea_get_text(input);
    if (cmd.find_first_of(' ') == std::string::npos) {
      std::string upper_cmd;
      std::transform(
          cmd.begin(),
          cmd.end(),
          std::back_inserter(upper_cmd),
          [](unsigned char c) { return std::toupper(c); });

      if (!all_macros.empty() || !history.empty()) {
        uint16_t index = 0;

        for (const auto &entry : history) {
          if (entry.rfind(upper_cmd, 0) == 0 ||
              entry.rfind(cmd, 0) == 0) {
            lv_table_set_cell_value(
                macro_list,
                index++,
                0,
                entry.c_str());
          }
        }

        for (const auto &entry : all_macros) {
          if (entry.rfind(upper_cmd, 0) == 0 ||
              entry.rfind(cmd, 0) == 0) {
            lv_table_set_cell_value(
                macro_list,
                index++,
                0,
                entry.c_str());
          }
        }

        lv_table_set_row_cnt(macro_list, index);
      }
    }
  }

  if (code == LV_EVENT_READY) {
    spdlog::debug("keyboard ready");

    const char *cmd = lv_textarea_get_text(input);
    if (cmd == nullptr || cmd[0] == '\0') {
      return;
    }

    lv_textarea_add_text(output, "> ");
    lv_textarea_add_text(output, cmd);
    lv_textarea_add_text(output, "\n");
    ws.gcode_script(cmd);

    if (!history.empty()) {
      const auto &front = history.front();

      if (front != std::string(cmd)) {
        if (history.size() >= 20) {
          history.pop_back();
        }
        history.push_front(cmd);

        json request = {
          {"namespace", "fluidd"},
          {"key", "console.commandHistory"},
          {"value", history}
        };

        ws.send_jsonrpc("server.database.post_item", request);
      }
    }

    lv_textarea_set_text(input, "");

    uint32_t index = 0;
    for (const auto &entry : history) {
      lv_table_set_cell_value(
          macro_list,
          index++,
          0,
          entry.c_str());
    }

    for (const auto &entry : all_macros) {
      lv_table_set_cell_value(
          macro_list,
          index++,
          0,
          entry.c_str());
    }
  }
}

void ConsolePanel::handle_select_macro(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED) {
    return;
  }

  uint16_t row;
  uint16_t col;
  lv_table_get_selected_cell(macro_list, &row, &col);

  const char *macro = lv_table_get_cell_value(macro_list, row, col);
  lv_textarea_set_text(input, macro);
}

void ConsolePanel::handle_macros(json &data)
{
  uint32_t index = 0;

  auto &db_history =
      State::get_instance()->get_data("/console/commandHistory"_json_pointer);

  if (!db_history.is_null()) {
    history = db_history.template get<std::list<std::string>>();
  }

  if (data.contains("result")) {
    const auto &macros = data["result"];
    for (const auto &entry : macros.items()) {
      all_macros.push_back(entry.key());
    }
  }

  std::lock_guard<std::mutex> lock(lv_lock);

  for (const auto &entry : history) {
    lv_table_set_cell_value(
        macro_list,
        index++,
        0,
        entry.c_str());
  }

  for (const auto &entry : all_macros) {
    lv_table_set_cell_value(
        macro_list,
        index++,
        0,
        entry.c_str());
  }
}

void ConsolePanel::handle_macro_response(json &data)
{
  if (!data.contains("params")) {
    return;
  }

  std::lock_guard<std::mutex> lock(lv_lock);

  for (auto &line : data["params"]) {
    lv_textarea_add_text(
        output,
        line.template get<std::string>().c_str());
    lv_textarea_add_text(output, "\n");
  }
}

void ConsolePanel::handle_send_macro(lv_event_t *event)
{
  if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
    lv_event_send(input, LV_EVENT_READY, this);
  }
}

void ConsolePanel::handle_clear_input(lv_event_t *event)
{
  if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
    lv_textarea_set_text(input, "");
  }
}
