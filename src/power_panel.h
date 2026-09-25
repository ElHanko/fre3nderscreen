#ifndef __POWER_PANEL_H__
#define __POWER_PANEL_H__

#include "lvgl/lvgl.h"
#include "websocket_client.h"

#include <map>
#include <mutex>
#include <string>

class PowerPanel {
 public:
  PowerPanel(KWebSocketClient &ws, std::mutex &lock);
  ~PowerPanel();

  void create_devices(json &j);
  void foreground();
  void handle_callback(lv_event_t *event);

  static void _handle_callback(lv_event_t *event) {
    auto *panel = static_cast<PowerPanel *>(event->user_data);
    panel->handle_callback(event);
  }

 private:
  void create_device(json &j);
  void handle_device_callback(json &j);

  KWebSocketClient &ws;
  std::mutex &lv_lock;

  lv_obj_t *cont;
  lv_obj_t *header_cont;
  lv_obj_t *back_btn;
  lv_obj_t *title_label;
  lv_obj_t *devices_cont;
  lv_obj_t *empty_label;

  std::map<std::string, lv_obj_t *> devices;
};

#endif // __POWER_PANEL_H__
