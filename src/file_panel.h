#ifndef __FILE_PANEL_H__
#define __FILE_PANEL_H__

#include "lvgl/lvgl.h"
#include "hv/json.hpp"

#include <string>

using json = nlohmann::json;

class FilePanel {
 public:
  explicit FilePanel(lv_obj_t *parent);
  ~FilePanel();

  void clear();
  void show_loading(const std::string &filename);
  void refresh_view(json &data, const std::string &gcode_path);
  lv_obj_t *get_container();

 private:
  void show_placeholder();

  lv_obj_t *file_cont;
  lv_obj_t *thumbnail;
  lv_obj_t *placeholder;
  lv_obj_t *fname_label;
  lv_obj_t *detail_label;
};

#endif // __FILE_PANEL_H__
