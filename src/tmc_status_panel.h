#ifndef __TMC_STATUS_PANEL_H__
#define __TMC_STATUS_PANEL_H__

#include "websocket_client.h"
#include "notify_consumer.h"
#include "tmc_status_container.h"
#include "lvgl/lvgl.h"

#include <map>
#include <memory>

class TmcStatusPanel : public NotifyConsumer {
 public:
  TmcStatusPanel(KWebSocketClient &ws,
                 std::mutex &lv_lock);
  ~TmcStatusPanel();

  void foreground();
  void background();

  void init(json &j);
  void consume(json &j);

 private:
  KWebSocketClient &ws;
  lv_obj_t *cont;
  lv_obj_t *header_cont;
  lv_obj_t *back_btn;
  lv_obj_t *title_label;
  lv_obj_t *top;
  lv_obj_t *toggle;
  lv_obj_t *metrics_cont;
  lv_obj_t *empty_label;
  std::map<std::string, std::shared_ptr<TmcStatusContainer>> metrics;
};

#endif // __TMC_STATUS_PANEL_H__
