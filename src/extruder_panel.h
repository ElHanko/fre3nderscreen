#ifndef __EXTRUDER_PANEL_H__
#define __EXTRUDER_PANEL_H__

#include "websocket_client.h"
#include "notify_consumer.h"
#include "spoolman_panel.h"
#include "sensor_container.h"
#include "numpad.h"
#include "lvgl/lvgl.h"

#include <string>

class ExtruderPanel : public NotifyConsumer {
 public:
  ExtruderPanel(KWebSocketClient &ws,
                std::mutex &lock,
                Numpad &numpad,
                SpoolmanPanel &spoolman);
  ~ExtruderPanel();

  void foreground();
  void enable_spoolman();
  void consume(json &data);
  void handle_callback(lv_event_t *event);
  void handle_selector_cb(lv_event_t *event);

  static void _handle_callback(lv_event_t *event) {
    auto *panel = static_cast<ExtruderPanel *>(event->user_data);
    panel->handle_callback(event);
  }

  static void _handle_selector_cb(lv_event_t *event) {
    auto *panel = static_cast<ExtruderPanel *>(event->user_data);
    panel->handle_selector_cb(event);
  }

 private:
  void set_button_enabled(lv_obj_t *button, bool enabled);
  const char *selected_value(lv_obj_t *selector, uint32_t index) const;

  KWebSocketClient &ws;
  lv_obj_t *panel_cont;
  SpoolmanPanel &spoolman_panel;
  SensorContainer extruder_temp;

  lv_obj_t *header_cont;
  lv_obj_t *back_btn;
  lv_obj_t *title_label;

  lv_obj_t *temp_selector;
  lv_obj_t *length_selector;
  lv_obj_t *speed_selector;
  uint32_t temp_idx;
  uint32_t length_idx;
  uint32_t speed_idx;

  lv_obj_t *load_btn;
  lv_obj_t *unload_btn;
  lv_obj_t *cooldown_btn;
  lv_obj_t *spoolman_btn;
  lv_obj_t *extrude_btn;
  lv_obj_t *retract_btn;

  std::string load_filament_macro;
  std::string unload_filament_macro;
  std::string cooldown_macro;
};

#endif // __EXTRUDER_PANEL_H__
