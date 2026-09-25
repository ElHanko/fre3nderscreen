#ifndef __PRINT_STATUS_PANEL_H__
#define __PRINT_STATUS_PANEL_H__

#include "websocket_client.h"
#include "notify_consumer.h"
#include "finetune_panel.h"
#include "mini_print_status.h"
#include "lvgl/lvgl.h"

#include <ctime>
#include <map>
#include <mutex>
#include <string>

class PrintStatusPanel : public NotifyConsumer {
 public:
  PrintStatusPanel(KWebSocketClient &ws,
                   std::mutex &lock,
                   lv_obj_t *mini_parent);
  ~PrintStatusPanel();

  void init(json &fans);
  void reset();
  void populate();
  void foreground();
  void background();

  void handle_metadata(const std::string &gcode_file, json &j);
  void handle_callback(lv_event_t *event);

  static void _handle_callback(lv_event_t *event) {
    auto *panel = static_cast<PrintStatusPanel *>(event->user_data);
    panel->handle_callback(event);
  }

  void consume(json &j);
  void update_time_progress(uint32_t time_passed);
  void update_flow_rate(double filament_used);
  void update_layers(json &info);
  int max_layer(json &info);
  int current_layer(json &info);

  FineTunePanel &get_finetune_panel();

 private:
  enum class ConfirmAction {
    None,
    CancelPrint,
    EmergencyStop,
  };

  void set_button_enabled(lv_obj_t *button, bool enabled);
  void request_confirmation(ConfirmAction action,
                            const char *message);
  void execute_confirmation();
  void close_confirmation();

  KWebSocketClient &ws;
  FineTunePanel finetune_panel;
  MiniPrintStatus mini_print_status;

  lv_obj_t *status_cont;
  lv_obj_t *header_cont;
  lv_obj_t *back_btn;
  lv_obj_t *title_label;

  lv_obj_t *thumbnail_cont;
  lv_obj_t *thumbnail;
  lv_obj_t *pbar_cont;
  lv_obj_t *filename_label;
  lv_obj_t *progress_bar;
  lv_obj_t *progress_label;

  lv_obj_t *detail_cont;
  lv_obj_t *extruder_temp;
  lv_obj_t *bed_temp;
  lv_obj_t *print_speed;
  lv_obj_t *z_offset;
  lv_obj_t *flow_rate;
  lv_obj_t *layers;
  lv_obj_t *fan0;
  lv_obj_t *elapsed;
  lv_obj_t *time_left;

  lv_obj_t *buttons_cont;
  lv_obj_t *finetune_btn;
  lv_obj_t *pause_btn;
  lv_obj_t *resume_btn;
  lv_obj_t *cancel_btn;
  lv_obj_t *emergency_btn;

  lv_obj_t *confirm_overlay;
  lv_obj_t *confirm_dialog;
  lv_obj_t *confirm_message;
  lv_obj_t *confirm_yes_btn;
  lv_obj_t *confirm_no_btn;
  ConfirmAction pending_confirmation;

  uint32_t estimated_time_s;

  std::time_t flow_ts;
  double last_filament_used;
  double filament_diameter;
  double flow;
  int extruder_target;
  int heater_bed_target;
  json current_file;

  std::map<std::string, int> fan_speeds;
};

#endif // __PRINT_STATUS_PANEL_H__
