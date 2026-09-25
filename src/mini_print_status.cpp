#include "mini_print_status.h"

#include "spdlog/fmt/fmt.h"

namespace {

constexpr uint32_t COLOR_CARD = 0x11171B;
constexpr uint32_t COLOR_BORDER = 0x25323A;
constexpr uint32_t COLOR_ACCENT = 0x00E5FF;
constexpr uint32_t COLOR_TEXT = 0xFFFFFF;
constexpr uint32_t COLOR_MUTED = 0xB8C0C5;

} // namespace

MiniPrintStatus::MiniPrintStatus(lv_obj_t *parent,
                                 lv_event_cb_t callback,
                                 void *user_data)
  : cont(lv_obj_create(parent))
  , progress_bar(lv_arc_create(cont))
  , thumb(lv_img_create(cont))
  , status_label(lv_label_create(cont))
  , status("n/a")
  , eta("...")
{
  lv_obj_add_flag(cont, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(cont, LV_OBJ_FLAG_FLOATING);
  lv_obj_add_flag(cont, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_set_size(cont, 246, 58);
  lv_obj_align(cont, LV_ALIGN_BOTTOM_MID, 0, -6);
  lv_obj_set_style_pad_all(cont, 7, 0);
  lv_obj_set_style_pad_column(cont, 8, 0);
  lv_obj_set_style_bg_color(cont, lv_color_hex(COLOR_CARD), 0);
  lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(cont, 1, 0);
  lv_obj_set_style_border_color(cont, lv_color_hex(COLOR_BORDER), 0);
  lv_obj_set_style_radius(cont, 8, 0);
  lv_obj_set_style_shadow_width(cont, 0, 0);
  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(
      cont,
      LV_FLEX_ALIGN_START,
      LV_FLEX_ALIGN_CENTER,
      LV_FLEX_ALIGN_CENTER);

  lv_obj_add_event_cb(
      cont,
      callback,
      LV_EVENT_CLICKED,
      user_data);

  lv_arc_set_rotation(progress_bar, 270);
  lv_arc_set_bg_angles(progress_bar, 0, 360);
  lv_arc_set_range(progress_bar, 0, 100);
  lv_arc_set_value(progress_bar, 0);
  lv_obj_set_size(progress_bar, 40, 40);
  lv_obj_set_style_arc_width(progress_bar, 5, LV_PART_MAIN);
  lv_obj_set_style_arc_width(progress_bar, 5, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(
      progress_bar,
      lv_color_hex(COLOR_BORDER),
      LV_PART_MAIN);
  lv_obj_set_style_arc_color(
      progress_bar,
      lv_color_hex(COLOR_ACCENT),
      LV_PART_INDICATOR);
  lv_obj_remove_style(progress_bar, nullptr, LV_PART_KNOB);
  lv_obj_clear_flag(progress_bar, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_set_size(thumb, 34, 34);
  lv_img_set_size_mode(thumb, LV_IMG_SIZE_MODE_REAL);
  lv_obj_add_flag(thumb, LV_OBJ_FLAG_HIDDEN);

  lv_obj_set_flex_grow(status_label, 1);
  lv_label_set_long_mode(status_label, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_font(status_label, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(status_label, lv_color_hex(COLOR_TEXT), 0);

  refresh_text();
}

MiniPrintStatus::~MiniPrintStatus()
{
  if (cont != nullptr) {
    lv_obj_del(cont);
    cont = nullptr;
  }
}

void MiniPrintStatus::refresh_text()
{
  lv_label_set_text(
      status_label,
      fmt::format(
          "Status: {}\nETA: {}",
          status,
          eta).c_str());
}

void MiniPrintStatus::show()
{
  lv_obj_clear_flag(cont, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(cont);
}

void MiniPrintStatus::hide()
{
  lv_obj_add_flag(cont, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_background(cont);
}

lv_obj_t *MiniPrintStatus::get_container()
{
  return cont;
}

void MiniPrintStatus::update_eta(std::string &eta_str)
{
  eta = eta_str;
  refresh_text();
}

void MiniPrintStatus::update_status(std::string &status_str)
{
  status = status_str;
  refresh_text();
}

void MiniPrintStatus::update_progress(int progress)
{
  lv_arc_set_value(progress_bar, progress);
}

void MiniPrintStatus::update_img(const std::string &img_path,
                                 size_t width)
{
  if (width == 0) {
    return;
  }

  const uint32_t zoom =
      static_cast<uint32_t>(
          (34.0 / static_cast<double>(width)) * 256.0);

  lv_img_set_zoom(thumb, zoom);
  lv_img_set_src(thumb, img_path.c_str());
  lv_obj_clear_flag(thumb, LV_OBJ_FLAG_HIDDEN);
}

void MiniPrintStatus::reset()
{
  lv_arc_set_value(progress_bar, 0);
  lv_img_set_src(thumb, nullptr);
  lv_obj_add_flag(thumb, LV_OBJ_FLAG_HIDDEN);

  eta = "...";
  status = "n/a";
  refresh_text();
}
