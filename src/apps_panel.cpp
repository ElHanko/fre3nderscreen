#include "apps_panel.h"

#include <algorithm>
#include <cstdlib>
#include <stdexcept>
#include <utility>
#include <dirent.h>
#include <fcntl.h>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace {
using nlohmann::json;
constexpr uint32_t BG = 0x080B0D;
constexpr uint32_t CARD = 0x11171B;
constexpr uint32_t BORDER = 0x25323A;
constexpr uint32_t CYAN = 0x00E5FF;
constexpr uint32_t RED = 0xFF3B30;
constexpr uint32_t WHITE = 0xFFFFFF;

json field(const json &obj, const char *name)
{
  if (!obj.is_object() || !obj.contains(name)) return nullptr;
  return obj.at(name);
}

std::string str(const json &obj, const char *name, const std::string &fallback = "Unavailable")
{
  const json item = field(obj, name);
  if (item.is_string()) return item.get<std::string>();
  if (item.is_boolean()) return item.get<bool>() ? "Yes" : "No";
  return fallback;
}

bool flag(const json &obj, const char *name)
{
  return field(obj, name) == true;
}

bool app_name(const std::string &value)
{
  if (value.empty() || value.size() > 64 || value[0] < 'a' || value[0] > 'z') return false;
  return std::all_of(value.begin(), value.end(), [](char c) {
    return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_';
  });
}

bool package_id(const std::string &id)
{
  return id.size() == 32 && std::all_of(id.begin(), id.end(), [](char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
  });
}

std::string error_message(const ManagementClient::Response &answer)
{
  try {
    auto obj = json::parse(answer.body);
    const json err = field(obj, "error");
    const json message = field(err, "message");
    if (message.is_string()) return message.get<std::string>();
  } catch (...) {}
  return answer.error.empty() ? "Invalid Management response" : answer.error;
}

std::string usb_root()
{
#ifdef SIMULATOR
  const char *path = std::getenv("FRE3NDERSCREEN_USB_ROOT");
  if (path && *path) return path;
#endif
  return "/run/fre3nder/usb";
}

bool safe_filename(const std::string &name)
{
  if (name.size() < 9 || name.size() > 128 ||
      name.compare(name.size() - 8, 8, ".fre3app") != 0) return false;
  for (char c : name)
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
          (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.')) return false;
  return true;
}

lv_obj_t *label(lv_obj_t *parent, const std::string &text, uint32_t color = WHITE)
{
  lv_obj_t *item = lv_label_create(parent);
  lv_obj_set_width(item, LV_PCT(100));
  lv_label_set_long_mode(item, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_color(item, lv_color_hex(color), 0);
  lv_obj_set_style_text_font(item, &lv_font_montserrat_12, 0);
  lv_label_set_text(item, text.c_str());
  return item;
}
}

AppsPanel::AppsPanel()
  : cont(lv_obj_create(lv_scr_act()))
  , back_btn(lv_btn_create(cont))
  , refresh_btn(lv_btn_create(cont))
  , content(lv_obj_create(cont))
  , message(nullptr)
  , summary(nullptr)
  , installed_list(nullptr)
  , selected_detail(nullptr)
  , staged_list(nullptr)
  , usb_list(nullptr)
  , actions(nullptr)
  , timer(nullptr)
{
  lv_obj_move_background(cont);
  lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
  lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(cont, 8, 0);
  lv_obj_set_style_bg_color(cont, lv_color_hex(BG), 0);
  lv_obj_set_style_border_width(cont, 0, 0);

  for (lv_obj_t *button : {back_btn, refresh_btn}) {
    lv_obj_set_size(button, 40, 34);
    lv_obj_set_style_bg_color(button, lv_color_hex(CARD), 0);
    lv_obj_set_style_border_width(button, 1, 0);
    lv_obj_set_style_border_color(button, lv_color_hex(BORDER), 0);
    lv_obj_set_style_radius(button, 8, 0);
    lv_obj_add_event_cb(button, event_callback, LV_EVENT_CLICKED, this);
  }
  lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 0, 3);
  lv_obj_align(refresh_btn, LV_ALIGN_TOP_RIGHT, 0, 3);
  lv_obj_t *back_icon = lv_label_create(back_btn);
  lv_label_set_text(back_icon, LV_SYMBOL_LEFT);
  lv_obj_set_style_text_color(back_icon, lv_color_hex(WHITE), 0);
  lv_obj_center(back_icon);
  lv_obj_t *refresh_icon = lv_label_create(refresh_btn);
  lv_label_set_text(refresh_icon, LV_SYMBOL_REFRESH);
  lv_obj_set_style_text_color(refresh_icon, lv_color_hex(CYAN), 0);
  lv_obj_center(refresh_icon);
  lv_obj_t *title = lv_label_create(cont);
  lv_label_set_text(title, "Apps");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(WHITE), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 7);

  lv_obj_set_size(content, LV_PCT(100), lv_disp_get_ver_res(NULL) - 58);
  lv_obj_align(content, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_pad_all(content, 9, 0);
  lv_obj_set_style_pad_row(content, 8, 0);
  lv_obj_set_style_border_width(content, 1, 0);
  lv_obj_set_style_border_color(content, lv_color_hex(BORDER), 0);
  lv_obj_set_style_bg_color(content, lv_color_hex(CARD), 0);
  lv_obj_set_style_radius(content, 8, 0);
  lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  message = label(content, "Open this page to load installed applications.");
  summary = label(content, "");
  installed_list = lv_obj_create(content);
  selected_detail = label(content, "");
  staged_list = lv_obj_create(content);
  usb_list = lv_obj_create(content);
  actions = lv_obj_create(content);
  for (lv_obj_t *list : {installed_list, staged_list, usb_list, actions}) {
    lv_obj_set_width(list, LV_PCT(100));
    lv_obj_set_height(list, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(list, 0, 0);
    lv_obj_set_style_pad_row(list, 5, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
  }
  timer = lv_timer_create(timer_callback, 120, this);
}

AppsPanel::~AppsPanel()
{
  if (timer) lv_timer_del(timer);
  if (worker.joinable()) worker.join();
  lv_obj_del(cont);
}

void AppsPanel::foreground()
{
  open = true;
  lv_obj_move_foreground(cont);
  load();
}

void AppsPanel::start(Operation kind, std::function<ManagementClient::Response()> request)
{
  if (busy) return;
  if (worker.joinable()) worker.join();
  request_kind = kind;
  busy = true;
  ready.store(false, std::memory_order_release);
  lv_label_set_text(message, kind == Operation::Status ? "Loading applications..." :
                    kind == Operation::Detail ? "Loading application details..." :
                    "Submitting request...");
  worker = std::thread([this, request]() {
    auto response = request();
    {
      std::lock_guard<std::mutex> guard(result_mutex);
      result = std::move(response);
    }
    ready.store(true, std::memory_order_release);
  });
}

void AppsPanel::load()
{
  start(Operation::Status, []() { return ManagementClient::apps_status(); });
}

void AppsPanel::detail()
{
  if (!app_name(selected_app)) return;
  const std::string name = selected_app;
  start(Operation::Detail, [name]() { return ManagementClient::app_detail(name); });
}

void AppsPanel::poll()
{
  if (ready.exchange(false, std::memory_order_acq_rel)) {
    ManagementClient::Response response;
    {
      std::lock_guard<std::mutex> guard(result_mutex);
      response = std::move(result);
    }
    if (worker.joinable()) worker.join();
    busy = false;
    if (!response.ok) {
      lv_label_set_text(message, error_message(response).c_str());
    } else {
      try {
        const json payload = json::parse(response.body);
        if (!payload.is_object() || field(payload, "api_version") != 1 ||
            field(payload, "ok") != true)
          throw std::runtime_error("Invalid Management result");
        if (request_kind == Operation::Status) {
          if (field(payload, "operation") != "apps-status" ||
              !field(payload, "apps").is_object())
            throw std::runtime_error("Invalid Apps status");
          snapshot = field(payload, "apps");
          const std::string state = str(field(snapshot, "execution"), "state", "");
          polling_execution = state == "running" || state == "uploading";
          last_refresh = lv_tick_get();
          lv_label_set_text(message, "");
          scan_usb();
          render();
        } else if (request_kind == Operation::Detail) {
          if (field(payload, "operation") != "apps-detail")
            throw std::runtime_error("Invalid app detail");
          const json app = field(payload, "app");
          if (str(app, "name") != selected_app)
            throw std::runtime_error("Unexpected app identity");
          const std::string details =
              "Selected: " + selected_app + "\nVersion: " + str(app, "version") +
              "\nPublisher: " + str(app, "publisher") +
              "\nStorage: " + str(app, "storage") +
              "\nRunning: " + str(app, "running") +
              "\nDisplay: " + str(app, "display_frontend") +
              "  Web: " + str(app, "web_frontend");
          lv_label_set_text(selected_detail, details.c_str());
          lv_label_set_text(message, "");
        } else {
          const std::string operation = str(payload, "operation");
          if (operation != "apps-accepted" && operation != "apps-discard" &&
              operation != "apps-acknowledge")
            throw std::runtime_error("Unexpected Apps operation");
          lv_label_set_text(message, "Operation accepted. Checking status...");
          load(); // Read, never resubmit, an asynchronous execution.
        }
      } catch (const std::exception &error) {
        lv_label_set_text(message, error.what());
      }
    }
  }
  if (open && !busy && polling_execution && lv_tick_elaps(last_refresh) > 1800)
    load();
}

void AppsPanel::add_button(lv_obj_t *parent, const std::string &text,
                           Action action, const std::string &argument, bool dangerous)
{
  lv_obj_t *button = lv_btn_create(parent);
  lv_obj_set_width(button, LV_PCT(100));
  lv_obj_set_height(button, 50);
  lv_obj_set_style_pad_all(button, 9, 0);
  lv_obj_set_style_bg_color(button, lv_color_hex(CARD), 0);
  lv_obj_set_style_border_width(button, 1, 0);
  lv_obj_set_style_border_color(button, lv_color_hex(dangerous ? RED : BORDER), 0);
  lv_obj_set_style_radius(button, 6, 0);
  lv_obj_add_event_cb(button, event_callback, LV_EVENT_CLICKED, this);
  lv_obj_t *title = label(button, text, dangerous ? RED : WHITE);
  lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
  if (action == Action::SelectApp && text.find('\n') != std::string::npos) {
    lv_obj_set_height(button, LV_SIZE_CONTENT);
    lv_obj_set_style_min_height(button, 50, 0);
  }
  controls.emplace(button, Control{action, argument});
}

void AppsPanel::scan_usb()
{
  usb_files.clear();
  const std::string root = usb_root();
  int fd = ::open(root.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
  if (fd < 0) return;
  DIR *directory = fdopendir(fd);
  if (!directory) { close(fd); return; }
  while (dirent *entry = readdir(directory)) {
    const std::string name = entry->d_name;
    if (!safe_filename(name)) continue;
    struct stat meta {};
    if (fstatat(fd, name.c_str(), &meta, AT_SYMLINK_NOFOLLOW) != 0 ||
        !S_ISREG(meta.st_mode) || meta.st_size <= 0 ||
        meta.st_size > 256LL * 1024 * 1024) continue;
    usb_files.push_back(name);
    if (usb_files.size() >= 32) break;
  }
  closedir(directory);
  std::sort(usb_files.begin(), usb_files.end());
  if (std::find(usb_files.begin(), usb_files.end(), selected_usb) == usb_files.end())
    selected_usb.clear();
}

void AppsPanel::render()
{
  controls.clear();
  for (lv_obj_t *part : {installed_list, staged_list, usb_list, actions}) lv_obj_clean(part);
  const json installed = field(snapshot, "installed");
  const json web = field(snapshot, "web");
  const json display = field(snapshot, "display");
  const std::string active_display = str(display, "active", "None");
  const std::string active_web = str(web, "active", "None");
  const json execution = field(snapshot, "execution");
  const json repository = field(snapshot, "repository");
  const std::string availability = str(repository, "availability", "unavailable");
  const json updates = field(repository, "updates");
  std::string summary_text = "INSTALLED APPS\nWeb: " + active_web +
                             "\nDisplay: " + active_display;
  if (availability == "checked") {
    summary_text += "\nRepository: check succeeded";
    if (updates.is_array() && updates.empty()) summary_text += "\nNo updates offered.";
  } else if (availability == "cached") {
    summary_text += "\nRepository: check failed (cached information)";
  } else if (str(repository, "configuration", "") == "unconfigured") {
    summary_text += "\nRepository: not configured. Update availability unknown.";
  } else {
    summary_text += "\nRepository: unavailable. Update availability unknown.";
  }
  const std::string last_success = str(repository, "last_success", "");
  if (!last_success.empty()) summary_text += "\nLast successful check: " + last_success;
  const std::string repository_error = str(field(repository, "error"), "message", "");
  if (!repository_error.empty()) summary_text += "\n" + repository_error;
  if (execution.is_object()) {
    const std::string state = str(execution, "state");
    summary_text += "\nLast operation: " + str(execution, "operation") + " (" + state + ")";
    const json issue = field(execution, "error");
    if (issue.is_object()) summary_text += "\n" + str(issue, "message");
  }
  lv_label_set_text(summary, summary_text.c_str());
  bool selected_exists = false;
  if (installed.is_array()) {
    for (const json &app : installed) {
      const std::string name = str(app, "name", "");
      if (!app_name(name)) continue;
      if (name == selected_app) selected_exists = true;
      std::string text = name + "  " + str(app, "version") +
                         "  [" + str(app, "storage") + "]";
      if (name == active_display) text += "  (Display)";
      if (name == active_web) text += "  (Web)";
      if (name == selected_app) text = "> " + text;
      if ((availability == "checked" || availability == "cached") && updates.is_array()) {
        for (const json &update : updates) {
          if (str(update, "name", "") != name) continue;
          const json offered = field(update, "offered");
          text += "\nUpdate: " + str(offered, "version") + " (" + str(offered, "channel") + ")";
          if (availability == "cached") text += " - Cached information";
          break;
        }
      }
      add_button(installed_list, text, Action::SelectApp, name);
    }
  }
  if (!selected_exists) {
    selected_app.clear();
    lv_label_set_text(selected_detail, "");
  }
  label(staged_list, "STAGED PACKAGES");
  const json staged = field(snapshot, "packages");
  const json verified = field(snapshot, "verified_packages");
  bool stage_exists = false;
  if (staged.is_array()) {
    for (const json &pkg : staged) {
      const std::string id = str(pkg, "id", "");
      if (!package_id(id)) continue;
      if (id == selected_stage) stage_exists = true;
      bool trusted = false;
      if (verified.is_array())
        for (const json &item : verified)
          if (item.is_string() && item.get<std::string>() == id) trusted = true;
      add_button(staged_list, (id == selected_stage ? "> " : "") +
                 id.substr(0, 8) + (trusted ? "  (verified)" : "  (unverified)"),
                 Action::SelectStage, id);
    }
  }
  if (!stage_exists) selected_stage.clear();
  label(usb_list, "USB PACKAGES");
  if (usb_files.empty()) label(usb_list, "No .fre3app found on the USB mount.");
  for (const std::string &name : usb_files)
    add_button(usb_list, (name == selected_usb ? "> " : "") + name,
               Action::SelectUsb, name);
  render_controls();
}

void AppsPanel::clear_confirmation()
{
  armed_action.clear();
}

void AppsPanel::render_controls()
{
  lv_obj_clean(actions);
  for (auto it = controls.begin(); it != controls.end();) {
    // Old buttons owned by the dynamic actions container have been deleted.
    // Other pointers remain valid until render() reconstructs their parents.
    if (it->second.action != Action::SelectApp &&
        it->second.action != Action::SelectStage &&
        it->second.action != Action::SelectUsb)
      it = controls.erase(it);
    else ++it;
  }
  label(actions, "ACTIONS");
  const json execution = field(snapshot, "execution");
  const std::string state = str(execution, "state", "");
  if (busy || state == "uploading" || state == "running") {
    label(actions, "An operation is in progress. Status refreshes automatically.");
    return;
  }
  if (state == "unknown") {
    label(actions, "Outcome unknown. Inspect installed apps before acknowledging.", RED);
    add_button(actions, armed_action == "acknowledge" ? "Confirm acknowledgement" :
               "Acknowledge unknown result", Action::Acknowledge, {}, true);
    return;
  }
  if (!selected_app.empty()) {
    label(actions, "App: " + selected_app);
    add_button(actions, "Read details", Action::Detail);
    const json apps = field(snapshot, "installed");
    bool is_display = false;
    if (apps.is_array()) for (const json &app : apps)
      if (str(app, "name", "") == selected_app) is_display = flag(app, "display_frontend");
    const std::string active = str(field(snapshot, "display"), "active", "");
    if (is_display && selected_app != active)
      add_button(actions, armed_action == "select" ? "Confirm display switch" :
                 "Select as display", Action::SelectDisplay, {}, true);
    if (selected_app != active)
      add_button(actions, armed_action == "remove" ? "Confirm removal" :
                 "Remove installed app", Action::Remove, {}, true);
    else label(actions, "Active display cannot be removed from this screen.");
  }
  if (!selected_stage.empty()) {
    label(actions, "Staged: " + selected_stage.substr(0, 8));
    const json verified = field(snapshot, "verified_packages");
    bool trusted = false;
    if (verified.is_array()) for (const json &item : verified)
      if (item.is_string() && item.get<std::string>() == selected_stage) trusted = true;
    if (!trusted) add_button(actions, "Verify signature", Action::Verify);
    if (trusted) {
      add_button(actions, armed_action == "install-sys" ? "Confirm SYS install" :
                 "Install to SYS", Action::InstallSys, {}, !armed_action.empty() && armed_action == "install-sys");
      add_button(actions, armed_action == "install-home" ? "Confirm HOME install" :
                 "Install to HOME", Action::InstallHome, {}, !armed_action.empty() && armed_action == "install-home");
      if (!selected_app.empty())
        add_button(actions, armed_action == "update" ? "Confirm update" :
                   "Update selected app", Action::Update, {}, !armed_action.empty() && armed_action == "update");
    }
    add_button(actions, "Discard staged package", Action::Discard);
  }
  if (!selected_usb.empty()) {
    label(actions, "USB: " + selected_usb);
    add_button(actions, "Upload and verify package", Action::Upload);
  }
  if (!armed_action.empty()) label(actions, "Select Confirm, or choose another item to cancel.", RED);
}

void AppsPanel::take_action(Action action)
{
  if (busy) return;
  const json execution = field(snapshot, "execution");
  const std::string state = str(execution, "state", "");
  if (state == "running" || state == "uploading") return;
  if (state == "unknown" && action != Action::Acknowledge) return;
  if (state != "unknown" && action == Action::Acknowledge) return;

  std::string operation;
  json body = json::object();
  std::string confirmation;
  switch (action) {
    case Action::Remove:
      if (!app_name(selected_app) || str(field(snapshot, "display"), "active", "") == selected_app) return;
      operation = "remove"; body["name"] = selected_app;
      confirmation = "remove"; break;
    case Action::SelectDisplay:
      if (!app_name(selected_app) || str(field(snapshot, "display"), "active", "") == selected_app) return;
      // A different display frontend may terminate this process. Confirmation is mandatory.
      operation = "display/select"; body["name"] = selected_app;
      confirmation = "select"; break;
    case Action::Verify:
      if (!package_id(selected_stage)) return;
      operation = "verify"; body["package_id"] = selected_stage; break;
    case Action::InstallSys:
    case Action::InstallHome:
      if (!package_id(selected_stage)) return;
      operation = "install"; body["package_id"] = selected_stage;
      body["storage"] = action == Action::InstallHome ? "home" : "sys";
      confirmation = action == Action::InstallHome ? "install-home" : "install-sys";
      break;
    case Action::Update:
      if (!package_id(selected_stage) || !app_name(selected_app)) return;
      operation = "update"; body["package_id"] = selected_stage; body["name"] = selected_app;
      confirmation = "update"; break;
    case Action::Discard:
      if (!package_id(selected_stage)) return;
      operation = "discard"; body["package_id"] = selected_stage; break;
    case Action::Acknowledge:
      operation = "acknowledge"; confirmation = "acknowledge"; break;
    case Action::Upload: {
      if (selected_usb.empty() || std::find(usb_files.begin(), usb_files.end(), selected_usb) == usb_files.end()) return;
      const std::string path = usb_root() + "/" + selected_usb;
      clear_confirmation();
      start(Operation::Action, [path]() { return ManagementClient::upload_app(path); });
      return;
    }
    default: return;
  }
  if (!confirmation.empty() && armed_action != confirmation) {
    armed_action = confirmation;
    render_controls();
    return;
  }
  clear_confirmation();
  start(Operation::Action, [operation, payload=body.dump()]() {
    return ManagementClient::apps_action(operation, payload);
  });
}

void AppsPanel::handle(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
  lv_obj_t *target = lv_event_get_current_target(event);
  if (target == back_btn) { open = false; lv_obj_move_background(cont); return; }
  if (target == refresh_btn) { clear_confirmation(); load(); return; }
  auto found = controls.find(target);
  if (found == controls.end()) return;
  const Control control = found->second;
  switch (control.action) {
    case Action::SelectApp:
      clear_confirmation(); selected_app = control.argument;
      lv_label_set_text(selected_detail, "Select 'Read details' to query service status.");
      render(); break;
    case Action::SelectStage:
      clear_confirmation(); selected_stage = control.argument; render(); break;
    case Action::SelectUsb:
      clear_confirmation(); selected_usb = control.argument; render(); break;
    case Action::Detail: detail(); break;
    default: take_action(control.action); break;
  }
}

void AppsPanel::event_callback(lv_event_t *event)
{
  static_cast<AppsPanel *>(event->user_data)->handle(event);
}

void AppsPanel::timer_callback(lv_timer_t *timer)
{
  static_cast<AppsPanel *>(timer->user_data)->poll();
}
