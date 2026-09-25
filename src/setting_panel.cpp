#include "setting_panel.h"

#include "config.h"
#include "spdlog/spdlog.h"
#include "subprocess.hpp"

#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;
namespace sp = subprocess;

LV_IMG_DECLARE(network_img);
LV_IMG_DECLARE(refresh_img);
LV_IMG_DECLARE(spoolman_img);
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
  lv_obj_set_style_pad_all(button, 10, 0);
  lv_obj_set_style_bg_color(button, lv_color_hex(COLOR_CARD), 0);
  lv_obj_set_style_border_width(button, 1, 0);
  lv_obj_set_style_border_color(button, lv_color_hex(COLOR_BORDER), 0);
  lv_obj_set_style_radius(button, 8, 0);
  lv_obj_set_style_shadow_width(button, 0, 0);
  lv_obj_set_style_bg_color(button, lv_color_hex(COLOR_CARD_PRESSED), LV_STATE_PRESSED);
  lv_obj_set_style_border_color(button, lv_color_hex(COLOR_ACCENT), LV_STATE_PRESSED);
  lv_obj_set_style_opa(button, LV_OPA_40, LV_STATE_DISABLED);
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
  lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(COLOR_TEXT), 0);

  return button;
}

} // namespace

SettingPanel::SettingPanel(KWebSocketClient &c,
                           std::mutex &l,
                           lv_obj_t *parent,
                           SpoolmanPanel &sm)
  : ws(c)
  , cont(lv_obj_create(parent))
  , wifi_panel(l)
  , sysinfo_panel()
  , spoolman_panel(sm)
  , wifi_btn(nullptr)
  , sysinfo_btn(nullptr)
  , restart_klipper_btn(nullptr)
  , restart_firmware_btn(nullptr)
  , spoolman_btn(nullptr)
  , screen_restart_btn(nullptr)
{
  lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
  lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(cont, 12, 0);
  lv_obj_set_style_pad_row(cont, 10, 0);
  lv_obj_set_style_pad_column(cont, 10, 0);
  lv_obj_set_style_border_width(cont, 0, 0);
  lv_obj_set_style_bg_color(cont, lv_color_hex(COLOR_BG), 0);

  static lv_coord_t rows[] = {
    LV_GRID_FR(1),
    LV_GRID_FR(1),
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  static lv_coord_t cols[] = {
    LV_GRID_FR(1),
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  lv_obj_set_grid_dsc_array(cont, cols, rows);

  wifi_btn = create_setting_card(
      cont, &network_img, "WiFi", &SettingPanel::_handle_callback, this);
  sysinfo_btn = create_setting_card(
      cont, &sysinfo_img, "System", &SettingPanel::_handle_callback, this);
  restart_klipper_btn = create_setting_card(
      cont, &refresh_img, "Restart\nKlipper", &SettingPanel::_handle_callback, this);
  restart_firmware_btn = create_setting_card(
      cont, &refresh_img, "Restart\nFirmware", &SettingPanel::_handle_callback, this);
  spoolman_btn = create_setting_card(
      cont, &spoolman_img, "Spoolman", &SettingPanel::_handle_callback, this);
  screen_restart_btn = create_setting_card(
      cont, &refresh_img, "Restart\nScreen", &SettingPanel::_handle_callback, this);

  struct CardPosition {
    lv_obj_t *button;
    uint8_t col;
    uint8_t row;
  };

  CardPosition cards[] = {
    {wifi_btn, 0, 0},
    {sysinfo_btn, 1, 0},
    {restart_klipper_btn, 0, 1},
    {restart_firmware_btn, 1, 1},
    {spoolman_btn, 0, 2},
    {screen_restart_btn, 1, 2},
  };

  for (const auto &card : cards) {
    lv_obj_set_grid_cell(
        card.button,
        LV_GRID_ALIGN_STRETCH, card.col, 1,
        LV_GRID_ALIGN_STRETCH, card.row, 1);
  }

  lv_obj_add_state(spoolman_btn, LV_STATE_DISABLED);
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

  if (button == wifi_btn) {
    spdlog::trace("wifi pressed");
    wifi_panel.foreground();
  } else if (button == sysinfo_btn) {
    spdlog::trace("setting system info pressed");
    sysinfo_panel.foreground();
  } else if (button == restart_klipper_btn) {
    spdlog::trace("setting restart klipper pressed");
    ws.send_jsonrpc("printer.restart");
  } else if (button == restart_firmware_btn) {
    spdlog::trace("setting restart firmware pressed");
    ws.send_jsonrpc("printer.firmware_restart");
  } else if (button == spoolman_btn) {
    spdlog::trace("setting spoolman pressed");
    spoolman_panel.foreground();
  } else if (button == screen_restart_btn) {
    spdlog::trace("restart Fre3nderScreen pressed");
    Config *conf = Config::get_instance();
    const auto init_script = conf->get<std::string>("/fre3nderscreen_init_script");
    const fs::path script(init_script);

    if (fs::exists(script) || init_script.rfind("service fre3nderscreen", 0) == 0) {
      sp::call({init_script, "restart"});
    } else {
      spdlog::warn("Failed to restart Fre3nderScreen. Did not find restart script.");
    }
  }
}

void SettingPanel::enable_spoolman()
{
  lv_obj_clear_state(spoolman_btn, LV_STATE_DISABLED);
}
