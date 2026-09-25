#ifndef __CONSOLE_PANEL_H__
#define __CONSOLE_PANEL_H__

#include "websocket_client.h"
#include "lvgl/lvgl.h"

#include <list>
#include <mutex>

class ConsolePanel {
 public:
  ConsolePanel(KWebSocketClient &ws, std::mutex &lock);
  ~ConsolePanel();

  lv_obj_t *get_container();
  void foreground();
  void handle_back(lv_event_t *e);
  void handle_kb_input(lv_event_t *e);
  void handle_select_macro(lv_event_t *e);
  void handle_macros(json &d);
  void handle_macro_response(json &d);
  void handle_send_macro(lv_event_t *e);
  void handle_clear_input(lv_event_t *e);

  static void _handle_back(lv_event_t *e) {
    auto *panel = static_cast<ConsolePanel *>(e->user_data);
    panel->handle_back(e);
  }

  static void _handle_kb_input(lv_event_t *e) {
    auto *panel = static_cast<ConsolePanel *>(e->user_data);
    panel->handle_kb_input(e);
  }

  static void _handle_select_macro(lv_event_t *e) {
    auto *panel = static_cast<ConsolePanel *>(e->user_data);
    panel->handle_select_macro(e);
  }

  static void _handle_send_macro(lv_event_t *e) {
    auto *panel = static_cast<ConsolePanel *>(e->user_data);
    panel->handle_send_macro(e);
  }

  static void _handle_clear_input(lv_event_t *e) {
    auto *panel = static_cast<ConsolePanel *>(e->user_data);
    panel->handle_clear_input(e);
  }

 private:
  KWebSocketClient &ws;
  std::mutex &lv_lock;

  lv_obj_t *console_cont;
  lv_obj_t *header_cont;
  lv_obj_t *back_btn;
  lv_obj_t *title_label;

  lv_obj_t *content_cont;
  lv_obj_t *output_label;
  lv_obj_t *output;
  lv_obj_t *history_label;
  lv_obj_t *macro_list;

  lv_obj_t *input_cont;
  lv_obj_t *input;
  lv_obj_t *clear_btn;
  lv_obj_t *send_btn;
  lv_obj_t *kb;

  std::list<std::string> all_macros;
  std::list<std::string> history;
};

#endif // __CONSOLE_PANEL_H__
