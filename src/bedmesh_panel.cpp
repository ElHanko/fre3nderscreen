#include "bedmesh_panel.h"
#include "state.h"
#include "spdlog/fmt/fmt.h"
#include "spdlog/spdlog.h"

#include <vector>
#include <cmath>
#include <algorithm>
#include <string>

static lv_color_t color_gradient(double offset);

namespace {

constexpr uint32_t COLOR_BG = 0x080B0D;
constexpr uint32_t COLOR_CARD = 0x11171B;
constexpr uint32_t COLOR_CARD_PRESSED = 0x18242A;
constexpr uint32_t COLOR_BORDER = 0x25323A;
constexpr uint32_t COLOR_ACCENT = 0x00E5FF;
constexpr uint32_t COLOR_TEXT = 0xFFFFFF;

void style_transparent(lv_obj_t *obj)
{
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
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
  lv_obj_set_style_opa(button, LV_OPA_50, LV_STATE_DISABLED);
}

lv_obj_t *create_control_button(lv_obj_t *parent,
                                const char *icon_text,
                                const char *label_text)
{
  lv_obj_t *button = lv_btn_create(parent);
  style_button(button);
  lv_obj_set_height(button, 46);
  lv_obj_set_flex_grow(button, 1);
  lv_obj_set_style_pad_all(button, 4, 0);
  lv_obj_set_style_pad_row(button, 2, 0);
  lv_obj_set_flex_flow(button, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(
      button,
      LV_FLEX_ALIGN_CENTER,
      LV_FLEX_ALIGN_CENTER,
      LV_FLEX_ALIGN_CENTER);

  lv_obj_t *icon = lv_label_create(button);
  lv_label_set_text(icon, icon_text);
  lv_obj_set_style_text_font(icon, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(icon, lv_color_hex(COLOR_ACCENT), 0);

  lv_obj_t *label = lv_label_create(button);
  lv_label_set_text(label, label_text);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(COLOR_TEXT), 0);

  return button;
}

} // namespace

BedMeshPanel::BedMeshPanel(KWebSocketClient &client,
                           std::mutex &lock)
  : NotifyConsumer(lock)
  , ws(client)
  , cont(lv_obj_create(lv_scr_act()))
  , prompt(lv_obj_create(lv_scr_act()))
  , header_cont(lv_obj_create(cont))
  , back_btn(lv_btn_create(header_cont))
  , title_label(lv_label_create(header_cont))
  , content_cont(lv_obj_create(cont))
  , mesh_table(lv_table_create(content_cont))
  , profile_cont(lv_obj_create(content_cont))
  , profile_table(lv_table_create(profile_cont))
  , profile_info(lv_table_create(profile_cont))
  , controls_cont(lv_obj_create(content_cont))
  , calibrate_btn(create_control_button(
        controls_cont, LV_SYMBOL_REFRESH, "Calibrate"))
  , clear_btn(create_control_button(
        controls_cont, LV_SYMBOL_CLOSE, "Clear"))
  , save_btn(create_control_button(
        controls_cont, LV_SYMBOL_SAVE, "Save"))
  , msgbox(lv_obj_create(prompt))
  , input(lv_textarea_create(msgbox))
  , kb(lv_keyboard_create(prompt))
{
  lv_obj_move_background(cont);
  lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
  lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(cont, 8, 0);
  lv_obj_set_style_pad_row(cont, 8, 0);
  lv_obj_set_style_border_width(cont, 0, 0);
  lv_obj_set_style_bg_color(cont, lv_color_hex(COLOR_BG), 0);

  static lv_coord_t root_rows[] = {
    42,
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  static lv_coord_t root_cols[] = {
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  lv_obj_set_grid_dsc_array(cont, root_cols, root_rows);

  lv_obj_set_grid_cell(
      header_cont,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 0, 1);
  style_transparent(header_cont);

  style_button(back_btn);
  lv_obj_set_size(back_btn, 40, 34);
  lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 0, 0);
  lv_obj_add_event_cb(
      back_btn,
      &BedMeshPanel::_handle_callback,
      LV_EVENT_CLICKED,
      this);

  lv_obj_t *back_label = lv_label_create(back_btn);
  lv_label_set_text(back_label, LV_SYMBOL_LEFT);
  lv_obj_set_style_text_font(back_label, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(back_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(back_label);

  lv_label_set_text(title_label, "Bed Mesh");
  lv_obj_set_style_text_font(title_label, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(title_label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(title_label);

  lv_obj_set_grid_cell(
      content_cont,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 1, 1);
  style_transparent(content_cont);
  lv_obj_set_style_pad_row(content_cont, 6, 0);
  lv_obj_set_flex_flow(content_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(
      content_cont,
      LV_FLEX_ALIGN_START,
      LV_FLEX_ALIGN_START,
      LV_FLEX_ALIGN_START);

  style_card(mesh_table);
  lv_obj_set_size(mesh_table, LV_PCT(100), 260);
  lv_obj_set_style_text_font(
      mesh_table,
      &lv_font_montserrat_10,
      LV_PART_ITEMS);
  lv_obj_set_style_text_color(
      mesh_table,
      lv_color_hex(COLOR_TEXT),
      LV_PART_ITEMS);
  lv_obj_set_scrollbar_mode(mesh_table, LV_SCROLLBAR_MODE_AUTO);
  lv_obj_add_flag(mesh_table, LV_OBJ_FLAG_HIDDEN);

  style_card(profile_cont);
  lv_obj_set_width(profile_cont, LV_PCT(100));
  lv_obj_set_flex_grow(profile_cont, 1);
  lv_obj_set_style_pad_all(profile_cont, 0, 0);
  lv_obj_clear_flag(profile_cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_flex_flow(profile_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(
      profile_cont,
      LV_FLEX_ALIGN_START,
      LV_FLEX_ALIGN_START,
      LV_FLEX_ALIGN_START);
  lv_obj_add_flag(profile_cont, LV_OBJ_FLAG_HIDDEN);

  const lv_coord_t content_width =
      lv_disp_get_hor_res(nullptr) - 34;

  lv_obj_set_width(profile_table, LV_PCT(100));
  lv_obj_set_height(profile_table, 112);
  lv_table_set_col_width(
      profile_table, 0, content_width * 66 / 100);
  lv_table_set_col_width(
      profile_table, 1, content_width * 17 / 100);
  lv_table_set_col_width(
      profile_table, 2, content_width * 17 / 100);
  lv_obj_set_style_text_font(
      profile_table,
      &lv_font_montserrat_12,
      LV_PART_ITEMS);
  lv_obj_set_scrollbar_mode(profile_table, LV_SCROLLBAR_MODE_AUTO);

  lv_obj_set_width(profile_info, LV_PCT(100));
  lv_obj_set_flex_grow(profile_info, 1);
  lv_table_set_col_width(
      profile_info, 0, content_width * 62 / 100);
  lv_table_set_col_width(
      profile_info, 1, content_width * 38 / 100);
  lv_obj_set_style_text_font(
      profile_info,
      &lv_font_montserrat_10,
      LV_PART_ITEMS);
  lv_obj_set_style_pad_top(
      profile_info,
      4,
      LV_PART_ITEMS | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_bottom(
      profile_info,
      4,
      LV_PART_ITEMS | LV_STATE_DEFAULT);
  lv_obj_set_scrollbar_mode(profile_info, LV_SCROLLBAR_MODE_AUTO);

  style_transparent(controls_cont);
  lv_obj_set_size(controls_cont, LV_PCT(100), 46);
  lv_obj_set_style_pad_column(controls_cont, 6, 0);
  lv_obj_set_flex_flow(controls_cont, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(
      controls_cont,
      LV_FLEX_ALIGN_START,
      LV_FLEX_ALIGN_CENTER,
      LV_FLEX_ALIGN_CENTER);

  lv_obj_add_event_cb(
      calibrate_btn,
      &BedMeshPanel::_handle_callback,
      LV_EVENT_CLICKED,
      this);
  lv_obj_add_event_cb(
      clear_btn,
      &BedMeshPanel::_handle_callback,
      LV_EVENT_CLICKED,
      this);
  lv_obj_add_event_cb(
      save_btn,
      &BedMeshPanel::_handle_callback,
      LV_EVENT_CLICKED,
      this);
  lv_obj_add_state(save_btn, LV_STATE_DISABLED);

  lv_obj_add_event_cb(
      mesh_table,
      &BedMeshPanel::_mesh_draw_cb,
      LV_EVENT_DRAW_PART_BEGIN,
      this);
  lv_obj_add_event_cb(
      profile_table,
      &BedMeshPanel::_handle_profile_action,
      LV_EVENT_VALUE_CHANGED,
      this);

  /*
   * Keep the existing save-profile prompt behavior for the later
   * dialog/popup rebrand pass.
   */
  lv_obj_set_style_pad_all(prompt, 0, 0);
  lv_obj_set_size(prompt, LV_PCT(100), LV_PCT(100));
  lv_obj_clear_flag(prompt, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(prompt, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_style_bg_opa(prompt, LV_OPA_70, 0);

  lv_textarea_set_one_line(input, true);
  lv_obj_set_width(input, LV_PCT(100));

  lv_obj_set_flex_flow(msgbox, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_flex_align(
      msgbox,
      LV_FLEX_ALIGN_SPACE_EVENLY,
      LV_FLEX_ALIGN_START,
      LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(msgbox, 25, 0);
  lv_obj_clear_flag(msgbox, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(msgbox, LV_PCT(60), LV_PCT(40));
  lv_obj_set_style_border_width(msgbox, 2, 0);
  lv_obj_set_style_bg_color(
      msgbox,
      lv_palette_darken(LV_PALETTE_GREY, 1),
      0);
  lv_obj_align(msgbox, LV_ALIGN_TOP_MID, 0, 20);

  lv_obj_t *label = lv_label_create(msgbox);
  lv_obj_set_width(label, LV_PCT(100));
  lv_label_set_text(
      label,
      "Saving the profile will restart the printer.");

  lv_obj_t *prompt_save_btn = lv_btn_create(msgbox);
  lv_obj_t *prompt_cancel_btn = lv_btn_create(msgbox);

  label = lv_label_create(prompt_save_btn);
  lv_label_set_text(label, "Save");

  label = lv_label_create(prompt_cancel_btn);
  lv_label_set_text(label, "Cancel");
  lv_obj_center(label);

  lv_obj_add_event_cb(
      prompt_save_btn,
      &BedMeshPanel::_handle_prompt_save,
      LV_EVENT_CLICKED,
      this);
  lv_obj_add_event_cb(
      prompt_cancel_btn,
      &BedMeshPanel::_handle_prompt_cancel,
      LV_EVENT_CLICKED,
      this);
  lv_obj_add_event_cb(
      input,
      &BedMeshPanel::_handle_kb_input,
      LV_EVENT_ALL,
      this);

  lv_keyboard_set_textarea(kb, input);
  lv_obj_move_background(prompt);

  ws.register_notify_update(this);
}

BedMeshPanel::~BedMeshPanel() {
  if (cont != NULL) {
    lv_obj_del(cont);
    cont = NULL;
  }

  if (prompt != NULL) {
    lv_obj_del(prompt);
    prompt = NULL;
  }
}

void BedMeshPanel::consume(json &j) {
  auto bm = j["/params/0/bed_mesh"_json_pointer];
  if (!bm.is_null()) {
    spdlog::trace("bedmesh panel consume {}",  bm["/profiles"_json_pointer].dump());
    refresh_views_with_lock(bm);
  }
}

void BedMeshPanel::refresh_views_with_lock(json &bm) {
  std::lock_guard<std::mutex> lock(lv_lock); // more grandular?
  refresh_views(bm);
}

void BedMeshPanel::refresh_views(json &bm) {  
  if (!bm.is_null()) {
    auto active_profile_j = bm["/profile_name"_json_pointer];
    size_t row_idx = 0;
    if(active_profile_j.is_null()) {
      lv_obj_add_state(save_btn, LV_STATE_DISABLED);
      return;
    }

    active_profile = active_profile_j.template get<std::string>();
    if (active_profile.length() > 0) {
      lv_obj_clear_state(save_btn, LV_STATE_DISABLED);

      refresh_profile_info(active_profile);
      lv_obj_clear_flag(mesh_table, LV_OBJ_FLAG_HIDDEN);
      
      auto mesh_json = bm["/probed_matrix"_json_pointer];
      if (mesh_json.is_null()) {
	mesh_json = State::get_instance()->get_data("/printer_state/bed_mesh/probed_matrix"_json_pointer);
      }

      mesh = mesh_json.template get<std::vector<std::vector<double>>>();

      // calculate cell width
      if (mesh.size() > 0 && mesh[0].size() > 0) {
	int available_width = std::max(
            40,
            (int)lv_disp_get_hor_res(nullptr) - 34);
        int col_width = std::max(
            4,
            available_width / (int)mesh[0].size());
	int cel_height = std::max(1, (int)(col_width / 2 - 8));

	lv_obj_set_style_pad_top(mesh_table, cel_height, LV_PART_ITEMS | LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(mesh_table, cel_height, LV_PART_ITEMS | LV_STATE_DEFAULT);

	lv_table_set_col_cnt(mesh_table, mesh[0].size());
	for (int i = 0; i < mesh[0].size(); i++) {
	  lv_table_set_col_width(mesh_table, i, col_width);
	}
      }

      for (int i = mesh.size() - 1; i >= 0; i--) {
	auto row = mesh[i];
	for (int j = 0; j < row.size(); j++) {
	  if (mesh.size() < 6 && row.size() < 6) {
	    lv_table_set_cell_value(mesh_table, row_idx, j, fmt::format("{:.2f}", row[j]).c_str());
	  } else {
	    lv_table_set_cell_value(mesh_table, row_idx, j, "");
	  }
	}
	row_idx++;
      }
      lv_table_set_row_cnt(mesh_table, row_idx);
    } else {
      // no active profile, hide mesh matrix
      lv_obj_add_flag(mesh_table, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_state(save_btn, LV_STATE_DISABLED);
    }

    // populate profiles tables
    auto profiles = bm["/profiles"_json_pointer];
    if (profiles.is_null()) {
      profiles = State::get_instance()->get_data("/printer_state/bed_mesh/profiles"_json_pointer);
    }

    spdlog::trace("active {}, profiles is {}", active_profile, profiles.dump());

    if (profiles.size() > 0) {
      lv_obj_clear_flag(profile_cont, LV_OBJ_FLAG_HIDDEN);
      
      row_idx = 0;
      for (auto &el : profiles.items()) {
	lv_table_clear_cell_ctrl(profile_table, row_idx, 0, LV_TABLE_CELL_CTRL_MERGE_RIGHT);
      
	bool is_active = el.key() == active_profile;
	if (is_active) {
	  lv_table_add_cell_ctrl(profile_table, row_idx, 0, LV_TABLE_CELL_CTRL_MERGE_RIGHT);
	  lv_table_set_cell_value(profile_table, row_idx, 1, "");
	} else {
	  lv_table_set_cell_value(profile_table, row_idx, 1, LV_SYMBOL_UPLOAD);
	}

	lv_table_set_cell_value(profile_table, row_idx, 0, el.key().c_str());
	lv_table_set_cell_value(profile_table, row_idx, 2, LV_SYMBOL_CLOSE);
	row_idx++;
      }
      lv_table_set_row_cnt(profile_table, row_idx);
    } else {
      // no profiles, hide profile table
      lv_obj_add_flag(profile_cont, LV_OBJ_FLAG_HIDDEN);      
    }
  }  
}

void BedMeshPanel::refresh_profile_info(std::string profile) {
  auto mesh_params = State::get_instance()->get_data(json::json_pointer(
			fmt::format("/printer_state/bed_mesh/profiles/{}/mesh_params", profile)));
  spdlog::trace("refreshing profile info {}, {}", profile, mesh_params.dump());

  if (!mesh_params.is_null()) {
    uint32_t rowidx = 0;
    auto &v = mesh_params["/algo"_json_pointer];
    if (!v.is_null()) {
      lv_table_set_cell_value(profile_info, rowidx, 0, "Algorithm");
      lv_table_set_cell_value(profile_info, rowidx, 1, v.template get<std::string>().c_str());
      rowidx++;
    }

    v = mesh_params["/tension"_json_pointer];
    if (!v.is_null()) {
      lv_table_set_cell_value(profile_info, rowidx, 0, "Tension");
      lv_table_set_cell_value(profile_info, rowidx, 1, fmt::format("{}", v.template get<double>()).c_str());
      rowidx++;
    }    

    std::vector<int> xvalues;
    for (auto &param : {"min_x", "max_x", "x_count", "mesh_x_pps"}) {
      v = mesh_params[json::json_pointer(fmt::format("/{}", param))];
      if (!v.is_null()) {
	xvalues.push_back(v.template get<int>());
      }
    }

    lv_table_set_cell_value(profile_info, rowidx, 0, "X (min, max, count, pps)");
    lv_table_set_cell_value(profile_info, rowidx, 1, fmt::format("{}", fmt::join(xvalues, ", ")).c_str());
    rowidx++;    

    std::vector<int> yvalues;
    for (auto &param : {"min_y", "max_y", "y_count", "mesh_y_pps"}) {
      v = mesh_params[json::json_pointer(fmt::format("/{}", param))];
      if (!v.is_null()) {
	yvalues.push_back(v.template get<int>());
      }
    }

    lv_table_set_cell_value(profile_info, rowidx, 0, "Y (min, max, count, pps)");
    lv_table_set_cell_value(profile_info, rowidx, 1, fmt::format("{}", fmt::join(xvalues, ", ")).c_str());
    rowidx++;
  }
}

void BedMeshPanel::foreground() {
  auto bm = State::get_instance()->get_data("/printer_state/bed_mesh"_json_pointer);
  spdlog::trace("bm {}", bm.dump());
  refresh_views(bm);
  
  lv_obj_move_foreground(cont);
}

void BedMeshPanel::handle_callback(lv_event_t *event) {
  lv_obj_t *btn = lv_event_get_current_target(event);
  if (btn == save_btn) {
    spdlog::trace("mesh save pressed");
    lv_obj_clear_flag(prompt, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);

    if (active_profile.length() > 0) {
      lv_textarea_set_text(input, active_profile.c_str());
    }
    
    lv_obj_move_foreground(prompt);
    
  } else if (btn == clear_btn) {
    spdlog::trace("mesh clear pressed");
    ws.gcode_script("BED_MESH_CLEAR");
    
  } else if (btn == calibrate_btn) {
    spdlog::trace("mesh calibrate pressed");
    auto v = State::get_instance()
      ->get_data("/printer_state/toolhead/homed_axes"_json_pointer);
    if (!v.is_null()) {
      if (v.template get<std::string>() == "xy") {
	ws.gcode_script("BED_MESH_CALIBRATE");
	return;
      }
    }
    ws.gcode_script("G28 X Y Z\nBED_MESH_CALIBRATE");

  } else if (btn == back_btn) {
    spdlog::trace("back button pressed");
    lv_obj_move_background(cont);
  }
}

void BedMeshPanel::handle_profile_action(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if(code == LV_EVENT_VALUE_CHANGED) {
    uint16_t row;
    uint16_t col;

    lv_table_get_selected_cell(profile_table, &row, &col);
    uint16_t row_count = lv_table_get_row_cnt(profile_table);
    if (row == LV_TABLE_CELL_NONE || col == LV_TABLE_CELL_NONE || row >= row_count) {
      return;
    }
    const char *selected = lv_table_get_cell_value(profile_table, row, col);    
    const char *profile_name = lv_table_get_cell_value(profile_table, row, 0);
    spdlog::trace("selected {}, {}, value {}", row, col, profile_name);
    if (col == 2) {
      // delete profile
      spdlog::trace("delete mesh {}", profile_name);
      ws.gcode_script(fmt::format("BED_MESH_PROFILE REMOVE=\"{}\"\nSAVE_CONFIG", profile_name));
    } else if (col == 1 && selected != NULL && strlen(selected) != 0) {
      // load profile
      spdlog::trace("selected {}, load mesh {}", strlen(selected), profile_name);
      ws.gcode_script(fmt::format("BED_MESH_PROFILE LOAD=\"{}\"", profile_name));

      // // populate profile info
      // refresh_profile_info(profile_name);
    } else if (col == 0) {
      // display mesh info and refresh bed mesh matrix
    }
      
  }
}

void BedMeshPanel::handle_prompt_save(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if(code == LV_EVENT_CLICKED) {
    const char *profile_name = lv_textarea_get_text(input);
    if (profile_name == NULL || strlen(profile_name) == 0) {
      return;
    }

    ws.gcode_script(fmt::format("BED_MESH_PROFILE SAVE=\"{}\"\nSAVE_CONFIG", profile_name));

    lv_textarea_set_text(input, "");
    lv_obj_add_flag(prompt, LV_OBJ_FLAG_HIDDEN);  
    lv_obj_move_background(prompt);
  }  
}

void BedMeshPanel::handle_prompt_cancel(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if(code == LV_EVENT_CLICKED) {
    lv_obj_add_flag(prompt, LV_OBJ_FLAG_HIDDEN);  
    lv_obj_move_background(prompt);
  }
}


void BedMeshPanel::handle_kb_input(lv_event_t *e)
{
  const lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_READY) {
    const char *profile_name = lv_textarea_get_text(input);
    if (profile_name == NULL || strlen(profile_name) == 0) {
      return;
    }

    ws.gcode_script(fmt::format("BED_MESH_PROFILE SAVE=\"{}\"\nSAVE_CONFIG", profile_name));

    lv_textarea_set_text(input, "");
    lv_obj_add_flag(prompt, LV_OBJ_FLAG_HIDDEN);  
    lv_obj_move_background(prompt);
  }
}

void BedMeshPanel::mesh_draw_cb(lv_event_t * e)
{
  lv_obj_t * obj = lv_event_get_target(e);
  lv_obj_draw_part_dsc_t * dsc = lv_event_get_draw_part_dsc(e);
  if(dsc->part == LV_PART_ITEMS && !mesh.empty()) {
    uint32_t row = dsc->id /  lv_table_get_col_cnt(obj);
    uint32_t col = dsc->id - row * lv_table_get_col_cnt(obj);

    dsc->label_dsc->align = LV_TEXT_ALIGN_CENTER;
    dsc->label_dsc->color = lv_palette_darken(LV_PALETTE_GREY, 3);
    
    // rows of the mesh is reversed
    int32_t reversed_row_idx = mesh.size() - row - 1;
    double offset = mesh[reversed_row_idx][col];
    lv_color_t color = color_gradient(offset);

    dsc->rect_dsc->bg_color = color;
    dsc->rect_dsc->bg_opa = LV_OPA_90;
  }
}

static lv_color_t color_gradient(double offset)
{
  double red_max = 0.25;
  uint32_t color = static_cast<uint32_t>(std::min(1.0, 1.0 - 1.0 / red_max * std::abs(offset)) * 255);
  if (offset > 0) {
    return lv_color_make(255, color, color);
  }

  if (offset < 0 ) {
    return lv_color_make(color, color, 255);
  }
  
  return lv_color_make(255, 255, 255);
}
