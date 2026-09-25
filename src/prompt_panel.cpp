#include "prompt_panel.h"
#include "spdlog/spdlog.h"

#include <cstring>
#include <string>

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
  lv_obj_set_style_pad_all(obj, 0, 0);
  lv_obj_set_style_border_width(obj, 0, 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
}

void style_dialog(lv_obj_t *obj)
{
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(obj, lv_color_hex(COLOR_BG), 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(obj, 1, 0);
  lv_obj_set_style_border_color(obj, lv_color_hex(COLOR_BORDER), 0);
  lv_obj_set_style_radius(obj, 10, 0);
  lv_obj_set_style_shadow_width(obj, 0, 0);
}

void style_button(lv_obj_t *button, const std::string &type)
{
  lv_obj_clear_flag(button, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(button, lv_color_hex(COLOR_CARD), 0);
  lv_obj_set_style_bg_opa(button, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(button, 1, 0);
  lv_obj_set_style_radius(button, 8, 0);
  lv_obj_set_style_shadow_width(button, 0, 0);
  lv_obj_set_style_bg_color(
      button,
      lv_color_hex(COLOR_CARD_PRESSED),
      LV_STATE_PRESSED);

  uint32_t border = COLOR_BORDER;
  if (type == "primary" || type == "info") {
    border = COLOR_ACCENT;
  } else if (type == "warning" || type == "error") {
    border = COLOR_DANGER;
  }

  lv_obj_set_style_border_color(button, lv_color_hex(border), 0);
  lv_obj_set_style_border_color(
      button,
      lv_color_hex(border),
      LV_STATE_PRESSED);
}

std::string trim_label(std::string value)
{
  while (!value.empty() &&
         (value.front() == ' ' || value.front() == '\t')) {
    value.erase(value.begin());
  }

  while (!value.empty() &&
         (value.back() == ' ' || value.back() == '\t')) {
    value.pop_back();
  }

  return value;
}

} // namespace

PromptPanel::PromptPanel(KWebSocketClient &websocket_client,
                         std::mutex &lock,
                         lv_obj_t *parent)
  : NotifyConsumer(lock)
  , ws(websocket_client)
  , prompt_cont(lv_obj_create(lv_scr_act()))
  , dialog_cont(lv_obj_create(prompt_cont))
  , header(lv_label_create(dialog_cont))
  , flex(lv_obj_create(dialog_cont))
  , footer_cont(lv_obj_create(dialog_cont))
  , button_group_cont(nullptr)
{
  (void)parent;

  lv_obj_set_size(prompt_cont, LV_PCT(100), LV_PCT(100));
  lv_obj_clear_flag(prompt_cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(prompt_cont, 0, 0);
  lv_obj_set_style_border_width(prompt_cont, 0, 0);
  lv_obj_set_style_radius(prompt_cont, 0, 0);
  lv_obj_set_style_bg_color(prompt_cont, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(prompt_cont, LV_OPA_70, 0);

  lv_obj_set_size(dialog_cont, LV_PCT(92), LV_PCT(86));
  lv_obj_center(dialog_cont);
  style_dialog(dialog_cont);
  lv_obj_set_style_pad_all(dialog_cont, 10, 0);
  lv_obj_set_style_pad_row(dialog_cont, 8, 0);

  static lv_coord_t rows[] = {
    52,
    LV_GRID_FR(1),
    62,
    LV_GRID_TEMPLATE_LAST
  };
  static lv_coord_t cols[] = {
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  lv_obj_set_grid_dsc_array(dialog_cont, cols, rows);

  lv_obj_set_grid_cell(
      header,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_CENTER, 0, 1);
  lv_label_set_text(header, "");
  lv_label_set_long_mode(header, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(header, LV_PCT(100));
  lv_obj_set_style_text_align(header, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(header, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(header, lv_color_hex(COLOR_TEXT), 0);

  lv_obj_set_grid_cell(
      flex,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 1, 1);
  style_transparent(flex);
  lv_obj_set_style_pad_row(flex, 8, 0);
  lv_obj_set_style_pad_right(flex, 3, 0);
  lv_obj_set_flex_flow(flex, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(
      flex,
      LV_FLEX_ALIGN_START,
      LV_FLEX_ALIGN_CENTER,
      LV_FLEX_ALIGN_CENTER);
  lv_obj_set_scroll_dir(flex, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(flex, LV_SCROLLBAR_MODE_AUTO);

  lv_obj_set_grid_cell(
      footer_cont,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 2, 1);
  style_transparent(footer_cont);
  lv_obj_set_style_pad_column(footer_cont, 8, 0);
  lv_obj_set_flex_flow(footer_cont, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(
      footer_cont,
      LV_FLEX_ALIGN_CENTER,
      LV_FLEX_ALIGN_CENTER,
      LV_FLEX_ALIGN_CENTER);

  ws.register_notify_update(this);
  ws.register_method_callback(
      "notify_gcode_response",
      "MainPanel",
      [this](json &data) {
        this->handle_macro_response(data);
      });

  background();
}

PromptPanel::~PromptPanel()
{
  if (prompt_cont != nullptr) {
    lv_obj_del(prompt_cont);
    prompt_cont = nullptr;
  }

  ws.unregister_notify_update(this);
}

void PromptPanel::consume(json &j)
{
  (void)j;
}

void PromptPanel::foreground()
{
  lv_obj_clear_flag(prompt_cont, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(prompt_cont);
}

void PromptPanel::background()
{
  lv_obj_add_flag(prompt_cont, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_background(prompt_cont);
}

void PromptPanel::handle_callback(lv_event_t *event)
{
  lv_obj_t *button = lv_event_get_current_target(event);
  if (button == nullptr) {
    return;
  }

  lv_obj_t *label = lv_obj_get_child(button, 0);
  lv_obj_t *command = lv_obj_get_child(button, 1);

  if (label != nullptr) {
    spdlog::debug(
        "prompt button: {}",
        lv_label_get_text(label));
  }

  if (command == nullptr) {
    return;
  }

  std::string cmd = lv_label_get_text(command);
  if (cmd.empty()) {
    return;
  }

  spdlog::debug("prompt command: {}", cmd);
  ws.gcode_script(cmd);
}

void PromptPanel::handle_macro_response(json &j)
{
  auto &value = j["/params/0"_json_pointer];
  if (value.is_null()) {
    return;
  }

  std::string response = value.template get<std::string>();
  if (response.find("// action:", 0) != 0) {
    return;
  }

  std::lock_guard<std::mutex> lock(lv_lock);

  std::string action = response.substr(10);
  spdlog::debug("prompt action: {}", action);

  if (action.find("prompt_begin") == 0) {
    std::string prompt_header = trim_label(action.substr(12));

    lv_obj_clean(footer_cont);
    lv_obj_clean(flex);
    button_group_cont = nullptr;

    lv_label_set_text(header, prompt_header.c_str());
    return;
  }

  if (action.find("prompt_text") == 0) {
    std::string prompt_text = trim_label(action.substr(11));

    lv_obj_t *text = lv_label_create(flex);
    lv_obj_set_width(text, LV_PCT(100));
    lv_label_set_long_mode(text, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(text, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(text, lv_color_hex(COLOR_TEXT), 0);
    lv_obj_set_style_text_align(text, LV_TEXT_ALIGN_LEFT, 0);
    lv_label_set_text(text, prompt_text.c_str());
    return;
  }

  if (action.find("prompt_button_group_start") == 0) {
    button_group_cont = lv_obj_create(flex);
    lv_obj_set_width(button_group_cont, LV_PCT(100));
    lv_obj_set_height(button_group_cont, LV_SIZE_CONTENT);
    style_transparent(button_group_cont);
    lv_obj_set_style_pad_row(button_group_cont, 8, 0);
    lv_obj_set_style_pad_column(button_group_cont, 8, 0);
    lv_obj_set_flex_flow(button_group_cont, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(
        button_group_cont,
        LV_FLEX_ALIGN_START,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER);
    return;
  }

  if (action.find("prompt_button_group_end") == 0) {
    button_group_cont = nullptr;
    return;
  }

  const bool footer_button =
      action.find("prompt_footer_button") == 0;
  const bool normal_button =
      action.find("prompt_button") == 0;

  if (footer_button || normal_button) {
    const size_t label_start =
        action.find("button", 0) + std::strlen("button");
    const size_t first = action.find("|", label_start);

    if (first == std::string::npos) {
      spdlog::warn("prompt button missing command: {}", action);
      return;
    }

    const size_t second = action.find("|", first + 1);

    std::string button_label =
        trim_label(action.substr(label_start, first - label_start));
    std::string button_command;
    std::string button_type = "none";

    if (second != std::string::npos) {
      button_command =
          action.substr(first + 1, second - first - 1);
      button_type =
          trim_label(action.substr(second + 1));
    } else {
      button_command = action.substr(first + 1);
    }

    lv_obj_t *parent = footer_cont;
    if (!footer_button) {
      parent = button_group_cont != nullptr
          ? button_group_cont
          : flex;
    }

    lv_obj_t *button = lv_btn_create(parent);
    style_button(button, button_type);
    lv_obj_set_height(button, 48);
    lv_obj_set_style_pad_all(button, 6, 0);

    if (footer_button) {
      lv_obj_set_flex_grow(button, 1);
      lv_obj_set_style_max_width(button, 112, 0);
    } else if (button_group_cont != nullptr) {
      lv_obj_set_width(button, LV_PCT(48));
    } else {
      lv_obj_set_width(button, LV_PCT(100));
    }

    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, button_label.c_str());
    lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
    lv_obj_set_width(label, LV_PCT(100));
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(COLOR_TEXT), 0);
    lv_obj_center(label);

    lv_obj_t *command_label = lv_label_create(button);
    lv_label_set_text(command_label, button_command.c_str());
    lv_obj_add_flag(command_label, LV_OBJ_FLAG_HIDDEN);

    lv_obj_add_event_cb(
        button,
        &PromptPanel::_handle_callback,
        LV_EVENT_PRESSED,
        this);
    return;
  }

  if (action.find("prompt_show") == 0) {
    foreground();
    return;
  }

  if (action.find("prompt_end") == 0) {
    background();
    lv_obj_clean(footer_cont);
    lv_obj_clean(flex);
    button_group_cont = nullptr;
    lv_label_set_text(header, "");
    return;
  }

  spdlog::debug("prompt action not supported: {}", action);
}
