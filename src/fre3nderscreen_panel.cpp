#include "fre3nderscreen_panel.h"

#include "config.h"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <map>
#include <unistd.h>

#ifdef FRE3NDERSCREEN_VERSION
#define FS_VERSION FRE3NDERSCREEN_VERSION
#else
#define FS_VERSION "dev-snapshot"
#endif

namespace {

constexpr uint32_t COLOR_BG = 0x080B0D;
constexpr uint32_t COLOR_CARD = 0x11171B;
constexpr uint32_t COLOR_CARD_PRESSED = 0x18242A;
constexpr uint32_t COLOR_BORDER = 0x25323A;
constexpr uint32_t COLOR_ACCENT = 0x00E5FF;
constexpr uint32_t COLOR_TEXT = 0xFFFFFF;
constexpr uint32_t COLOR_MUTED = 0xB8C0C5;

const std::map<int32_t, uint32_t> sleepsec_to_dd_idx = {
  {-1, 0},
  {300, 1},
  {600, 2},
  {1800, 3},
  {3600, 4},
  {18000, 5}
};

const std::map<std::string, int32_t> sleep_label_to_sec = {
  {"Never", -1},
  {"5 Minutes", 300},
  {"10 Minutes", 600},
  {"30 Minutes", 1800},
  {"1 Hour", 3600},
  {"5 Hours", 18000}
};

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

void style_transparent_container(lv_obj_t *obj)
{
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(obj, 0, 0);
  lv_obj_set_style_border_width(obj, 0, 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
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

void style_dropdown(lv_obj_t *dropdown)
{
  lv_obj_set_size(dropdown, 116, 34);
  lv_obj_set_style_bg_color(dropdown, lv_color_hex(COLOR_CARD_PRESSED), 0);
  lv_obj_set_style_border_width(dropdown, 1, 0);
  lv_obj_set_style_border_color(dropdown, lv_color_hex(COLOR_BORDER), 0);
  lv_obj_set_style_radius(dropdown, 6, 0);
  lv_obj_set_style_text_color(dropdown, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_set_style_text_font(dropdown, &lv_font_montserrat_12, 0);
}

lv_obj_t *create_setting_row(lv_obj_t *parent, const char *text)
{
  lv_obj_t *row = lv_obj_create(parent);
  style_transparent_container(row);
  lv_obj_set_width(row, LV_PCT(100));
  lv_obj_set_height(row, 48);

  lv_obj_t *label = lv_label_create(row);
  lv_label_set_text(label, text);
  lv_obj_set_style_text_color(label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
  lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);

  return row;
}

} // namespace

std::vector<std::string> Fre3nderScreenPanel::log_levels = {
  "trace",
  "debug",
  "info"
};

Fre3nderScreenPanel::Fre3nderScreenPanel()
  : cont(lv_obj_create(lv_scr_act()))
  , header_cont(lv_obj_create(cont))
  , back_btn(lv_btn_create(header_cont))
  , title_label(lv_label_create(header_cont))
  , content_cont(lv_obj_create(cont))
  , version_card(lv_obj_create(content_cont))
  , settings_card(lv_obj_create(content_cont))
  , display_sleep_dd(nullptr)
  , loglevel_dd(nullptr)
  , prompt_estop_toggle(nullptr)
  , z_icon_toggle(nullptr)
  , restart_btn(lv_btn_create(content_cont))
  , loglevel(1)
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
      &Fre3nderScreenPanel::_handle_callback,
      LV_EVENT_CLICKED,
      this);

  lv_obj_t *back_label = lv_label_create(back_btn);
  lv_label_set_text(back_label, LV_SYMBOL_LEFT);
  lv_obj_set_style_text_font(back_label, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(back_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(back_label);

  lv_label_set_text(title_label, "Fre3nderScreen");
  lv_obj_set_style_text_font(title_label, &lv_font_montserrat_18, 0);
  lv_obj_set_style_text_color(title_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(title_label);

  lv_obj_set_grid_cell(
      content_cont,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 1, 1);
  lv_obj_set_style_pad_all(content_cont, 0, 0);
  lv_obj_set_style_pad_row(content_cont, 8, 0);
  lv_obj_set_style_border_width(content_cont, 0, 0);
  lv_obj_set_style_bg_opa(content_cont, LV_OPA_TRANSP, 0);
  lv_obj_set_flex_flow(content_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(
      content_cont,
      LV_FLEX_ALIGN_START,
      LV_FLEX_ALIGN_START,
      LV_FLEX_ALIGN_START);

  style_card(version_card);
  lv_obj_set_width(version_card, LV_PCT(100));
  lv_obj_set_height(version_card, 62);
  lv_obj_set_style_pad_all(version_card, 10, 0);

  lv_obj_t *version_title = lv_label_create(version_card);
  lv_label_set_text(version_title, "Version");
  lv_obj_set_style_text_color(version_title, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_set_style_text_font(version_title, &lv_font_montserrat_14, 0);
  lv_obj_align(version_title, LV_ALIGN_TOP_LEFT, 0, 0);

  lv_obj_t *version_value = lv_label_create(version_card);
  lv_label_set_text(version_value, FS_VERSION);
  lv_obj_set_style_text_color(version_value, lv_color_hex(COLOR_MUTED), 0);
  lv_obj_set_style_text_font(version_value, &lv_font_montserrat_12, 0);
  lv_obj_align(version_value, LV_ALIGN_BOTTOM_LEFT, 0, 0);

  style_card(settings_card);
  lv_obj_set_width(settings_card, LV_PCT(100));
  lv_obj_set_height(settings_card, 212);
  lv_obj_set_style_pad_all(settings_card, 10, 0);
  lv_obj_set_style_pad_row(settings_card, 2, 0);
  lv_obj_set_flex_flow(settings_card, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(
      settings_card,
      LV_FLEX_ALIGN_START,
      LV_FLEX_ALIGN_START,
      LV_FLEX_ALIGN_START);

  Config *conf = Config::get_instance();

  lv_obj_t *row = create_setting_row(settings_card, "Display Sleep");
  display_sleep_dd = lv_dropdown_create(row);
  style_dropdown(display_sleep_dd);
  lv_obj_align(display_sleep_dd, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_dropdown_set_options(
      display_sleep_dd,
      "Never\n"
      "5 Minutes\n"
      "10 Minutes\n"
      "30 Minutes\n"
      "1 Hour\n"
      "5 Hours");

  auto value = conf->get_json("/display_sleep_sec");
  if (!value.is_null()) {
    auto sleep_sec = value.template get<int32_t>();
    const auto found = sleepsec_to_dd_idx.find(sleep_sec);
    if (found != sleepsec_to_dd_idx.end()) {
      lv_dropdown_set_selected(display_sleep_dd, found->second);
    }
  }
  lv_obj_add_event_cb(
      display_sleep_dd,
      &Fre3nderScreenPanel::_handle_callback,
      LV_EVENT_VALUE_CHANGED,
      this);

  row = create_setting_row(settings_card, "Log Level");
  loglevel_dd = lv_dropdown_create(row);
  style_dropdown(loglevel_dd);
  lv_obj_align(loglevel_dd, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_dropdown_set_options(loglevel_dd, "trace\ndebug\ninfo");

  value = conf->get_json("/log_level");
  if (!value.is_null()) {
    const auto selected = value.template get<std::string>();
    const auto found = std::find(log_levels.begin(), log_levels.end(), selected);
    if (found != log_levels.end()) {
      loglevel = std::distance(log_levels.begin(), found);
    }
  }
  lv_dropdown_set_selected(loglevel_dd, loglevel);
  lv_obj_add_event_cb(
      loglevel_dd,
      &Fre3nderScreenPanel::_handle_callback,
      LV_EVENT_VALUE_CHANGED,
      this);

  row = create_setting_row(settings_card, "Confirm E-stop");
  prompt_estop_toggle = lv_switch_create(row);
  lv_obj_align(prompt_estop_toggle, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_obj_set_style_bg_color(
      prompt_estop_toggle,
      lv_color_hex(COLOR_ACCENT),
      LV_PART_INDICATOR | LV_STATE_CHECKED);

  value = conf->get_json("/prompt_emergency_stop");
  if (value.is_null() || value.template get<bool>()) {
    lv_obj_add_state(prompt_estop_toggle, LV_STATE_CHECKED);
  } else {
    lv_obj_clear_state(prompt_estop_toggle, LV_STATE_CHECKED);
  }
  lv_obj_add_event_cb(
      prompt_estop_toggle,
      &Fre3nderScreenPanel::_handle_callback,
      LV_EVENT_VALUE_CHANGED,
      this);

  row = create_setting_row(settings_card, "Invert Z Icon");
  z_icon_toggle = lv_switch_create(row);
  lv_obj_align(z_icon_toggle, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_obj_set_style_bg_color(
      z_icon_toggle,
      lv_color_hex(COLOR_ACCENT),
      LV_PART_INDICATOR | LV_STATE_CHECKED);

  value = conf->get_json("/invert_z_icon");
  if (!value.is_null() && value.template get<bool>()) {
    lv_obj_add_state(z_icon_toggle, LV_STATE_CHECKED);
  } else {
    lv_obj_clear_state(z_icon_toggle, LV_STATE_CHECKED);
  }
  lv_obj_add_event_cb(
      z_icon_toggle,
      &Fre3nderScreenPanel::_handle_callback,
      LV_EVENT_VALUE_CHANGED,
      this);

  style_button(restart_btn);
  lv_obj_set_width(restart_btn, LV_PCT(100));
  lv_obj_set_height(restart_btn, 52);
  lv_obj_add_event_cb(
      restart_btn,
      &Fre3nderScreenPanel::_handle_callback,
      LV_EVENT_CLICKED,
      this);

  lv_obj_t *restart_label = lv_label_create(restart_btn);
  lv_label_set_text(restart_label, LV_SYMBOL_REFRESH "  Restart Screen");
  lv_obj_set_style_text_font(restart_label, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(restart_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(restart_label);
}

Fre3nderScreenPanel::~Fre3nderScreenPanel()
{
  if (cont != nullptr) {
    lv_obj_del(cont);
    cont = nullptr;
  }
}

void Fre3nderScreenPanel::foreground()
{
  lv_obj_move_foreground(cont);
}

void Fre3nderScreenPanel::handle_callback(lv_event_t *event)
{
  const lv_event_code_t code = lv_event_get_code(event);
  lv_obj_t *obj = lv_event_get_current_target(event);

  if (code == LV_EVENT_CLICKED) {
    if (obj == back_btn) {
      lv_obj_move_background(cont);
      return;
    }

    if (obj == restart_btn) {
      spdlog::trace("restart Fre3nderScreen pressed");

      std::string argv0;
      {
        std::ifstream cmdline("/proc/self/cmdline", std::ios::binary);
        std::getline(cmdline, argv0, '\0');
      }
      if (argv0.empty()) {
        spdlog::error("Failed to restart Fre3nderScreen: cannot read current command line");
        return;
      }

      ::execl("/proc/self/exe", argv0.c_str(), static_cast<char *>(nullptr));
      const int error = errno;
      spdlog::error("Failed to restart Fre3nderScreen: {}", std::strerror(error));
      return;
    }
  }

  if (code != LV_EVENT_VALUE_CHANGED) {
    return;
  }

  Config *conf = Config::get_instance();

  if (obj == display_sleep_dd) {
    char selected[64];
    lv_dropdown_get_selected_str(
        display_sleep_dd,
        selected,
        sizeof(selected));

    const auto found = sleep_label_to_sec.find(selected);
    if (found != sleep_label_to_sec.end()) {
      conf->set<int32_t>("/display_sleep_sec", found->second);
      conf->save();
    }
  } else if (obj == loglevel_dd) {
    const auto selected = lv_dropdown_get_selected(loglevel_dd);
    if (selected < log_levels.size() && selected != loglevel) {
      loglevel = selected;
      const auto level = spdlog::level::from_str(log_levels[loglevel]);

      spdlog::set_level(level);
      spdlog::flush_on(level);
      spdlog::debug("setting log_level to {}", log_levels[loglevel]);

      conf->set<std::string>("/log_level", log_levels[loglevel]);
      conf->save();
    }
  } else if (obj == prompt_estop_toggle) {
    const bool enabled =
        lv_obj_has_state(prompt_estop_toggle, LV_STATE_CHECKED);
    conf->set<bool>("/prompt_emergency_stop", enabled);
    conf->save();
  } else if (obj == z_icon_toggle) {
    const bool inverted =
        lv_obj_has_state(z_icon_toggle, LV_STATE_CHECKED);
    conf->set<bool>("/invert_z_icon", inverted);
    conf->save();
  }
}
