#include "tmc_tune_panel.h"
#include "state.h"
#include "spdlog/spdlog.h"
#include "spdlog/fmt/fmt.h"

#include <utility>


static std::map<std::string, int> goal_idx_map = {
  {"auto", 0 },
  {"silent", 1 },
  {"performance", 2 },
  {"autoswitch", 3 }
};

static std::map<std::string, std::pair<int16_t, int16_t>> tmc_sg_range = {
  {"tmc2130", { -64, 63 } },
  {"tmc2209", { 0, 255 } },
  {"tmc2240", { -64, 63 } },
  {"tmc2660", { -64, 63 } },
  {"tmc5160", { -64, 63 } }
};

namespace {

constexpr uint32_t COLOR_BG = 0x080B0D;
constexpr uint32_t COLOR_CARD = 0x11171B;
constexpr uint32_t COLOR_CARD_PRESSED = 0x18242A;
constexpr uint32_t COLOR_BORDER = 0x25323A;
constexpr uint32_t COLOR_ACCENT = 0x00E5FF;
constexpr uint32_t COLOR_TEXT = 0xFFFFFF;
constexpr uint32_t COLOR_MUTED = 0xB8C0C5;

void style_transparent(lv_obj_t *obj)
{
  lv_obj_set_style_pad_all(obj, 0, 0);
  lv_obj_set_style_border_width(obj, 0, 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
}

void style_card(lv_obj_t *obj)
{
  lv_obj_set_style_bg_color(obj, lv_color_hex(COLOR_CARD), 0);
  lv_obj_set_style_border_width(obj, 1, 0);
  lv_obj_set_style_border_color(obj, lv_color_hex(COLOR_BORDER), 0);
  lv_obj_set_style_radius(obj, 8, 0);
  lv_obj_set_style_shadow_width(obj, 0, 0);
}

void style_button(lv_obj_t *button)
{
  lv_obj_clear_flag(button, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(button, lv_color_hex(COLOR_CARD), 0);
  lv_obj_set_style_border_width(button, 1, 0);
  lv_obj_set_style_border_color(button, lv_color_hex(COLOR_BORDER), 0);
  lv_obj_set_style_radius(button, 8, 0);
  lv_obj_set_style_shadow_width(button, 0, 0);
  lv_obj_set_style_bg_color(
      button,
      lv_color_hex(COLOR_CARD_PRESSED),
      LV_STATE_PRESSED);
  lv_obj_set_style_border_color(
      button,
      lv_color_hex(COLOR_ACCENT),
      LV_STATE_PRESSED);
}

void style_dropdown(lv_obj_t *dropdown)
{
  lv_obj_set_style_bg_color(dropdown, lv_color_hex(COLOR_CARD_PRESSED), 0);
  lv_obj_set_style_border_width(dropdown, 1, 0);
  lv_obj_set_style_border_color(dropdown, lv_color_hex(COLOR_BORDER), 0);
  lv_obj_set_style_radius(dropdown, 6, 0);
  lv_obj_set_style_text_font(dropdown, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(dropdown, lv_color_hex(COLOR_TEXT), 0);
}

} // namespace

AutoTmcContainer::AutoTmcContainer(
    const std::list<std::string> &motors,
    const std::string &stepper_name,
    int motor_idx,
    int goal_idx,
    bool sg_configured,
    int16_t sgthrs,
    std::pair<int16_t, int16_t> sg_min_max,
    lv_obj_t *parent)
  : cont(lv_obj_create(parent))
  , name(stepper_name)
  , motors_dd(lv_dropdown_create(cont))
  , tuning_goal_dd(lv_dropdown_create(cont))
  , spinbox_cont(lv_obj_create(cont))
  , sensorless_threshold(lv_spinbox_create(spinbox_cont))
  , configured_motor_idx(motor_idx)
  , configured_goal_idx(goal_idx)
  , has_sg(sg_configured)
  , configured_sensorless_thrs(sgthrs)
  , sg_range(sg_min_max)
{
  style_card(cont);
  lv_obj_set_width(cont, LV_PCT(100));
  lv_obj_set_height(cont, has_sg ? 174 : 132);
  lv_obj_set_style_pad_all(cont, 10, 0);
  lv_obj_set_style_pad_row(cont, 8, 0);
  lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

  static lv_coord_t rows_with_sg[] = {
    28,
    38,
    38,
    46,
    LV_GRID_TEMPLATE_LAST
  };
  static lv_coord_t rows_without_sg[] = {
    28,
    38,
    38,
    LV_GRID_TEMPLATE_LAST
  };
  static lv_coord_t cols[] = {
    116,
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };

  lv_obj_set_grid_dsc_array(
      cont,
      cols,
      has_sg ? rows_with_sg : rows_without_sg);

  lv_obj_t *name_label = lv_label_create(cont);
  lv_label_set_text(name_label, name.c_str());
  lv_obj_set_style_text_font(name_label, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(name_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_set_grid_cell(
      name_label,
      LV_GRID_ALIGN_START, 0, 2,
      LV_GRID_ALIGN_CENTER, 0, 1);

  lv_dropdown_clear_options(motors_dd);
  size_t index = 0;
  lv_dropdown_add_option(motors_dd, "Not Configured", index++);
  for (auto &motor : motors) {
    lv_dropdown_add_option(
        motors_dd,
        motor.substr(motor.find(' ') + 1).c_str(),
        index++);
  }
  lv_dropdown_set_selected(motors_dd, motor_idx);
  style_dropdown(motors_dd);
  lv_obj_set_grid_cell(
      motors_dd,
      LV_GRID_ALIGN_STRETCH, 1, 1,
      LV_GRID_ALIGN_CENTER, 1, 1);

  lv_obj_t *label = lv_label_create(cont);
  lv_label_set_text(label, "Motor");
  lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(COLOR_MUTED), 0);
  lv_obj_set_grid_cell(
      label,
      LV_GRID_ALIGN_START, 0, 1,
      LV_GRID_ALIGN_CENTER, 1, 1);

  lv_dropdown_set_options(
      tuning_goal_dd,
      "auto\n"
      "silent\n"
      "performance\n"
      "autoswitch");
  lv_dropdown_set_selected(tuning_goal_dd, goal_idx);
  style_dropdown(tuning_goal_dd);
  lv_obj_set_grid_cell(
      tuning_goal_dd,
      LV_GRID_ALIGN_STRETCH, 1, 1,
      LV_GRID_ALIGN_CENTER, 2, 1);

  label = lv_label_create(cont);
  lv_label_set_text(label, "Tuning Goal");
  lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(COLOR_MUTED), 0);
  lv_obj_set_grid_cell(
      label,
      LV_GRID_ALIGN_START, 0, 1,
      LV_GRID_ALIGN_CENTER, 2, 1);

  style_transparent(spinbox_cont);
  lv_obj_clear_flag(spinbox_cont, LV_OBJ_FLAG_SCROLLABLE);

  if (!has_sg) {
    lv_obj_add_flag(spinbox_cont, LV_OBJ_FLAG_HIDDEN);
    return;
  }

  label = lv_label_create(cont);
  lv_label_set_text(label, "Sensorless");
  lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(COLOR_MUTED), 0);
  lv_obj_set_grid_cell(
      label,
      LV_GRID_ALIGN_START, 0, 1,
      LV_GRID_ALIGN_CENTER, 3, 1);

  lv_obj_set_grid_cell(
      spinbox_cont,
      LV_GRID_ALIGN_STRETCH, 1, 1,
      LV_GRID_ALIGN_STRETCH, 3, 1);

  lv_spinbox_set_range(
      sensorless_threshold,
      sg_range.first,
      sg_range.second);
  lv_spinbox_set_value(sensorless_threshold, sgthrs);
  lv_spinbox_set_step(sensorless_threshold, 5);
  lv_spinbox_set_digit_format(sensorless_threshold, 3, 0);
  lv_obj_set_width(sensorless_threshold, 72);
  lv_obj_set_style_bg_color(
      sensorless_threshold,
      lv_color_hex(COLOR_CARD_PRESSED),
      0);
  lv_obj_set_style_border_width(sensorless_threshold, 1, 0);
  lv_obj_set_style_border_color(
      sensorless_threshold,
      lv_color_hex(COLOR_BORDER),
      0);
  lv_obj_set_style_radius(sensorless_threshold, 6, 0);
  lv_obj_set_style_text_color(
      sensorless_threshold,
      lv_color_hex(COLOR_TEXT),
      0);
  lv_obj_set_style_border_width(
      sensorless_threshold,
      0,
      LV_PART_CURSOR);
  lv_obj_set_style_border_opa(
      sensorless_threshold,
      LV_OPA_0,
      LV_PART_CURSOR);
  lv_obj_set_style_bg_opa(
      sensorless_threshold,
      LV_OPA_0,
      LV_PART_CURSOR);
  lv_textarea_set_cursor_click_pos(sensorless_threshold, false);
  lv_obj_center(sensorless_threshold);

  lv_obj_t *minus_btn = lv_btn_create(spinbox_cont);
  style_button(minus_btn);
  lv_obj_set_size(minus_btn, 36, 36);
  lv_obj_align_to(
      minus_btn,
      sensorless_threshold,
      LV_ALIGN_OUT_LEFT_MID,
      -6,
      0);
  lv_obj_t *minus_label = lv_label_create(minus_btn);
  lv_label_set_text(minus_label, LV_SYMBOL_MINUS);
  lv_obj_set_style_text_color(
      minus_label,
      lv_color_hex(COLOR_ACCENT),
      0);
  lv_obj_center(minus_label);
  lv_obj_add_event_cb(
      minus_btn,
      [](lv_event_t *event) {
        const lv_event_code_t code = lv_event_get_code(event);
        if (code == LV_EVENT_SHORT_CLICKED ||
            code == LV_EVENT_LONG_PRESSED_REPEAT) {
          lv_spinbox_decrement(
              static_cast<lv_obj_t *>(event->user_data));
        }
      },
      LV_EVENT_ALL,
      sensorless_threshold);

  lv_obj_t *plus_btn = lv_btn_create(spinbox_cont);
  style_button(plus_btn);
  lv_obj_set_size(plus_btn, 36, 36);
  lv_obj_align_to(
      plus_btn,
      sensorless_threshold,
      LV_ALIGN_OUT_RIGHT_MID,
      6,
      0);
  lv_obj_t *plus_label = lv_label_create(plus_btn);
  lv_label_set_text(plus_label, LV_SYMBOL_PLUS);
  lv_obj_set_style_text_color(
      plus_label,
      lv_color_hex(COLOR_ACCENT),
      0);
  lv_obj_center(plus_label);
  lv_obj_add_event_cb(
      plus_btn,
      [](lv_event_t *event) {
        const lv_event_code_t code = lv_event_get_code(event);
        if (code == LV_EVENT_SHORT_CLICKED ||
            code == LV_EVENT_LONG_PRESSED_REPEAT) {
          lv_spinbox_increment(
              static_cast<lv_obj_t *>(event->user_data));
        }
      },
      LV_EVENT_ALL,
      sensorless_threshold);
}

AutoTmcContainer::~AutoTmcContainer() {
  if (cont != NULL) {
    lv_obj_del(cont);
    cont = NULL;
  }
}

std::string AutoTmcContainer::get_config_macro() {
  char buf[128];
  lv_dropdown_get_selected_str(motors_dd, buf, sizeof(buf));
  std::string motor = std::string(buf);
  if (motor == "Not Configured") {
    return fmt::format("_GUPPY_DELETE_CONFIG SECTION=\"autotune_tmc {}\"", name);
  }

  lv_dropdown_get_selected_str(tuning_goal_dd, buf, sizeof(buf));
  if (has_sg) {
    auto sg_value = lv_spinbox_get_value(sensorless_threshold);
    return fmt::format("_GUPPY_SAVE_CONFIG SECTION=\"autotune_tmc {}\" KEY_VALUE=\"motor:{},tuning_goal:{},{}:{}\"",
		       name, motor, buf, sg_range.first == 0 ? "sg4_thrs" : "sgt", sg_value);
  } else {
    return fmt::format("_GUPPY_SAVE_CONFIG SECTION=\"autotune_tmc {}\" KEY_VALUE=\"motor:{},tuning_goal:{}\"",
		       name, motor, buf);
  }
}

TmcTunePanel::TmcTunePanel(KWebSocketClient &client)
  : ws(client)
  , cont(lv_obj_create(lv_scr_act()))
  , header_cont(lv_obj_create(cont))
  , back_btn(lv_btn_create(header_cont))
  , title_label(lv_label_create(header_cont))
  , controls_cont(lv_obj_create(cont))
  , btns_cont(lv_obj_create(cont))
  , save_btn(lv_btn_create(btns_cont))
{
  motor_parser._delim = ":";

  lv_obj_move_background(cont);
  lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
  lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(cont, 8, 0);
  lv_obj_set_style_pad_row(cont, 6, 0);
  lv_obj_set_style_border_width(cont, 0, 0);
  lv_obj_set_style_bg_color(cont, lv_color_hex(COLOR_BG), 0);

  static lv_coord_t rows[] = {
    42,
    LV_GRID_FR(1),
    68,
    LV_GRID_TEMPLATE_LAST
  };
  static lv_coord_t cols[] = {
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  lv_obj_set_grid_dsc_array(cont, cols, rows);

  lv_obj_set_grid_cell(
      header_cont,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 0, 1);
  style_transparent(header_cont);
  lv_obj_clear_flag(header_cont, LV_OBJ_FLAG_SCROLLABLE);

  style_button(back_btn);
  lv_obj_set_size(back_btn, 40, 34);
  lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 0, 0);
  lv_obj_add_event_cb(
      back_btn,
      [](lv_event_t *event) {
        auto *panel = static_cast<TmcTunePanel *>(event->user_data);
        panel->background();
      },
      LV_EVENT_CLICKED,
      this);

  lv_obj_t *back_label = lv_label_create(back_btn);
  lv_label_set_text(back_label, LV_SYMBOL_LEFT);
  lv_obj_set_style_text_font(back_label, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(back_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(back_label);

  lv_label_set_text(title_label, "TMC Tune");
  lv_obj_set_style_text_font(title_label, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(title_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(title_label);

  lv_obj_set_grid_cell(
      controls_cont,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 1, 1);
  style_transparent(controls_cont);
  lv_obj_set_style_pad_row(controls_cont, 8, 0);
  lv_obj_set_style_pad_right(controls_cont, 4, 0);
  lv_obj_set_flex_flow(controls_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_scroll_dir(controls_cont, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(controls_cont, LV_SCROLLBAR_MODE_AUTO);

  lv_obj_set_grid_cell(
      btns_cont,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 2, 1);
  style_transparent(btns_cont);
  lv_obj_clear_flag(btns_cont, LV_OBJ_FLAG_SCROLLABLE);

  style_button(save_btn);
  lv_obj_set_size(save_btn, LV_PCT(100), 58);
  lv_obj_center(save_btn);
  lv_obj_add_event_cb(
      save_btn,
      [](lv_event_t *event) {
        auto *panel = static_cast<TmcTunePanel *>(event->user_data);
        panel->save_config();
      },
      LV_EVENT_CLICKED,
      this);

  lv_obj_t *save_icon = lv_label_create(save_btn);
  lv_label_set_text(save_icon, LV_SYMBOL_SAVE);
  lv_obj_set_style_text_font(save_icon, &lv_font_montserrat_18, 0);
  lv_obj_set_style_text_color(save_icon, lv_color_hex(COLOR_ACCENT), 0);
  lv_obj_align(save_icon, LV_ALIGN_LEFT_MID, 12, 0);

  lv_obj_t *save_label = lv_label_create(save_btn);
  lv_label_set_text(save_label, "Save & Restart");
  lv_obj_set_style_text_font(save_label, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(save_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(save_label);
}

TmcTunePanel::~TmcTunePanel() {
  if (cont != NULL) {
    lv_obj_del(cont);
    cont = NULL;
  }
}

void TmcTunePanel::init(json &j, fs::path &kp) {
  if (motor_parser.LoadFromFile(kp.string().c_str()) != 0) {
    spdlog::error("Failed to load motor_databse.cfg for TMC AUTOTUNE");
    return;
  }

  motor_index.clear();
  steppers.clear();

  // 0 reserved for no configuration
  auto motors = motor_parser.GetSections();
  int motor_idx = 1;
  for (auto &s : motors) {
    motor_index.insert({s.substr(s.find(' ') + 1), motor_idx++});
  }

  State *s = State::get_instance();
  auto v = s->get_data("/printer_state/configfile/config"_json_pointer);

  std::map<std::string, json> tuned_config;
  std::vector<std::string> configured_steppers;

  for (auto &el : v.items()) {
    if (el.key().rfind("autotune_tmc ", 0) == 0) {
      // existing auto tune tmc configs      
      std::string stepper_name = el.key().substr(el.key().find(' ') + 1);
      tuned_config.insert({stepper_name, el.value()});

      spdlog::debug("found tuned stepper {}, {}", stepper_name, el.value().dump());
    }
    
    if (el.key().rfind("tmc", 0) == 0 && el.key().find(' ') != std::string::npos) {
      spdlog::debug("found configured tmc {}", el.key());      
      configured_steppers.push_back(el.key());
    }
  }

  for (auto &s : configured_steppers) {
    std::string tmctype = s.substr(0, s.find(' '));
    std::string stepper_name = s.substr(s.find(' ') + 1);
    spdlog::debug("tmctype {}, stepper {}, s {}", tmctype, stepper_name, s);
    
    auto endstop_pin = v[json::json_pointer(fmt::format("/{}/endstop_pin", stepper_name))];
    auto sg_range_el = tmc_sg_range.find(tmctype);
    bool has_virtual_endstop = !endstop_pin.is_null()
      && endstop_pin.template get<std::string>().find("virtual_endstop") != std::string::npos
      && sg_range_el != tmc_sg_range.end();

    spdlog::debug("has_virtual_endstop {}, {}, {}", !endstop_pin.is_null(),
		  !endstop_pin.is_null() ? endstop_pin.template get<std::string>().find("virtual_endstop") != std::string::npos : false,
		  !endstop_pin.is_null() ? endstop_pin.template get<std::string>(): "null");
    
    auto sg_range = sg_range_el != tmc_sg_range.end()
      ? sg_range_el->second
      : std::make_pair<int16_t, int16_t>(0, 0);

    const auto &el = tuned_config.find(stepper_name);
    if (el != tuned_config.end()) {
      auto motor = el->second["/motor"_json_pointer];
      auto goal = el->second["/tuning_goal"_json_pointer];
      auto sgthrs = el->second["/sg4_thrs"_json_pointer];
      
      spdlog::debug("found tuned stepper {}, {}, {}",
		    motor.is_null() ? "NULL" : motor.template get<std::string>(),
		    goal.is_null() ? "auto" : goal.template get<std::string>(),
		    sgthrs.is_null() ? 10 : std::stoi(sgthrs.template get<std::string>())
		    );

      int midx = 0;
      if (!motor.is_null()) {
	const auto &el = motor_index.find(motor.template get<std::string>());
	if (el != motor_index.end()) {
	  midx = el->second;
	}
      }

      int goal_idx = 0;
      if (!goal.is_null()) {
	const auto &el = goal_idx_map.find(goal.template get<std::string>());
	if (el != goal_idx_map.end()) {
	  goal_idx = el->second;
	}
      }

      int16_t sg_value = !sgthrs.is_null()
	? std::stoi(sgthrs.template get<std::string>())
	: (sg_range.first == 0 ? 80 : 1); // sg4 default 80, sgt default 1

      steppers.push_back(std::make_shared<AutoTmcContainer>(motors, el->first, midx, goal_idx,
							    has_virtual_endstop, sg_value, sg_range,
							    controls_cont));
    } else {
      spdlog::debug("did not find tuned config for stepper {}", s);
      int16_t sg_value = sg_range.first == 0 ? 80 : 1; // sg4 default 80, sgt default 1
      steppers.push_back(std::make_shared<AutoTmcContainer>(motors, stepper_name, 0, 0,
							    has_virtual_endstop, sg_value, sg_range,
							    controls_cont));
    }
  }
}

void TmcTunePanel::foreground() {
  lv_obj_move_foreground(cont);
}

void TmcTunePanel::background() {
  lv_obj_move_background(cont);
}

void TmcTunePanel::save_config() {
  std::vector<std::string> save_configs;
  for (auto &s : steppers) {
    save_configs.push_back(s->get_config_macro());
  }

  ws.gcode_script(fmt::format("{}\nSAVE_CONFIG", fmt::join(save_configs, "\n")));
}
