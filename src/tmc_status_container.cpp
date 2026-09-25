#include "tmc_status_container.h"
#include "spdlog/spdlog.h"
#include "spdlog/fmt/fmt.h"

namespace {

constexpr uint32_t COLOR_CARD = 0x11171B;
constexpr uint32_t COLOR_BORDER = 0x25323A;
constexpr uint32_t COLOR_TEXT = 0xFFFFFF;
constexpr uint32_t COLOR_MUTED = 0xB8C0C5;

} // namespace

TmcStatusContainer::TmcStatusContainer(
    KWebSocketClient &client,
    lv_obj_t *parent,
    const std::string &stepper_name)
  : ws(client)
  , cont(lv_obj_create(parent))
  , chart_cont(lv_obj_create(cont))
  , label(lv_label_create(cont))
  , legend(lv_obj_create(chart_cont))
  , chart(lv_chart_create(chart_cont))
  , sg_series(lv_chart_add_series(
        chart,
        lv_palette_main(LV_PALETTE_ORANGE),
        LV_CHART_AXIS_PRIMARY_Y))
  , irms_series(lv_chart_add_series(
        chart,
        lv_palette_main(LV_PALETTE_RED),
        LV_CHART_AXIS_PRIMARY_Y))
  , semin_series(lv_chart_add_series(
        chart,
        lv_palette_main(LV_PALETTE_BLUE),
        LV_CHART_AXIS_PRIMARY_Y))
  , semax_series(lv_chart_add_series(
        chart,
        lv_palette_main(LV_PALETTE_GREEN),
        LV_CHART_AXIS_PRIMARY_Y))
  , stepper_config(lv_obj_create(cont))
  , semin_sb(
        stepper_config,
        "semin",
        0,
        15,
        0,
        [this, stepper_name](int value) {
          auto name =
              stepper_name.substr(stepper_name.find(' ') + 1);
          update_tmc_value(name, "semin", value);
        })
  , semax_sb(
        stepper_config,
        "semax",
        0,
        15,
        0,
        [this, stepper_name](int value) {
          auto name =
              stepper_name.substr(stepper_name.find(' ') + 1);
          update_tmc_value(name, "semax", value);
        })
  , seup_sb(
        stepper_config,
        "seup",
        0,
        3,
        0,
        [this, stepper_name](int value) {
          auto name =
              stepper_name.substr(stepper_name.find(' ') + 1);
          update_tmc_value(name, "seup", value);
        })
  , sedn_sb(
        stepper_config,
        "sedn",
        0,
        3,
        0,
        [this, stepper_name](int value) {
          auto name =
              stepper_name.substr(stepper_name.find(' ') + 1);
          update_tmc_value(name, "sedn", value);
        })
  , toff_sb(
        stepper_config,
        "toff",
        0,
        15,
        0,
        [this, stepper_name](int value) {
          auto name =
              stepper_name.substr(stepper_name.find(' ') + 1);
          update_tmc_value(name, "toff", value);
        })
  , tbl_sb(
        stepper_config,
        "tbl",
        0,
        3,
        0,
        [this, stepper_name](int value) {
          auto name =
              stepper_name.substr(stepper_name.find(' ') + 1);
          update_tmc_value(name, "tbl", value);
        })
  , hstrt_sb(
        stepper_config,
        "hstrt",
        0,
        7,
        0,
        [this, stepper_name](int value) {
          auto name =
              stepper_name.substr(stepper_name.find(' ') + 1);
          update_tmc_value(name, "hstrt", value);
        })
  , hend_sb(
        stepper_config,
        "hend",
        0,
        15,
        0,
        [this, stepper_name](int value) {
          auto name =
              stepper_name.substr(stepper_name.find(' ') + 1);
          update_tmc_value(name, "hend", value);
        })
{
  lv_obj_set_width(cont, LV_PCT(100));
  lv_obj_set_height(cont, LV_SIZE_CONTENT);
  lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(cont, lv_color_hex(COLOR_CARD), 0);
  lv_obj_set_style_border_width(cont, 1, 0);
  lv_obj_set_style_border_color(cont, lv_color_hex(COLOR_BORDER), 0);
  lv_obj_set_style_radius(cont, 8, 0);
  lv_obj_set_style_shadow_width(cont, 0, 0);
  lv_obj_set_style_pad_all(cont, 10, 0);
  lv_obj_set_style_pad_row(cont, 8, 0);
  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);

  lv_obj_set_width(label, LV_PCT(100));
  lv_label_set_text(
      label,
      fmt::format("{} (current/load)", stepper_name).c_str());
  lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(COLOR_TEXT), 0);

  lv_obj_set_width(chart_cont, LV_PCT(100));
  lv_obj_set_height(chart_cont, 210);
  lv_obj_clear_flag(chart_cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(chart_cont, 6, 0);
  lv_obj_set_style_border_width(chart_cont, 0, 0);
  lv_obj_set_style_bg_opa(chart_cont, LV_OPA_TRANSP, 0);
  lv_obj_set_flex_flow(chart_cont, LV_FLEX_FLOW_COLUMN);

  lv_obj_set_width(legend, LV_PCT(100));
  lv_obj_set_height(legend, LV_SIZE_CONTENT);
  lv_obj_clear_flag(legend, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(legend, 0, 0);
  lv_obj_set_style_pad_column(legend, 8, 0);
  lv_obj_set_style_pad_row(legend, 2, 0);
  lv_obj_set_style_border_width(legend, 0, 0);
  lv_obj_set_style_bg_opa(legend, LV_OPA_TRANSP, 0);
  lv_obj_set_flex_flow(legend, LV_FLEX_FLOW_ROW_WRAP);

  lv_obj_t *axis_label = lv_label_create(legend);
  lv_label_set_text(axis_label, "sg_result");
  lv_obj_set_style_text_font(axis_label, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(
      axis_label,
      lv_palette_main(LV_PALETTE_ORANGE),
      0);

  axis_label = lv_label_create(legend);
  lv_label_set_text(axis_label, "i_rms");
  lv_obj_set_style_text_font(axis_label, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(
      axis_label,
      lv_palette_main(LV_PALETTE_RED),
      0);

  axis_label = lv_label_create(legend);
  lv_label_set_text(axis_label, "semin");
  lv_obj_set_style_text_font(axis_label, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(
      axis_label,
      lv_palette_main(LV_PALETTE_BLUE),
      0);

  axis_label = lv_label_create(legend);
  lv_label_set_text(axis_label, "semin + semax");
  lv_obj_set_style_text_font(axis_label, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(
      axis_label,
      lv_palette_main(LV_PALETTE_GREEN),
      0);

  lv_obj_set_width(chart, LV_PCT(100));
  lv_obj_set_flex_grow(chart, 1);
  lv_obj_set_style_size(chart, 0, LV_PART_INDICATOR);
  lv_obj_set_style_text_color(chart, lv_color_hex(COLOR_MUTED), 0);

  lv_chart_set_range(
      chart,
      LV_CHART_AXIS_PRIMARY_Y,
      0,
      1600);
  lv_chart_set_axis_tick(
      chart,
      LV_CHART_AXIS_PRIMARY_Y,
      0,
      0,
      6,
      5,
      true,
      36);
  lv_chart_set_div_line_count(chart, 3, 8);
  lv_chart_set_point_count(chart, 5000);
  lv_chart_set_zoom_x(chart, 5000);
  lv_obj_scroll_to_x(chart, LV_COORD_MAX, LV_ANIM_OFF);

  lv_obj_set_width(stepper_config, LV_PCT(100));
  lv_obj_set_height(stepper_config, LV_SIZE_CONTENT);
  lv_obj_clear_flag(stepper_config, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(stepper_config, 0, 0);
  lv_obj_set_style_pad_row(stepper_config, 4, 0);
  lv_obj_set_style_border_width(stepper_config, 0, 0);
  lv_obj_set_style_bg_opa(stepper_config, LV_OPA_TRANSP, 0);
  lv_obj_set_flex_flow(stepper_config, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(
      stepper_config,
      LV_FLEX_ALIGN_START,
      LV_FLEX_ALIGN_CENTER,
      LV_FLEX_ALIGN_CENTER);
}

TmcStatusContainer::~TmcStatusContainer()
{
  if (cont != nullptr) {
    lv_obj_del(cont);
    cont = nullptr;
  }
}

void TmcStatusContainer::update(json &stepper)
{
  if (stepper.is_null()) {
    return;
  }

  auto value = stepper["/i_rms"_json_pointer];
  if (!value.is_null()) {
    lv_chart_set_next_value(
        chart,
        irms_series,
        value.template get<double>());
  }

  value = stepper["/semin"_json_pointer];
  int semin = 0;
  if (!value.is_null()) {
    semin = value.template get<int>();
    lv_chart_set_next_value(
        chart,
        semin_series,
        semin * 32);
    semin_sb.update_value(semin);
  }

  value = stepper["/semax"_json_pointer];
  if (!value.is_null()) {
    auto semax = value.template get<int>();
    lv_chart_set_next_value(
        chart,
        semax_series,
        (semin + semax + 1) * 32);
    semax_sb.update_value(semax);
  }

  value = stepper["/seup"_json_pointer];
  if (!value.is_null()) {
    seup_sb.update_value(value.template get<int>());
  }

  value = stepper["/sedn"_json_pointer];
  if (!value.is_null()) {
    sedn_sb.update_value(value.template get<int>());
  }

  value = stepper["/toff"_json_pointer];
  if (!value.is_null()) {
    toff_sb.update_value(value.template get<int>());
  }

  value = stepper["/tbl"_json_pointer];
  if (!value.is_null()) {
    tbl_sb.update_value(value.template get<int>());
  }

  value = stepper["/hstrt"_json_pointer];
  if (!value.is_null()) {
    hstrt_sb.update_value(value.template get<int>());
  }

  value = stepper["/hend"_json_pointer];
  if (!value.is_null()) {
    hend_sb.update_value(value.template get<int>());
  }

  value = stepper["/sg_result"_json_pointer];
  if (!value.is_null()) {
    lv_chart_set_next_value(
        chart,
        sg_series,
        value.template get<int>());
  }
}

void TmcStatusContainer::update_tmc_value(
    const std::string &stepper_name,
    const std::string &field_name,
    int value)
{
  ws.gcode_script(fmt::format(
      "SET_TMC_FIELD field={} value={} stepper={}",
      field_name,
      value,
      stepper_name));
}
