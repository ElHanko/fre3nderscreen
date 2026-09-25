#include "file_panel.h"

#include "utils.h"
#include "spdlog/fmt/fmt.h"

#include <ctime>
#include <experimental/filesystem>
#include <iomanip>
#include <sstream>

namespace fs = std::experimental::filesystem;

namespace {

constexpr uint32_t COLOR_CARD = 0x11171B;
constexpr uint32_t COLOR_BORDER = 0x25323A;
constexpr uint32_t COLOR_TEXT = 0xFFFFFF;
constexpr uint32_t COLOR_MUTED = 0xB8C0C5;

} // namespace

FilePanel::FilePanel(lv_obj_t *parent)
  : file_cont(lv_obj_create(parent))
  , thumbnail(lv_img_create(file_cont))
  , placeholder(lv_label_create(file_cont))
  , fname_label(lv_label_create(file_cont))
  , detail_label(lv_label_create(file_cont))
{
  lv_obj_set_size(file_cont, LV_PCT(100), LV_PCT(100));
  lv_obj_clear_flag(file_cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(file_cont, 8, 0);
  lv_obj_set_style_pad_row(file_cont, 4, 0);
  lv_obj_set_style_pad_column(file_cont, 8, 0);
  lv_obj_set_style_radius(file_cont, 8, 0);
  lv_obj_set_style_border_width(file_cont, 1, 0);
  lv_obj_set_style_border_color(file_cont, lv_color_hex(COLOR_BORDER), 0);
  lv_obj_set_style_bg_color(file_cont, lv_color_hex(COLOR_CARD), 0);

  static lv_coord_t rows[] = {
    22,
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  static lv_coord_t cols[] = {
    66,
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  lv_obj_set_grid_dsc_array(file_cont, cols, rows);

  lv_obj_set_grid_cell(
      fname_label,
      LV_GRID_ALIGN_STRETCH, 0, 2,
      LV_GRID_ALIGN_CENTER, 0, 1);
  lv_obj_set_width(fname_label, LV_PCT(100));
  lv_label_set_long_mode(fname_label, LV_LABEL_LONG_SCROLL);
  lv_obj_set_style_text_font(fname_label, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(fname_label, lv_color_hex(COLOR_TEXT), 0);

  lv_obj_set_grid_cell(
      thumbnail,
      LV_GRID_ALIGN_CENTER, 0, 1,
      LV_GRID_ALIGN_CENTER, 1, 1);

  lv_label_set_text(placeholder, LV_SYMBOL_FILE);
  lv_obj_set_style_text_font(placeholder, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(placeholder, lv_color_hex(COLOR_MUTED), 0);
  lv_obj_set_grid_cell(
      placeholder,
      LV_GRID_ALIGN_CENTER, 0, 1,
      LV_GRID_ALIGN_CENTER, 1, 1);

  lv_label_set_long_mode(detail_label, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_font(detail_label, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(detail_label, lv_color_hex(COLOR_MUTED), 0);
  lv_obj_set_grid_cell(
      detail_label,
      LV_GRID_ALIGN_STRETCH, 1, 1,
      LV_GRID_ALIGN_CENTER, 1, 1);

  clear();
}

FilePanel::~FilePanel()
{
  // The LVGL object tree is owned by the parent Files tab.
}

void FilePanel::show_placeholder()
{
  lv_obj_add_flag(thumbnail, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(placeholder, LV_OBJ_FLAG_HIDDEN);
}

void FilePanel::clear()
{
  show_placeholder();
  lv_label_set_text(fname_label, "Select a file");
  lv_label_set_text(detail_label, "Choose a G-code file from the list.");
}

void FilePanel::show_loading(const std::string &filename)
{
  show_placeholder();
  lv_label_set_text(fname_label, filename.c_str());
  lv_label_set_text(detail_label, "Loading metadata...");
}

void FilePanel::refresh_view(json &j, const std::string &gcode_path)
{
  std::stringstream time_stream;
  auto value = j["/result/modified"_json_pointer];

  if (!value.is_null()) {
    std::time_t timestamp = value.template get<std::time_t>();
    std::tm lt = *std::localtime(&timestamp);
    time_stream << std::put_time(&lt, "%Y-%m-%d %H:%M");
  } else {
    time_stream << "unknown";
  }

  value = j["/result/estimated_time"_json_pointer];
  const int eta = value.is_null() ? -1 : value.template get<int>();

  value = j["/result/filament_weight_total"_json_pointer];
  const int filament_weight = value.is_null() ? -1 : value.template get<int>();

  value = j["/result/size"_json_pointer];
  const size_t size_bytes = value.is_null() ? 0 : value.template get<size_t>();

  const auto filename = fs::path(gcode_path).filename();
  lv_label_set_text(fname_label, filename.string().c_str());

  const std::string detail = fmt::format(
      "Time: {}\nFilament: {}\nSize: {} MB\nModified: {}",
      eta > 0 ? KUtils::eta_string(eta) : "unknown",
      filament_weight > 0
          ? fmt::format("{} g", filament_weight)
          : std::string("unknown"),
      KUtils::bytes_to_mb(size_bytes),
      time_stream.str());
  lv_label_set_text(detail_label, detail.c_str());

  auto thumb_detail = KUtils::get_thumbnail(gcode_path, j, 0.25);
  if (thumb_detail.first.empty()) {
    show_placeholder();
    return;
  }

  lv_img_set_src(thumbnail, ("A:" + thumb_detail.first).c_str());

  if (thumb_detail.second > 0) {
    const uint32_t zoom =
        static_cast<uint32_t>((64.0 / static_cast<double>(thumb_detail.second)) * 256.0);
    lv_img_set_zoom(thumbnail, zoom);
  } else {
    lv_img_set_zoom(thumbnail, 256);
  }

  lv_obj_add_flag(placeholder, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(thumbnail, LV_OBJ_FLAG_HIDDEN);
}

lv_obj_t *FilePanel::get_container()
{
  return file_cont;
}

const char *FilePanel::get_thumbnail_path()
{
  return static_cast<const char *>(lv_img_get_src(thumbnail));
}
