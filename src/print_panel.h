#ifndef __PRINT_PANEL_H__
#define __PRINT_PANEL_H__

#include "lvgl/lvgl.h"
#include "websocket_client.h"
#include "notify_consumer.h"
#include "file_panel.h"
#include "print_status_panel.h"
#include "tree.h"

#include <string>
#include <vector>

class PrintPanel : public NotifyConsumer {
 public:
  PrintPanel(KWebSocketClient &ws,
             std::mutex &lv_lock,
             lv_obj_t *parent,
             PrintStatusPanel &ps);
  ~PrintPanel();

  void consume(json &data);
  void subscribe();
  void handle_file_table(lv_event_t *event);
  void handle_toolbar(lv_event_t *event);
  void handle_print(lv_event_t *event);
  void handle_status(lv_event_t *event);
  void handle_metadata(const std::string &path, json &data);

  static void _handle_file_table(lv_event_t *event) {
    auto *panel = static_cast<PrintPanel *>(event->user_data);
    panel->handle_file_table(event);
  }

  static void _handle_toolbar(lv_event_t *event) {
    auto *panel = static_cast<PrintPanel *>(event->user_data);
    panel->handle_toolbar(event);
  }

  static void _handle_print(lv_event_t *event) {
    auto *panel = static_cast<PrintPanel *>(event->user_data);
    panel->handle_print(event);
  }

  static void _handle_status(lv_event_t *event) {
    auto *panel = static_cast<PrintPanel *>(event->user_data);
    panel->handle_status(event);
  }

 private:
  void show_dir();
  void show_file_detail(Tree *file);
  void refresh_print_state();
  void update_action_states();
  void update_sort_style();

  KWebSocketClient &ws;

  lv_obj_t *cont;
  lv_obj_t *toolbar;
  lv_obj_t *refresh_btn;
  lv_obj_t *modified_sort_btn;
  lv_obj_t *az_sort_btn;
  lv_obj_t *file_table;
  lv_obj_t *detail_cont;
  FilePanel file_panel;
  lv_obj_t *action_cont;
  lv_obj_t *status_btn;
  lv_obj_t *print_btn;

  Tree root;
  Tree *cur_dir;
  Tree *cur_file;
  std::vector<Tree *> row_entries;
  PrintStatusPanel &print_status;
  uint32_t sort_type;
  bool has_parent_row;
  bool print_active;
};

#endif // __PRINT_PANEL_H__
