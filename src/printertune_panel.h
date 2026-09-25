#ifndef __PRINTERTUNE_PANEL_H__
#define __PRINTERTUNE_PANEL_H__

#include "finetune_panel.h"
#include "limits_panel.h"
#include "bedmesh_panel.h"
#include "inputshaper_panel.h"
#include "belts_calibration_panel.h"
#include "tmc_tune_panel.h"
#include "tmc_status_panel.h"
#include "power_panel.h"
#include "lvgl/lvgl.h"

#include <mutex>

class PrinterTunePanel {
 public:
  PrinterTunePanel(KWebSocketClient &client,
                   std::mutex &lock,
                   FineTunePanel &finetune);
  ~PrinterTunePanel();

  lv_obj_t *get_container();
  BedMeshPanel &get_bedmesh_panel();
  PowerPanel &get_power_panel();

  void foreground();
  void init(json &j);
  void handle_callback(lv_event_t *event);

  static void _handle_callback(lv_event_t *event) {
    auto *panel = static_cast<PrinterTunePanel *>(event->user_data);
    panel->handle_callback(event);
  }

 private:
  lv_obj_t *cont;
  lv_obj_t *header_cont;
  lv_obj_t *back_btn;
  lv_obj_t *title_label;
  lv_obj_t *cards_cont;

  BedMeshPanel bedmesh_panel;
  FineTunePanel &finetune_panel;
  LimitsPanel limits_panel;
  InputShaperPanel inputshaper_panel;
  BeltsCalibrationPanel belts_calibration_panel;
  TmcTunePanel tmc_tune_panel;
  TmcStatusPanel tmc_status_panel;
  PowerPanel power_panel;

  lv_obj_t *bedmesh_btn;
  lv_obj_t *finetune_btn;
  lv_obj_t *inputshaper_btn;
  lv_obj_t *belts_calibration_btn;
  lv_obj_t *limits_btn;
  lv_obj_t *tmc_tune_btn;
  lv_obj_t *tmc_status_btn;
  lv_obj_t *power_devices_btn;
};

#endif // __PRINTERTUNE_PANEL_H__
