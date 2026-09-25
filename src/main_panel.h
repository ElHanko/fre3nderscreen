#ifndef __MAIN_PANEL_H__
#define __MAIN_PANEL_H__

#include "websocket_client.h"
#include "notify_consumer.h"
#include "sensor_container.h"
#include "prompt_panel.h"
#include "numpad.h"
#include "homing_panel.h"
#include "extruder_panel.h"
#include "fan_panel.h"
#include "led_panel.h"
#include "print_panel.h"
#include "macros_panel.h"
#include "console_panel.h"
#include "printertune_panel.h"
#include "setting_panel.h"
#include "print_status_panel.h"
#include "spoolman_panel.h"
#include "lvgl/lvgl.h"

#include <mutex>
#include <map>
#include <memory>

class MainPanel : public NotifyConsumer {
 public:
  MainPanel(KWebSocketClient &ws,
            std::mutex &lv_lock,
            SpoolmanPanel &sm);

  ~MainPanel();
  void consume(json &data);
  void init(json &data);
  void subscribe();
  PrinterTunePanel& get_tune_panel();
  void enable_spoolman();
  void set_header_warning(bool enabled);

  void create_panel();
  void create_sensors(json &temp_sensors);
  void create_fans(json &temp_fans);
  void create_leds(json &leds);
  void handle_homing_cb(lv_event_t *event);
  void handle_extrude_cb(lv_event_t *event);
  void handle_fanpanel_cb(lv_event_t *event);
  void handle_ledpanel_cb(lv_event_t *event);

  static void _handle_homing_cb(lv_event_t *event) {
    MainPanel *panel = (MainPanel*)event->user_data;
    panel->handle_homing_cb(event);
  };

  static void _handle_extrude_cb(lv_event_t *event) {
    MainPanel *panel = (MainPanel*)event->user_data;
    panel->handle_extrude_cb(event);
  };

  static void _handle_fanpanel_cb(lv_event_t *event) {
    MainPanel *panel = (MainPanel*)event->user_data;
    panel->handle_fanpanel_cb(event);
  };

  static void _handle_ledpanel_cb(lv_event_t *event) {
    MainPanel *panel = (MainPanel*)event->user_data;
    panel->handle_ledpanel_cb(event);
  };

  static void _handle_nav_cb(lv_event_t *event) {
    MainPanel *panel = (MainPanel*)event->user_data;
    panel->handle_nav_cb(event);
  };

  static void _handle_more_cb(lv_event_t *event) {
    MainPanel *panel = (MainPanel*)event->user_data;
    panel->handle_more_cb(event);
  };

  static void _refresh_header_status(lv_timer_t *timer) {
    MainPanel *panel = (MainPanel*)timer->user_data;
    panel->refresh_header_status();
  };

 private:
  void create_main(lv_obj_t *parent);
  void create_control();
  void create_more();
  void create_footer();
  void handle_nav_cb(lv_event_t *event);
  void handle_more_cb(lv_event_t *event);
  void set_nav_active(lv_obj_t *button);
  void refresh_header_status();

  KWebSocketClient &ws;
  HomingPanel homing_panel;
  FanPanel fan_panel;
  LedPanel led_panel;
  lv_obj_t *tabview;
  lv_obj_t *main_tab;
  lv_obj_t *control_tab;
  lv_obj_t *macros_tab;
  MacrosPanel macros_panel;
  lv_obj_t *console_tab;
  ConsolePanel console_panel;
  lv_obj_t *printertune_tab;
  lv_obj_t *setting_tab;
  SettingPanel setting_panel;
  lv_obj_t *more_tab;
  lv_obj_t *main_cont;
  PrintStatusPanel print_status_panel;
  PrintPanel print_panel;
  PrinterTunePanel printertune_panel;
  Numpad numpad;
  ExtruderPanel extruder_panel;
  PromptPanel prompt_panel;
  SpoolmanPanel &spoolman_panel;

  lv_obj_t *control_cont;
  lv_obj_t *more_cont;
  lv_obj_t *more_macros_btn;
  lv_obj_t *more_console_btn;
  lv_obj_t *more_tune_btn;

  lv_obj_t *footer_cont;
  lv_obj_t *nav_home_btn;
  lv_obj_t *nav_control_btn;
  lv_obj_t *nav_files_btn;
  lv_obj_t *nav_settings_btn;
  lv_obj_t *nav_more_btn;

  lv_obj_t *header_cont;
  lv_obj_t *brand_label;
  lv_obj_t *status_img;
  lv_obj_t *status_label;
  lv_timer_t *status_timer;
  bool warning_active;

  lv_obj_t *temp_cont;
  lv_obj_t *temp_chart;

  std::map<std::string, std::shared_ptr<SensorContainer>> sensors;

};
#endif // __MAIN_PANEL_H__
