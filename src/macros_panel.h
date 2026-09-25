#ifndef __MACROS_PANEL_H__
#define __MACROS_PANEL_H__

#include "websocket_client.h"
#include "macro_item.h"
#include "lvgl/lvgl.h"

#include <vector>
#include <memory>

class MacrosPanel {
 public:
  explicit MacrosPanel(KWebSocketClient &client);
  ~MacrosPanel();

  void populate();
  void foreground();
  void handle_callback(lv_event_t *event);

  static void _handle_callback(lv_event_t *event) {
    auto *panel = static_cast<MacrosPanel *>(event->user_data);
    panel->handle_callback(event);
  }

 private:
  KWebSocketClient &ws;

  lv_obj_t *cont;
  lv_obj_t *header_cont;
  lv_obj_t *back_btn;
  lv_obj_t *title_label;

  lv_obj_t *content_cont;
  lv_obj_t *controls_card;
  lv_obj_t *show_hide_switch;
  lv_obj_t *top_cont;
  lv_obj_t *kb;

  std::vector<std::shared_ptr<MacroItem>> macro_items;
};

#endif // __MACROS_PANEL_H__
