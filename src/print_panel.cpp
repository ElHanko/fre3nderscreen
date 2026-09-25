#include "print_panel.h"

#include "state.h"
#include "utils.h"
#include "spdlog/spdlog.h"

#include <algorithm>

#define SORTED_BY_NAME 1
#define SORTED_BY_MODIFIED 2

namespace {

constexpr uint32_t COLOR_BG = 0x080B0D;
constexpr uint32_t COLOR_CARD = 0x11171B;
constexpr uint32_t COLOR_CARD_PRESSED = 0x18242A;
constexpr uint32_t COLOR_BORDER = 0x25323A;
constexpr uint32_t COLOR_ACCENT = 0x00E5FF;
constexpr uint32_t COLOR_TEXT = 0xFFFFFF;

lv_obj_t *create_text_button(lv_obj_t *parent,
                             const char *text,
                             lv_event_cb_t callback,
                             void *user_data)
{
  lv_obj_t *button = lv_btn_create(parent);
  lv_obj_clear_flag(button, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(button, lv_color_hex(COLOR_CARD), 0);
  lv_obj_set_style_border_width(button, 1, 0);
  lv_obj_set_style_border_color(button, lv_color_hex(COLOR_BORDER), 0);
  lv_obj_set_style_radius(button, 8, 0);
  lv_obj_set_style_shadow_width(button, 0, 0);
  lv_obj_set_style_bg_color(button, lv_color_hex(COLOR_CARD_PRESSED), LV_STATE_PRESSED);
  lv_obj_set_style_border_color(button, lv_color_hex(COLOR_ACCENT), LV_STATE_PRESSED);
  lv_obj_set_style_opa(button, LV_OPA_40, LV_STATE_DISABLED);
  lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, user_data);

  lv_obj_t *label = lv_label_create(button);
  lv_label_set_text(label, text);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(label);

  return button;
}

bool is_print_active(const std::string &state)
{
  return state == "printing" || state == "paused";
}

} // namespace

PrintPanel::PrintPanel(KWebSocketClient &websocket,
                       std::mutex &lock,
                       lv_obj_t *parent,
                       PrintStatusPanel &ps)
  : NotifyConsumer(lock)
  , ws(websocket)
  , cont(lv_obj_create(parent))
  , toolbar(lv_obj_create(cont))
  , refresh_btn(create_text_button(toolbar, LV_SYMBOL_REFRESH " Reload",
                                   &PrintPanel::_handle_toolbar, this))
  , modified_sort_btn(create_text_button(toolbar, "Newest",
                                         &PrintPanel::_handle_toolbar, this))
  , az_sort_btn(create_text_button(toolbar, "A-Z",
                                   &PrintPanel::_handle_toolbar, this))
  , file_table(lv_table_create(cont))
  , detail_cont(lv_obj_create(cont))
  , file_panel(detail_cont)
  , action_cont(lv_obj_create(cont))
  , status_btn(create_text_button(action_cont, "Status",
                                  &PrintPanel::_handle_status, this))
  , print_btn(create_text_button(action_cont, "Print",
                                 &PrintPanel::_handle_print, this))
  , root("", "", 0)
  , cur_dir(&root)
  , cur_file(nullptr)
  , print_status(ps)
  , sort_type(SORTED_BY_MODIFIED)
  , has_parent_row(false)
  , print_active(false)
{
  spdlog::trace("building Fre3nder files panel");

  lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
  lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(cont, 8, 0);
  lv_obj_set_style_pad_row(cont, 8, 0);
  lv_obj_set_style_border_width(cont, 0, 0);
  lv_obj_set_style_bg_color(cont, lv_color_hex(COLOR_BG), 0);

  static lv_coord_t rows[] = {
    38,
    LV_GRID_FR(1),
    126,
    42,
    LV_GRID_TEMPLATE_LAST
  };
  static lv_coord_t cols[] = {
    LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  lv_obj_set_grid_dsc_array(cont, cols, rows);

  lv_obj_set_grid_cell(
      toolbar,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 0, 1);
  lv_obj_clear_flag(toolbar, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(toolbar, 0, 0);
  lv_obj_set_style_pad_column(toolbar, 6, 0);
  lv_obj_set_style_border_width(toolbar, 0, 0);
  lv_obj_set_style_bg_opa(toolbar, LV_OPA_TRANSP, 0);
  lv_obj_set_flex_flow(toolbar, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(
      toolbar,
      LV_FLEX_ALIGN_SPACE_BETWEEN,
      LV_FLEX_ALIGN_CENTER,
      LV_FLEX_ALIGN_CENTER);

  lv_obj_t *toolbar_buttons[] = {
    refresh_btn,
    modified_sort_btn,
    az_sort_btn,
  };
  for (lv_obj_t *button : toolbar_buttons) {
    lv_obj_set_height(button, LV_PCT(100));
    lv_obj_set_flex_grow(button, 1);
    lv_obj_set_style_pad_all(button, 4, 0);
  }

  lv_obj_set_grid_cell(
      file_table,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 1, 1);
  lv_table_set_col_cnt(file_table, 1);
  lv_table_set_col_width(file_table, 0, LV_PCT(100));
  lv_obj_set_scroll_dir(file_table, LV_DIR_VER);
  lv_obj_add_event_cb(file_table, &PrintPanel::_handle_file_table, LV_EVENT_VALUE_CHANGED, this);
  lv_obj_set_style_bg_color(file_table, lv_color_hex(COLOR_CARD), LV_PART_MAIN);
  lv_obj_set_style_border_width(file_table, 1, LV_PART_MAIN);
  lv_obj_set_style_border_color(file_table, lv_color_hex(COLOR_BORDER), LV_PART_MAIN);
  lv_obj_set_style_radius(file_table, 8, LV_PART_MAIN);
  lv_obj_set_style_text_font(file_table, &lv_font_montserrat_12, LV_PART_ITEMS);
  lv_obj_set_style_text_color(file_table, lv_color_hex(COLOR_TEXT), LV_PART_ITEMS);
  lv_obj_set_style_bg_color(file_table, lv_color_hex(COLOR_CARD), LV_PART_ITEMS);
  lv_obj_set_style_border_color(file_table, lv_color_hex(COLOR_BORDER), LV_PART_ITEMS);

  lv_obj_set_grid_cell(
      detail_cont,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 2, 1);
  lv_obj_clear_flag(detail_cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(detail_cont, 0, 0);
  lv_obj_set_style_border_width(detail_cont, 0, 0);
  lv_obj_set_style_bg_opa(detail_cont, LV_OPA_TRANSP, 0);

  lv_obj_set_grid_cell(
      action_cont,
      LV_GRID_ALIGN_STRETCH, 0, 1,
      LV_GRID_ALIGN_STRETCH, 3, 1);
  lv_obj_clear_flag(action_cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(action_cont, 0, 0);
  lv_obj_set_style_pad_column(action_cont, 8, 0);
  lv_obj_set_style_border_width(action_cont, 0, 0);
  lv_obj_set_style_bg_opa(action_cont, LV_OPA_TRANSP, 0);
  lv_obj_set_flex_flow(action_cont, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(
      action_cont,
      LV_FLEX_ALIGN_SPACE_BETWEEN,
      LV_FLEX_ALIGN_CENTER,
      LV_FLEX_ALIGN_CENTER);

  lv_obj_t *action_buttons[] = {
    status_btn,
    print_btn,
  };
  for (lv_obj_t *button : action_buttons) {
    lv_obj_set_height(button, LV_PCT(100));
    lv_obj_set_flex_grow(button, 1);
  }

  file_panel.clear();
  update_sort_style();
  update_action_states();

  ws.register_notify_update(this);
}

PrintPanel::~PrintPanel()
{
  ws.unregister_notify_update(this);
}

void PrintPanel::consume(json &j)
{
  auto &value = j["/params/0/print_stats/state"_json_pointer];
  if (value.is_null()) {
    return;
  }

  std::lock_guard<std::mutex> lock(lv_lock);
  print_active = is_print_active(value.template get<std::string>());
  update_action_states();
}

void PrintPanel::subscribe()
{
  ws.send_jsonrpc("server.files.list", R"({"root":"gcodes"})"_json, [this](json &d) {
    std::lock_guard<std::mutex> lock(lv_lock);

    const std::string cur_path = cur_dir == nullptr ? "" : cur_dir->full_path;

    root.clear();
    cur_file = nullptr;
    row_entries.clear();

    if (d.contains("result") && d["result"].is_array()) {
      for (const auto &file : d["result"]) {
        if (!file.contains("path") || !file.contains("modified")) {
          continue;
        }
        root.add_path(
            KUtils::split(file["path"].template get<std::string>(), '/'),
            file["path"].template get<std::string>(),
            file["modified"].template get<uint32_t>());
      }
    }

    cur_dir = root.find_path(KUtils::split(cur_path, '/'));
    if (cur_dir == nullptr) {
      cur_dir = &root;
    }

    refresh_print_state();
    show_dir();
  });
}

void PrintPanel::handle_file_table(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED) {
    return;
  }

  uint16_t row = LV_TABLE_CELL_NONE;
  uint16_t col = LV_TABLE_CELL_NONE;
  lv_table_get_selected_cell(file_table, &row, &col);

  if (row == LV_TABLE_CELL_NONE ||
      col == LV_TABLE_CELL_NONE ||
      row >= row_entries.size()) {
    return;
  }

  if (has_parent_row && row == 0) {
    if (cur_dir != nullptr && cur_dir->parent != cur_dir) {
      cur_dir = cur_dir->parent;
      show_dir();
    }
    return;
  }

  Tree *entry = row_entries[row];
  if (entry == nullptr) {
    return;
  }

  if (entry->is_leaf()) {
    show_file_detail(entry);
  } else {
    cur_dir = entry;
    show_dir();
  }
}

void PrintPanel::handle_toolbar(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
    return;
  }

  lv_obj_t *button = lv_event_get_current_target(event);

  if (button == refresh_btn) {
    subscribe();
  } else if (button == modified_sort_btn) {
    sort_type = SORTED_BY_MODIFIED;
    show_dir();
  } else if (button == az_sort_btn) {
    sort_type = SORTED_BY_NAME;
    show_dir();
  }
}

void PrintPanel::handle_print(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_CLICKED ||
      cur_file == nullptr ||
      print_active) {
    return;
  }

  spdlog::debug("starting print {}", cur_file->full_path);
  json input = {{"filename", cur_file->full_path}};
  ws.send_jsonrpc("printer.print.start", input);

  print_active = true;
  update_action_states();
  print_status.foreground();
}

void PrintPanel::handle_status(lv_event_t *event)
{
  if (lv_event_get_code(event) == LV_EVENT_CLICKED && print_active) {
    print_status.foreground();
  }
}

void PrintPanel::handle_metadata(const std::string &path, json &j)
{
  if (!j.contains("result")) {
    return;
  }

  std::lock_guard<std::mutex> lock(lv_lock);

  /*
   * Metadata replies are asynchronous. Reloading the file list rebuilds
   * root.children, so a Tree* captured by the request could become dangling.
   * Resolve the reply against the currently selected path instead.
   */
  if (cur_file == nullptr || cur_file->full_path != path) {
    return;
  }

  cur_file->set_metadata(j);
  file_panel.refresh_view(cur_file->metadata, cur_file->full_path);
}

void PrintPanel::show_dir()
{
  if (cur_dir == nullptr) {
    cur_dir = &root;
  }

  cur_file = nullptr;
  file_panel.clear();
  row_entries.clear();

  std::vector<Tree *> entries;
  entries.reserve(cur_dir->children.size());

  for (auto &entry : cur_dir->children) {
    entries.push_back(&entry.second);
  }

  std::sort(entries.begin(), entries.end(), [this](const Tree *left, const Tree *right) {
    if (left->is_leaf() != right->is_leaf()) {
      return !left->is_leaf();
    }

    if (sort_type == SORTED_BY_MODIFIED) {
      return left->date_modified > right->date_modified;
    }

    return left->name < right->name;
  });

  has_parent_row = cur_dir->parent != cur_dir;

  uint32_t row = 0;
  if (has_parent_row) {
    lv_table_set_cell_value_fmt(file_table, row++, 0, LV_SYMBOL_DIRECTORY "  %s", "..");
    row_entries.push_back(nullptr);
  }

  for (Tree *entry : entries) {
    if (entry->is_leaf()) {
      lv_table_set_cell_value_fmt(
          file_table, row++, 0, LV_SYMBOL_FILE "  %s", entry->name.c_str());
    } else {
      lv_table_set_cell_value_fmt(
          file_table, row++, 0, LV_SYMBOL_DIRECTORY "  %s", entry->name.c_str());
    }
    row_entries.push_back(entry);
  }

  lv_table_set_row_cnt(file_table, row);
  lv_obj_scroll_to_y(file_table, 0, LV_ANIM_OFF);

  update_sort_style();
  update_action_states();
}

void PrintPanel::show_file_detail(Tree *file)
{
  if (file == nullptr || !file->is_leaf()) {
    return;
  }

  cur_file = file;
  update_action_states();

  if (file->contains_metadata()) {
    file_panel.refresh_view(file->metadata, file->full_path);
    return;
  }

  file_panel.show_loading(file->name);

  const std::string path = file->full_path;
  ws.send_jsonrpc(
      "server.files.metadata",
      {{"filename", path}},
      [path, this](json &data) { handle_metadata(path, data); });
}

void PrintPanel::refresh_print_state()
{
  auto &value = State::get_instance()
      ->get_data("/printer_state/print_stats/state"_json_pointer);

  if (value.is_null()) {
    print_active = false;
  } else {
    print_active = is_print_active(value.template get<std::string>());
  }

  update_action_states();
}

void PrintPanel::update_action_states()
{
  if (print_active) {
    lv_obj_clear_state(status_btn, LV_STATE_DISABLED);
  } else {
    lv_obj_add_state(status_btn, LV_STATE_DISABLED);
  }

  if (cur_file != nullptr && !print_active) {
    lv_obj_clear_state(print_btn, LV_STATE_DISABLED);
  } else {
    lv_obj_add_state(print_btn, LV_STATE_DISABLED);
  }
}

void PrintPanel::update_sort_style()
{
  lv_obj_set_style_border_color(
      modified_sort_btn,
      lv_color_hex(sort_type == SORTED_BY_MODIFIED ? COLOR_ACCENT : COLOR_BORDER),
      0);
  lv_obj_set_style_border_color(
      az_sort_btn,
      lv_color_hex(sort_type == SORTED_BY_NAME ? COLOR_ACCENT : COLOR_BORDER),
      0);
}
