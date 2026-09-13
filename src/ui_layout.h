#ifndef __UI_LAYOUT_H__
#define __UI_LAYOUT_H__

#include "lvgl/lvgl.h"

namespace UiLayout {

inline bool compact_portrait()
{
  const lv_coord_t width = lv_disp_get_hor_res(NULL);
  const lv_coord_t height = lv_disp_get_ver_res(NULL);

  return width <= 320 && height > width;
}

inline lv_coord_t tab_bar_width()
{
  return compact_portrait() ? 48 : 60;
}

} // namespace UiLayout

#endif // __UI_LAYOUT_H__
