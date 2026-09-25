#include "main_panel.h"
#include "state.h"
#include "lvgl/lvgl.h"
#include "spdlog/spdlog.h"

#include <dirent.h>
#include <fstream>
#include <string>
#include <sys/stat.h>

LV_IMG_DECLARE(filament_img);
LV_IMG_DECLARE(light_img);
LV_IMG_DECLARE(network_img);
LV_IMG_DECLARE(move);
LV_IMG_DECLARE(extruder);
LV_IMG_DECLARE(bed);
LV_IMG_DECLARE(fan);

namespace {

constexpr uint16_t TAB_HOME = 0;
constexpr uint16_t TAB_CONTROL = 1;
constexpr uint16_t TAB_FILES = 2;
constexpr uint16_t TAB_MACROS = 3;
constexpr uint16_t TAB_CONSOLE = 4;
constexpr uint16_t TAB_TUNE = 5;
constexpr uint16_t TAB_SETTINGS = 6;
constexpr uint16_t TAB_MORE = 7;
constexpr lv_coord_t FOOTER_HEIGHT = 56;

enum class NetworkLink {
  None,
  Lan,
  Wifi,
};

NetworkLink detect_network_link()
{
  DIR *dir = opendir("/sys/class/net");
  if (dir == NULL) {
    return NetworkLink::None;
  }

  bool lan = false;
  bool wifi = false;

  while (dirent *entry = readdir(dir)) {
    const std::string iface = entry->d_name;
    if (iface == "." || iface == ".." || iface == "lo") {
      continue;
    }

    std::ifstream state_file("/sys/class/net/" + iface + "/operstate");
    std::string state;
    state_file >> state;
    if (state != "up") {
      continue;
    }

    struct stat st {};
    if (stat(("/sys/class/net/" + iface + "/wireless").c_str(), &st) == 0) {
      wifi = true;
    } else {
      lan = true;
    }
  }

  closedir(dir);

  if (lan) {
    return NetworkLink::Lan;
  }
  if (wifi) {
    return NetworkLink::Wifi;
  }
  return NetworkLink::None;
}

void style_plain_button(lv_obj_t *button)
{
  lv_obj_set_style_bg_opa(button, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(button, 0, 0);
  lv_obj_set_style_shadow_width(button, 0, 0);
  lv_obj_set_style_radius(button, 0, 0);
}

void style_dark_button(lv_obj_t *button)
{
  lv_obj_set_style_bg_color(button, lv_color_hex(0x11171B), 0);
  lv_obj_set_style_border_width(button, 1, 0);
  lv_obj_set_style_border_color(button, lv_color_hex(0x25323A), 0);
  lv_obj_set_style_shadow_width(button, 0, 0);
  lv_obj_set_style_radius(button, 8, 0);
  lv_obj_set_style_text_color(button, lv_color_hex(0xFFFFFF), 0);
}

lv_obj_t *create_control_card(lv_obj_t *parent,
                              const void *image,
                              const char *text,
                              lv_event_cb_t callback,
                              void *user_data)
{
  lv_obj_t *button = lv_btn_create(parent);
  style_dark_button(button);
  lv_obj_clear_flag(button, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(button, 10, 0);
  lv_obj_set_style_bg_color(button, lv_color_hex(0x18242A), LV_STATE_PRESSED);
  lv_obj_set_style_border_color(button, lv_color_hex(0x00E5FF), LV_STATE_PRESSED);
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
  lv_obj_set_style_img_recolor(icon, lv_color_hex(0xFFFFFF), 0);

  lv_obj_t *label = lv_label_create(button);
  lv_label_set_text(label, text);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);

  return button;
}

} // namespace

MainPanel::MainPanel(KWebSocketClient &websocket,
                     std::mutex &lock,
                     SpoolmanPanel &sm)
  : NotifyConsumer(lock)
  , ws(websocket)
  , homing_panel(ws, lock)
  , fan_panel(ws, lock)
  , led_panel(ws, lock)
  , tabview(lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 0))
  , main_tab(lv_tabview_add_tab(tabview, "Home"))
  , control_tab(lv_tabview_add_tab(tabview, "Control"))
  , files_tab(lv_tabview_add_tab(tabview, "Files"))
  , macros_tab(lv_tabview_add_tab(tabview, "Macros"))
  , macros_panel(ws, lock, macros_tab)
  , console_tab(lv_tabview_add_tab(tabview, "Console"))
  , console_panel(ws, lock, console_tab)
  , printertune_tab(lv_tabview_add_tab(tabview, "Tune"))
  , setting_tab(lv_tabview_add_tab(tabview, "Settings"))
  , setting_panel(websocket, lock, setting_tab, sm)
  , more_tab(lv_tabview_add_tab(tabview, "More"))
  , main_cont(lv_obj_create(main_tab))
  , print_status_panel(websocket, lock, main_cont)
  , print_panel(ws, lock, files_tab, print_status_panel)
  , printertune_panel(ws, lock, printertune_tab, print_status_panel.get_finetune_panel())
  , numpad(Numpad(main_cont))
  , extruder_panel(ws, lock, numpad, sm)
  , prompt_panel(websocket, lock, main_cont)
  , spoolman_panel(sm)
  , control_cont(lv_obj_create(control_tab))
  , more_cont(lv_obj_create(more_tab))
  , more_macros_btn(lv_btn_create(more_cont))
  , more_console_btn(lv_btn_create(more_cont))
  , more_tune_btn(lv_btn_create(more_cont))
  , footer_cont(NULL)
  , nav_home_btn(NULL)
  , nav_control_btn(NULL)
  , nav_files_btn(NULL)
  , nav_settings_btn(NULL)
  , nav_more_btn(NULL)
  , header_cont(lv_obj_create(main_cont))
  , brand_label(lv_label_create(header_cont))
  , status_img(lv_img_create(header_cont))
  , status_label(lv_label_create(header_cont))
  , status_timer(NULL)
  , warning_active(false)
  , temp_cont(lv_obj_create(main_cont))
  , temp_chart(lv_chart_create(main_cont))
{
    ws.register_notify_update(this);
}

MainPanel::~MainPanel() {
  if (status_timer != NULL) {
    lv_timer_del(status_timer);
    status_timer = NULL;
  }

  if (footer_cont != NULL) {
    lv_obj_del(footer_cont);
    footer_cont = NULL;
  }

  if (tabview != NULL) {
    lv_obj_del(tabview);
    tabview = NULL;
  }

  sensors.clear();
}

void MainPanel::subscribe() {
  spdlog::trace("main panel subscribing");
  ws.send_jsonrpc("printer.gcode.help", [this](json &d) { console_panel.handle_macros(d); });
  print_panel.subscribe();
}

PrinterTunePanel& MainPanel::get_tune_panel() {
  return printertune_panel;
}

void MainPanel::init(json &j) {
  std::lock_guard<std::mutex> lock(lv_lock);
  for (const auto &el : sensors) {
    auto target_value = j[json::json_pointer(fmt::format("/result/status/{}/target", el.first))];
    if (!target_value.is_null()) {
      int target = target_value.template get<int>();
      el.second->update_target(target);
    }

    auto temp_value = j[json::json_pointer(fmt::format("/result/status/{}/temperature", el.first))];
    if (!temp_value.is_null()) {
      int value = temp_value.template get<int>();
      el.second->update_series(value);
      el.second->update_value(value);
    }
  }

  macros_panel.populate();

  auto fans = State::get_instance()->get_display_fans();
  print_status_panel.init(fans);
  printertune_panel.init(j);
}

void MainPanel::consume(json &j) {
  std::lock_guard<std::mutex> lock(lv_lock);
  for (const auto &el : sensors) {
    auto target_value = j[json::json_pointer(fmt::format("/params/0/{}/target", el.first))];
    if (!target_value.is_null()) {
      int target = target_value.template get<int>();
      el.second->update_target(target);
    }

    auto temp_value = j[json::json_pointer(fmt::format("/params/0/{}/temperature", el.first))];
    if (!temp_value.is_null()) {
      int value = temp_value.template get<int>();
      el.second->update_series(value);
      el.second->update_value(value);
    }
  }
}

void MainPanel::create_panel() {
  lv_obj_set_size(
      tabview,
      LV_PCT(100),
      lv_disp_get_ver_res(NULL) - FOOTER_HEIGHT);
  lv_obj_align(tabview, LV_ALIGN_TOP_MID, 0, 0);

  lv_obj_t *tab_btns = lv_tabview_get_tab_btns(tabview);
  lv_obj_set_size(tab_btns, 0, 0);

  lv_obj_t *tab_content = lv_tabview_get_content(tabview);
  lv_obj_clear_flag(tab_content, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *tabs[] = {
    main_tab,
    control_tab,
    files_tab,
    macros_tab,
    console_tab,
    printertune_tab,
    setting_tab,
    more_tab,
  };

  for (lv_obj_t *tab : tabs) {
    lv_obj_set_style_pad_all(tab, 0, 0);
    lv_obj_set_style_border_width(tab, 0, 0);
    lv_obj_set_style_bg_color(tab, lv_color_hex(0x080B0D), 0);
  }

  create_main(main_tab);
  create_control();
  create_more();
  create_footer();

  lv_tabview_set_act(tabview, TAB_HOME, LV_ANIM_OFF);
  set_nav_active(nav_home_btn);

  refresh_header_status();
  status_timer = lv_timer_create(&MainPanel::_refresh_header_status, 2000, this);
}

void MainPanel::set_header_warning(bool enabled) {
  std::lock_guard<std::mutex> lock(lv_lock);
  warning_active = enabled;
  refresh_header_status();
}

void MainPanel::create_footer()
{
  footer_cont = lv_obj_create(lv_scr_act());
  lv_obj_set_size(footer_cont, LV_PCT(100), FOOTER_HEIGHT);
  lv_obj_align(footer_cont, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_clear_flag(footer_cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(footer_cont, 0, 0);
  lv_obj_set_style_pad_column(footer_cont, 0, 0);
  lv_obj_set_style_border_width(footer_cont, 0, 0);
  lv_obj_set_style_bg_color(footer_cont, lv_color_hex(0x0D1216), 0);
  lv_obj_set_flex_flow(footer_cont, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(
      footer_cont,
      LV_FLEX_ALIGN_SPACE_EVENLY,
      LV_FLEX_ALIGN_CENTER,
      LV_FLEX_ALIGN_CENTER);

  struct NavDef {
    lv_obj_t **button;
    const char *icon;
    const char *text;
  };

  NavDef navs[] = {
    {&nav_home_btn, LV_SYMBOL_HOME, "Home"},
    {&nav_control_btn, LV_SYMBOL_EDIT, "Control"},
    {&nav_files_btn, LV_SYMBOL_DIRECTORY, "Files"},
    {&nav_settings_btn, LV_SYMBOL_SETTINGS, "Settings"},
    {&nav_more_btn, LV_SYMBOL_BARS, "More"},
  };

  for (auto &nav : navs) {
    *nav.button = lv_btn_create(footer_cont);
    style_plain_button(*nav.button);
    lv_obj_set_height(*nav.button, LV_PCT(100));
    lv_obj_set_flex_grow(*nav.button, 1);
    lv_obj_set_style_pad_all(*nav.button, 0, 0);
    lv_obj_set_style_pad_row(*nav.button, 1, 0);
    lv_obj_set_flex_flow(*nav.button, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(
        *nav.button,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER);
    lv_obj_add_event_cb(*nav.button, &MainPanel::_handle_nav_cb, LV_EVENT_CLICKED, this);

    lv_obj_t *icon = lv_label_create(*nav.button);
    lv_label_set_text(icon, nav.icon);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(icon, lv_color_hex(0xFFFFFF), 0);

    lv_obj_t *label = lv_label_create(*nav.button);
    lv_label_set_text(label, nav.text);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
  }

  lv_obj_move_foreground(footer_cont);
}

void MainPanel::set_nav_active(lv_obj_t *button)
{
  lv_obj_t *buttons[] = {
    nav_home_btn,
    nav_control_btn,
    nav_files_btn,
    nav_settings_btn,
    nav_more_btn,
  };

  for (lv_obj_t *nav : buttons) {
    if (nav == NULL) {
      continue;
    }
    const lv_color_t color =
        nav == button ? lv_color_hex(0x00E5FF) : lv_color_hex(0xFFFFFF);
    lv_obj_t *icon = lv_obj_get_child(nav, 0);
    lv_obj_t *label = lv_obj_get_child(nav, 1);
    if (icon != NULL) {
      lv_obj_set_style_text_color(icon, color, 0);
    }
    if (label != NULL) {
      lv_obj_set_style_text_color(label, color, 0);
    }
  }
}

void MainPanel::handle_nav_cb(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
    return;
  }

  lv_obj_t *target = lv_event_get_current_target(event);

  if (target == nav_home_btn) {
    lv_tabview_set_act(tabview, TAB_HOME, LV_ANIM_OFF);
    set_nav_active(nav_home_btn);
  } else if (target == nav_control_btn) {
    lv_tabview_set_act(tabview, TAB_CONTROL, LV_ANIM_OFF);
    set_nav_active(nav_control_btn);
  } else if (target == nav_files_btn) {
    lv_tabview_set_act(tabview, TAB_FILES, LV_ANIM_OFF);
    set_nav_active(nav_files_btn);
  } else if (target == nav_settings_btn) {
    lv_tabview_set_act(tabview, TAB_SETTINGS, LV_ANIM_OFF);
    set_nav_active(nav_settings_btn);
  } else if (target == nav_more_btn) {
    lv_tabview_set_act(tabview, TAB_MORE, LV_ANIM_OFF);
    set_nav_active(nav_more_btn);
  }
}

void MainPanel::create_control()
{
  lv_obj_set_size(control_cont, LV_PCT(100), LV_PCT(100));
  lv_obj_clear_flag(control_cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(control_cont, 12, 0);
  lv_obj_set_style_pad_row(control_cont, 10, 0);
  lv_obj_set_style_pad_column(control_cont, 10, 0);
  lv_obj_set_style_border_width(control_cont, 0, 0);
  lv_obj_set_style_bg_color(control_cont, lv_color_hex(0x080B0D), 0);

  static lv_coord_t rows[] = {
    LV_GRID_FR(1),
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  static lv_coord_t cols[] = {
    LV_GRID_FR(1),
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };

  lv_obj_set_grid_dsc_array(control_cont, cols, rows);

  struct ControlDef {
    const void *image;
    const char *text;
    lv_event_cb_t callback;
    uint8_t col;
    uint8_t row;
  };

  ControlDef controls[] = {
    {&move, "Homing", &MainPanel::_handle_homing_cb, 0, 0},
    {&filament_img, "Extrude", &MainPanel::_handle_extrude_cb, 1, 0},
    {&fan, "Fans", &MainPanel::_handle_fanpanel_cb, 0, 1},
    {&light_img, "LED", &MainPanel::_handle_ledpanel_cb, 1, 1},
  };

  for (const auto &control : controls) {
    lv_obj_t *button = create_control_card(
        control_cont,
        control.image,
        control.text,
        control.callback,
        this);

    lv_obj_set_grid_cell(
        button,
        LV_GRID_ALIGN_STRETCH, control.col, 1,
        LV_GRID_ALIGN_STRETCH, control.row, 1);
  }
}

void MainPanel::create_more()
{
  lv_obj_set_size(more_cont, LV_PCT(100), LV_PCT(100));
  lv_obj_clear_flag(more_cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(more_cont, 12, 0);
  lv_obj_set_style_pad_row(more_cont, 10, 0);
  lv_obj_set_style_border_width(more_cont, 0, 0);
  lv_obj_set_style_bg_color(more_cont, lv_color_hex(0x080B0D), 0);
  lv_obj_set_flex_flow(more_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(
      more_cont,
      LV_FLEX_ALIGN_START,
      LV_FLEX_ALIGN_CENTER,
      LV_FLEX_ALIGN_CENTER);

  struct MoreDef {
    lv_obj_t *button;
    const char *text;
  };

  MoreDef buttons[] = {
    {more_macros_btn, "Macros"},
    {more_console_btn, "Console"},
    {more_tune_btn, "Tune"},
  };

  for (auto &entry : buttons) {
    lv_obj_set_size(entry.button, LV_PCT(100), 64);
    style_dark_button(entry.button);
    lv_obj_add_event_cb(entry.button, &MainPanel::_handle_more_cb, LV_EVENT_CLICKED, this);

    lv_obj_t *label = lv_label_create(entry.button);
    lv_label_set_text(label, entry.text);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(label);
  }
}

void MainPanel::handle_more_cb(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
    return;
  }

  lv_obj_t *target = lv_event_get_current_target(event);
  set_nav_active(nav_more_btn);

  if (target == more_macros_btn) {
    lv_tabview_set_act(tabview, TAB_MACROS, LV_ANIM_OFF);
  } else if (target == more_console_btn) {
    lv_tabview_set_act(tabview, TAB_CONSOLE, LV_ANIM_OFF);
  } else if (target == more_tune_btn) {
    lv_tabview_set_act(tabview, TAB_TUNE, LV_ANIM_OFF);
  }
}

void MainPanel::refresh_header_status()
{
  lv_obj_add_flag(status_img, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(status_label, LV_OBJ_FLAG_HIDDEN);

  if (warning_active) {
    lv_label_set_text(status_label, LV_SYMBOL_WARNING);
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xFF3B30), 0);
    lv_obj_clear_flag(status_label, LV_OBJ_FLAG_HIDDEN);
    return;
  }

  switch (detect_network_link()) {
    case NetworkLink::Lan:
      lv_img_set_src(status_img, &network_img);
      lv_img_set_zoom(status_img, 128);
      lv_obj_set_style_img_recolor_opa(status_img, LV_OPA_COVER, 0);
      lv_obj_set_style_img_recolor(status_img, lv_color_hex(0x00E5FF), 0);
      lv_obj_clear_flag(status_img, LV_OBJ_FLAG_HIDDEN);
      break;

    case NetworkLink::Wifi:
      lv_label_set_text(status_label, LV_SYMBOL_WIFI);
      lv_obj_set_style_text_color(status_label, lv_color_hex(0x00E5FF), 0);
      lv_obj_clear_flag(status_label, LV_OBJ_FLAG_HIDDEN);
      break;

    case NetworkLink::None:
      break;
  }
}

void MainPanel::handle_homing_cb(lv_event_t *event) {
  spdlog::trace("clicked homing1");
  if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
    spdlog::trace("clicked homing");
    homing_panel.foreground();
  }
}

void MainPanel::handle_extrude_cb(lv_event_t *event) {
  if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
    spdlog::trace("clicked extruder");
    extruder_panel.foreground();
  }
}

void MainPanel::handle_fanpanel_cb(lv_event_t *event) {
  if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
    spdlog::trace("clicked fan panel");
    fan_panel.foreground();
  }
}

void MainPanel::handle_ledpanel_cb(lv_event_t *event) {
  if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
    spdlog::trace("clicked led panel");
    led_panel.foreground();
  }
}

void MainPanel::create_main(lv_obj_t *parent)
{
    lv_obj_set_size(main_cont, LV_PCT(100), LV_PCT(100));
    lv_obj_clear_flag(main_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(main_cont, 6, 0);
    lv_obj_set_style_pad_row(main_cont, 6, 0);
    lv_obj_set_style_border_width(main_cont, 0, 0);
    lv_obj_set_style_bg_color(main_cont, lv_color_hex(0x080B0D), 0);

    static lv_coord_t rows[] = {
      44,
      58,
      LV_GRID_FR(1),
      LV_GRID_TEMPLATE_LAST
    };
    static lv_coord_t cols[] = {
      LV_GRID_FR(1),
      LV_GRID_TEMPLATE_LAST
    };

    lv_obj_set_grid_dsc_array(main_cont, cols, rows);

    lv_obj_clear_flag(header_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(header_cont, 10, 0);
    lv_obj_set_style_radius(header_cont, 8, 0);
    lv_obj_set_style_border_width(header_cont, 1, 0);
    lv_obj_set_style_border_color(header_cont, lv_color_hex(0x25323A), 0);
    lv_obj_set_style_bg_color(header_cont, lv_color_hex(0x11171B), 0);
    lv_obj_set_grid_cell(
        header_cont,
        LV_GRID_ALIGN_STRETCH, 0, 1,
        LV_GRID_ALIGN_STRETCH, 0, 1);

    lv_label_set_text(brand_label, "FRE3NDER");
    lv_obj_set_style_text_font(brand_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(brand_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(brand_label, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_20, 0);
    lv_obj_align(status_label, LV_ALIGN_RIGHT_MID, -1, 0);
    lv_obj_align(status_img, LV_ALIGN_RIGHT_MID, -1, 0);

    lv_obj_clear_flag(temp_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(temp_cont, 0, 0);
    lv_obj_set_style_border_width(temp_cont, 0, 0);
    lv_obj_set_style_bg_opa(temp_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(temp_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(
        temp_cont,
        LV_FLEX_ALIGN_SPACE_BETWEEN,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER);
    lv_obj_set_grid_cell(
        temp_cont,
        LV_GRID_ALIGN_STRETCH, 0, 1,
        LV_GRID_ALIGN_STRETCH, 1, 1);

    lv_obj_set_grid_cell(
        temp_chart,
        LV_GRID_ALIGN_STRETCH, 0, 1,
        LV_GRID_ALIGN_STRETCH, 2, 1);

    lv_obj_set_style_radius(temp_chart, 8, 0);
    lv_obj_set_style_border_width(temp_chart, 1, 0);
    lv_obj_set_style_border_color(temp_chart, lv_color_hex(0x25323A), 0);
    lv_obj_set_style_bg_color(temp_chart, lv_color_hex(0x11171B), 0);
    lv_obj_set_style_line_color(temp_chart, lv_color_hex(0x25323A), LV_PART_MAIN);
    lv_obj_set_style_line_width(temp_chart, 1, LV_PART_MAIN);
    lv_obj_set_style_line_width(temp_chart, 2, LV_PART_ITEMS);
    lv_obj_set_style_size(temp_chart, 0, LV_PART_INDICATOR);

    lv_chart_set_range(
        temp_chart,
        LV_CHART_AXIS_PRIMARY_Y,
        0,
        300);

    lv_chart_set_axis_tick(
        temp_chart,
        LV_CHART_AXIS_PRIMARY_Y,
        0,
        0,
        4,
        2,
        false,
        0);

    lv_chart_set_div_line_count(temp_chart, 4, 4);
    lv_chart_set_point_count(temp_chart, 5000);
    lv_chart_set_zoom_x(temp_chart, 5000);
    lv_obj_scroll_to_x(temp_chart, LV_COORD_MAX, LV_ANIM_OFF);

    (void)parent;
}

void MainPanel::create_sensors(json &temp_sensors) {
  std::lock_guard<std::mutex> lock(lv_lock);
  sensors.clear();

  for (auto &sensor : temp_sensors.items()) {
    std::string key = sensor.key();
    if (key != "extruder" && key != "heater_bed") {
      continue;
    }

    bool controllable = sensor.value()["controllable"].template get<bool>();

    const bool is_nozzle = key == "extruder";
    lv_color_t color_code =
        is_nozzle ? lv_color_hex(0x00E5FF) : lv_color_hex(0xFFFFFF);
    const char *display_name = is_nozzle ? "Nozzle" : "Bed";
    const void *sensor_img = is_nozzle ? (const void *)&extruder : (const void *)&bed;

    lv_chart_series_t *temp_series =
      lv_chart_add_series(temp_chart, color_code, LV_CHART_AXIS_PRIMARY_Y);

    auto sensor_widget = std::make_shared<SensorContainer>(
        ws, temp_cont, sensor_img, 150,
        display_name, color_code, controllable, false,
        numpad, key, temp_chart, temp_series);

    sensor_widget->use_home_layout();
    sensors.insert({key, sensor_widget});
  }
}

void MainPanel::create_fans(json &fans) {
  fan_panel.create_fans(fans);
}

void MainPanel::create_leds(json &leds) {
  led_panel.init(leds);
}

void MainPanel::enable_spoolman() {
  spoolman_panel.init();
  setting_panel.enable_spoolman();
  extruder_panel.enable_spoolman();
}
