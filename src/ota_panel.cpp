#include "ota_panel.h"

#include <algorithm>
#include <cstdlib>
#include <dirent.h>
#include <fcntl.h>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

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
  return obj.is_object() && obj.contains(name) ? obj.at(name) : json(nullptr);
}
std::string str(const json &obj, const char *name, const std::string &fallback = "Unavailable")
{
  const json value = field(obj, name);
  if (value.is_string()) return value.get<std::string>();
  if (value.is_boolean()) return value.get<bool>() ? "Yes" : "No";
  return fallback;
}
bool id_ok(const std::string &id)
{
  return id.size() == 32 && std::all_of(id.begin(), id.end(), [](char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
  });
}
std::string error_message(const ManagementClient::Response &answer)
{
  try {
    const json err = field(json::parse(answer.body), "error");
    const json message = field(err, "message");
    if (message.is_string()) return message.get<std::string>();
  } catch (...) {}
  return answer.error.empty() ? "Management response invalid" : answer.error;
}
std::string usb_root()
{
#ifdef SIMULATOR
  const char *path = std::getenv("FRE3NDERSCREEN_USB_ROOT");
  if (path && *path) return path;
#endif
  return "/run/fre3nder/usb";
}
bool ota_filename(const std::string &name)
{
  if (name.size() < 5 || name.size() > 128 ||
      name.compare(name.size() - 4, 4, ".ota") != 0) return false;
  return std::all_of(name.begin(), name.end(), [](char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.';
  });
}
std::string package_from_transaction(const json &transaction)
{
  const std::string path = str(field(transaction, "package"), "path", "");
  constexpr const char *prefix = "/ota/packages/";
  if (path.compare(0, 14, prefix) != 0 || path.size() != 14 + 32 + 4 ||
      path.substr(path.size() - 4) != ".ota") return {};
  const auto id = path.substr(14, 32);
  return id_ok(id) ? id : std::string();
}
std::string operation_for_state(const std::string &state)
{
  if (state == "awaiting-confirmation") return "confirm";
  if (state == "confirmed") return "write";
  if (state == "written-awaiting-readback") return "readback";
  if (state == "readback-verified") return "prepare-activation";
  if (state == "activation-prepared") return "reboot";
  return {};
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
} // namespace

OtaPanel::OtaPanel()
  : cont(lv_obj_create(lv_scr_act())), back_btn(lv_btn_create(cont)),
    refresh_btn(lv_btn_create(cont)), content(lv_obj_create(cont)),
    message(nullptr), details(nullptr), packages(nullptr), choices(nullptr),
    actions(nullptr), timer(nullptr)
{
  lv_obj_move_background(cont);
  lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
  lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(cont, 8, 0);
  lv_obj_set_style_bg_color(cont, lv_color_hex(BG), 0);
  lv_obj_set_style_border_width(cont, 0, 0);
  for (auto *button : {back_btn, refresh_btn}) {
    lv_obj_set_size(button, 40, 34);
    lv_obj_set_style_bg_color(button, lv_color_hex(CARD), 0);
    lv_obj_set_style_border_width(button, 1, 0);
    lv_obj_set_style_border_color(button, lv_color_hex(BORDER), 0);
    lv_obj_set_style_radius(button, 8, 0);
    lv_obj_add_event_cb(button, event_callback, LV_EVENT_CLICKED, this);
  }
  lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 0, 3);
  lv_obj_align(refresh_btn, LV_ALIGN_TOP_RIGHT, 0, 3);
  auto *back_icon = lv_label_create(back_btn);
  lv_label_set_text(back_icon, LV_SYMBOL_LEFT);
  lv_obj_set_style_text_color(back_icon, lv_color_hex(WHITE), 0);
  lv_obj_center(back_icon);
  auto *refresh_icon = lv_label_create(refresh_btn);
  lv_label_set_text(refresh_icon, LV_SYMBOL_REFRESH);
  lv_obj_set_style_text_color(refresh_icon, lv_color_hex(CYAN), 0);
  lv_obj_center(refresh_icon);
  auto *title = lv_label_create(cont);
  lv_label_set_text(title, "Updates");
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
  message = label(content, "Open this page to inspect OTA state.");
  details = label(content, "");
  packages = lv_obj_create(content);
  choices = lv_obj_create(content);
  actions = lv_obj_create(content);
  for (auto *part : {packages, choices, actions}) {
    lv_obj_set_width(part, LV_PCT(100));
    lv_obj_set_height(part, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(part, 0, 0);
    lv_obj_set_style_pad_row(part, 5, 0);
    lv_obj_set_style_border_width(part, 0, 0);
    lv_obj_set_style_bg_opa(part, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(part, LV_FLEX_FLOW_COLUMN);
  }
  timer = lv_timer_create(timer_callback, 120, this);
}
OtaPanel::~OtaPanel()
{
  if (timer) lv_timer_del(timer);
  if (worker.joinable()) worker.join();
  lv_obj_del(cont);
}
void OtaPanel::foreground()
{
  open = true;
  lv_obj_move_foreground(cont);
  refresh();
}
void OtaPanel::start(bool is_status, std::string operation,
                     std::function<ManagementClient::Response()> request)
{
  if (busy) return;
  if (worker.joinable()) worker.join();
  status_request = is_status;
  request_operation = std::move(operation);
  busy = true;
  ready.store(false, std::memory_order_release);
  if (!is_status) {
    submitted_operation = request_operation;
    lv_label_set_text(message, "Sending OTA request. Do not repeat it.");
  }
  worker = std::thread([this, request]() {
    auto answer = request();
    {
      std::lock_guard<std::mutex> guard(result_mutex);
      result = std::move(answer);
    }
    ready.store(true, std::memory_order_release);
  });
}
void OtaPanel::refresh()
{
  start(true, "status", [] { return ManagementClient::ota_status(); });
}
void OtaPanel::scan_usb()
{
  usb_files.clear();
  const std::string root = usb_root();
  const int fd = ::open(root.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
  if (fd < 0) return;
  DIR *dir = fdopendir(fd);
  if (!dir) { close(fd); return; }
  while (dirent *entry = readdir(dir)) {
    const std::string name = entry->d_name;
    if (!ota_filename(name)) continue;
    struct stat meta {};
    if (fstatat(fd, name.c_str(), &meta, AT_SYMLINK_NOFOLLOW) != 0 ||
        !S_ISREG(meta.st_mode) || meta.st_size <= 0 ||
        meta.st_size > 96LL * 1024 * 1024) continue;
    usb_files.push_back(name);
    if (usb_files.size() >= 32) break;
  }
  closedir(dir);
  std::sort(usb_files.begin(), usb_files.end());
  if (std::find(usb_files.begin(), usb_files.end(), selected_usb) == usb_files.end())
    selected_usb.clear();
}
std::string OtaPanel::transaction_package() const
{
  return package_from_transaction(field(field(snapshot, "core"), "transaction"));
}
std::string OtaPanel::next_operation() const
{
  const json core = field(snapshot, "core");
  return operation_for_state(str(field(core, "transaction"), "state", ""));
}
bool OtaPanel::execution_blocks() const
{
  const json execution = field(snapshot, "execution");
  const std::string state = str(execution, "state", "");
  return busy || uncertain || reboot_sent ||
      field(field(snapshot, "core"), "busy") == true ||
      state == "running" || state == "uploading" || state == "unknown" ||
      !pending_execution.empty();
}
bool OtaPanel::backup_choice_valid(const std::string &role, const std::string &choice) const
{
  if (choice.empty()) return false;
  const json offer = field(field(preflight, "backup_offer"), role.c_str());
  if (!offer.is_object()) return false;
  if (choice == "none") return field(offer, "required") == false;
  if (field(offer, "offered") != true) return false;
  const json targets = field(offer, "targets");
  if (!targets.is_array()) return false;
  for (const json &target : targets)
    if (target.is_string() && target.get<std::string>() == choice) return true;
  return false;
}
void OtaPanel::clear_selection()
{
  armed.clear();
  check_after_verify = false;
  preflight = nullptr;
  planned = nullptr;
  home_choice.clear();
  sys_choice.clear();
}

// A running workflow is bound to the selected package and the Core transaction.
// On error or a lost receipt we stop; a fresh status read never resumes it.
void OtaPanel::pause_update(const std::string &reason, bool unclear)
{
  update_active = false;
  check_after_verify = false;
  if (unclear) uncertain = true;
  lv_label_set_text(message, reason.c_str());
}

void OtaPanel::advance_update(const std::string &completed, const json &core_result)
{
  if (completed == "verify" && check_after_verify) {
    check_after_verify = false;
    if (!id_ok(selected_package) ||
        str(field(core_result, "package"), "version", "").empty()) {
      pause_update("Package verification result is invalid. Inspect status.", true);
      return;
    }
    submit("preflight", {{"package_id", selected_package}});
    return;
  }
  if (!update_active || !open) return;
  if (execution_blocks()) {
    pause_update("Another OTA operation is active. Update stopped.", true);
    return;
  }
  if (!id_ok(update_package)) {
    pause_update("Update package identity is missing. Inspect status.", true);
    return;
  }
  const json core = field(snapshot, "core");
  const json transaction = field(core, "transaction");
  if (completed == "backup-plan") {
    const json selection = field(core_result, "backup_selection");
    for (const auto &role : {"home", "sys"}) {
      const auto expected = std::string(role) == "home" ? update_home : update_sys;
      const json item = field(selection, role);
      const json target = field(item, "target");
      if (field(item, "selected") != (expected != "none") ||
          (expected == "none" ? !target.is_null() :
           !target.is_string() || target.get<std::string>() != expected)) {
        pause_update("Backup plan differs from the confirmed selection. Update stopped.", true);
        return;
      }
    }
    if (str(field(core_result, "package"), "version", "") != update_version ||
        str(field(core_result, "target"), "slot", "") != update_target ||
        field(core, "transaction").is_object()) {
      pause_update("Core update context changed during backup planning.", true);
      return;
    }
    submit("backup", {{"package_id", update_package},
                      {"home", update_home}, {"sys", update_sys}});
    return;
  }
  const std::map<std::string, std::string> states = {
    {"backup", "awaiting-confirmation"}, {"confirm", "confirmed"},
    {"write", "written-awaiting-readback"},
    {"readback", "readback-verified"},
    {"prepare-activation", "activation-prepared"},
  };
  auto expected = states.find(completed);
  if (expected == states.end()) return;
  const std::string transaction_id = str(transaction, "transaction_id", "");
  if (!transaction.is_object() || !id_ok(transaction_id) ||
      (update_transaction.empty() ? completed != "backup" :
       transaction_id != update_transaction) ||
      transaction_package() != update_package ||
      str(transaction, "state", "") != expected->second ||
      str(field(transaction, "package"), "version", "") != update_version ||
      str(field(transaction, "target"), "slot", "") != update_target) {
    pause_update("Core transaction changed or did not advance as expected. Update stopped.", true);
    return;
  }
  if (completed == "backup") {
    update_transaction = transaction_id;
    const json backup = field(transaction, "backup_result");
    for (const auto &role : {"home", "sys"}) {
      const json item = field(backup, role);
      const auto selected = field(item, "selected");
      const auto required = (std::string(role) == "home" ? update_home : update_sys) != "none";
      if (selected != required || (required && field(item, "verified") != true)) {
        pause_update("Required backup was not verified. Update stopped.", true);
        return;
      }
    }
  }
  const std::string next = operation_for_state(expected->second);
  if (next.empty()) {
    pause_update("Unknown next OTA phase. Update stopped.", true);
    return;
  }
  const json request = next == "reboot" ? json({{"transaction_id", update_transaction}})
      : json({{"package_id", update_package}, {"transaction_id", update_transaction}});
  // Core status is authoritative; do not issue the next operation from HTTP 202 alone.
  submit(next, request);
  if (next == "reboot") reboot_sent = true;
}
void OtaPanel::poll()
{
  if (ready.exchange(false, std::memory_order_acq_rel)) {
    ManagementClient::Response answer;
    {
      std::lock_guard<std::mutex> guard(result_mutex);
      answer = std::move(result);
    }
    if (worker.joinable()) worker.join();
    busy = false;
    if (!answer.ok) {
      const auto error = error_message(answer);
      lv_label_set_text(message, error.c_str());
      // A lost POST response is not evidence that the Core rejected it.
      const bool unclear = !status_request &&
          (answer.status == 0 || answer.status >= 500);
      const bool lost_status = status_request && !pending_execution.empty();
      pause_update(unclear ? "OTA submission outcome unclear. Inspect Core status before retrying."
                          : lost_status ? "OTA status unavailable during an operation. Inspect Core before continuing."
                          : error, unclear || lost_status);
      if (status_request && lost_status) pending_execution.clear();
      if (!status_request && request_operation == "reboot" && !unclear)
        reboot_sent = false; // An explicit server rejection did not request reboot.
      render();
    } else {
      try {
        const json payload = json::parse(answer.body);
        if (field(payload, "api_version") != 1 || field(payload, "ok") != true)
          throw std::runtime_error("Invalid Management OTA response");
        if (status_request) {
          if (field(payload, "operation") != "ota-status" || !field(payload, "ota").is_object())
            throw std::runtime_error("Invalid OTA status");
          snapshot = field(payload, "ota");
          last_refresh = lv_tick_get();
          scan_usb();
          const json execution = field(snapshot, "execution");
          const std::string state = str(execution, "state", "");
          if (!pending_execution.empty()) {
            if (str(execution, "id", "") != pending_execution ||
                str(execution, "operation", "") != submitted_operation) {
              pause_update("OTA execution receipt changed. Inspect Core state; do not retry.", true);
            } else if (state == "succeeded") {
              const json core_result = field(execution, "result");
              const std::string op = str(execution, "operation", "");
              const std::string receipt_tx = str(execution, "transaction_id", "");
              const bool tx_matches = !update_active || update_transaction.empty() ||
                  op == "backup-plan" || op == "backup" ||
                  receipt_tx == update_transaction;
              if (str(core_result, "operation", "") == op && field(core_result, "ok") == true &&
                  tx_matches) {
                if (op != "reboot" && str(execution, "package_id", "") !=
                    (update_active ? update_package : selected_package)) {
                  pause_update("OTA receipt belongs to a different package. Inspect Core state.", true);
                } else {
                  if (op == "preflight") {
                    preflight = core_result;
                    preflight["package_id"] = str(execution, "package_id", "");
                    home_choice.clear(); sys_choice.clear(); planned = nullptr;
                  } else if (op == "backup-plan") {
                    planned = core_result;
                    planned["package_id"] = str(execution, "package_id", "");
                  }
                  if (op == "reboot") {
                    update_active = false;
                    if (str(core_result, "state", "") == "reboot-requested" &&
                        str(core_result, "transaction_id", "") == update_transaction) {
                      lv_label_set_text(message, "Printer reboot requested. Verify the new boot on the device.");
                    } else {
                      pause_update("Reboot request outcome unclear. Inspect the printer.", true);
                    }
                  } else {
                    lv_label_set_text(message, (op + " completed.").c_str());
                  }
                }
              } else {
                pause_update("OTA Core result could not be verified. Inspect state.", true);
              }
              pending_execution.clear();
              if (!uncertain && op != "reboot") advance_update(op, core_result);
            } else if (state == "failed" || state == "unknown") {
              pause_update("OTA " + state + ": " +
                  str(field(execution, "error"), "message", "Inspect Core state."),
                  state == "unknown");
              if (str(execution, "operation", "") == "reboot" && state == "failed")
                reboot_sent = false; // Explicit failure is not reboot acknowledgement.
              pending_execution.clear();
            } else if (state != "running" && state != "uploading") {
              pause_update("Unknown OTA execution state. Inspect Core status.", true);
              pending_execution.clear();
            }
          }
          render();
        } else {
          if (answer.status != 202 || field(payload, "operation") != "ota-accepted")
            throw std::runtime_error("Unexpected OTA acceptance");
          const json execution = field(payload, "execution");
          const std::string id = str(execution, "id", "");
          if (!id_ok(id) || str(execution, "operation", "") != request_operation)
            throw std::runtime_error("Invalid OTA acceptance identity");
          pending_execution = id;
          if (request_operation == "verify") {
            const auto uploaded = str(execution, "package_id", "");
            if (id_ok(uploaded)) selected_package = uploaded;
          }
          lv_label_set_text(message, request_operation == "reboot" ?
              "Restart accepted for processing; checking Core result..." :
              "Request accepted; checking Core result...");
          refresh();
        }
      } catch (const std::exception &error) {
        pause_update(error.what(), true);
      }
    }
  }
  if (open && !busy && !uncertain &&
      !pending_execution.empty() && lv_tick_elaps(last_refresh) > 1500) refresh();
}
void OtaPanel::add_button(lv_obj_t *parent, const std::string &text,
                          Action action, const std::string &value, bool dangerous)
{
  lv_obj_t *button = lv_btn_create(parent);
  lv_obj_set_width(button, LV_PCT(100));
  lv_obj_set_height(button, 48);
  lv_obj_set_style_pad_all(button, 8, 0);
  lv_obj_set_style_bg_color(button, lv_color_hex(CARD), 0);
  lv_obj_set_style_border_width(button, 1, 0);
  lv_obj_set_style_border_color(button, lv_color_hex(dangerous ? RED : BORDER), 0);
  lv_obj_set_style_radius(button, 6, 0);
  lv_obj_add_event_cb(button, event_callback, LV_EVENT_CLICKED, this);
  auto *text_label = label(button, text, dangerous ? RED : WHITE);
  lv_obj_set_style_text_align(text_label, LV_TEXT_ALIGN_CENTER, 0);
  controls.emplace(button, Control{action, value});
}
void OtaPanel::render()
{
  controls.clear();
  for (auto *part : {packages, choices, actions}) lv_obj_clean(part);
  const json core = field(snapshot, "core");
  const json transaction = field(core, "transaction");
  const json execution = field(snapshot, "execution");
  const std::string transaction_id = str(transaction, "transaction_id", "");
  const std::string tx_package = transaction_package();
  if (transaction.is_object() && id_ok(tx_package)) selected_package = tx_package;
  std::string info = "OTA CORE\n";
  const json known = field(core, "known_good");
  info += "Known good: " + str(known, "version", "None") + "\n";
  if (transaction.is_object()) {
    info += "Phase: " + str(transaction, "state") +
            "\nVersion: " + str(field(transaction, "package"), "version") +
            "\nActive: " + str(field(transaction, "runtime"), "active_slot") +
            "   Target: " + str(field(transaction, "target"), "slot") +
            "\nTransaction: " + transaction_id.substr(0, 8);
    const json backups = field(transaction, "backup_result");
    for (const char *role : {"home", "sys"}) {
      const json item = field(backups, role);
      info += std::string("\n") + (role[0] == 'h' ? "HOME: " : "SYS: ");
      info += field(item, "selected") == true ?
          (field(item, "verified") == true ? "Verified" : "Unverified") : "Declined";
    }
  } else if (preflight.is_object()) {
    info += "Installed: " + str(field(preflight, "runtime"), "installed_version") +
            "\nPackage: " + str(field(preflight, "package"), "version") +
            "\nActive: " + str(field(preflight, "runtime"), "active_slot") +
            "   Target: " + str(field(preflight, "target"), "slot");
  } else {
    info += "No active transaction";
  }
  if (execution.is_object()) {
    info += "\nLast: " + str(execution, "operation") + " (" + str(execution, "state") + ")";
    if (field(execution, "error").is_object())
      info += "\n" + str(field(execution, "error"), "message");
  }
  lv_label_set_text(details, info.c_str());

  label(packages, "STAGED OTA PACKAGES");
  const json staged = field(snapshot, "packages");
  if (staged.is_array()) for (const json &item : staged) {
    const auto id = str(item, "id", "");
    if (!id_ok(id)) continue;
    add_button(packages, (id == selected_package ? "> " : "") + id.substr(0, 8) + "  (staged)",
               Action::SelectPackage, id);
  }
  label(packages, "USB OTA PACKAGES");
  if (usb_files.empty()) label(packages, "No .ota package on the USB mount.");
  for (const auto &file : usb_files)
    add_button(packages, (file == selected_usb ? "> " : "") + file, Action::SelectUsb, file);

  if (preflight.is_object() && !transaction.is_object()) {
    label(choices, "BACKUP DESTINATIONS (select both explicitly)");
    for (const auto &entry : {std::make_pair("home", "usb"), std::make_pair("home", "none"),
                             std::make_pair("sys", "usb"), std::make_pair("sys", "home"),
                             std::make_pair("sys", "none")}) {
      if (!backup_choice_valid(entry.first, entry.second)) continue;
      const std::string current = entry.first == std::string("home") ? home_choice : sys_choice;
      const auto action = entry.first == std::string("home") ?
          (entry.second == std::string("usb") ? Action::HomeUsb : Action::HomeNone) :
          (entry.second == std::string("usb") ? Action::SysUsb :
           entry.second == std::string("home") ? Action::SysHome : Action::SysNone);
      add_button(choices, (current == entry.second ? "> " : "") +
                 std::string(entry.first == std::string("home") ? "HOME: " : "SYS: ") +
                 (entry.second == std::string("none") ? "NO BACKUP" : entry.second),
                 action, {}, entry.second == std::string("none"));
    }
    label(choices, "HOME preserved after OTA. SYS resets on the new boot.");
    if (home_choice == "none" || sys_choice == "none")
      label(choices, "WARNING: Selected backups will NOT be created for declined roles.", RED);
  }

  if (uncertain) {
    label(actions, "Outcome unclear. Inspect Core status through SSH. No automatic retry.", RED);
    return;
  }
  if (reboot_sent) {
    label(actions, pending_execution.empty() ?
        "Restart requested. Check the printer boot, version and services." :
        "Restart submitted; awaiting acknowledgement. Do not resubmit.", CYAN);
    return;
  }
  if (update_active) {
    label(actions, "Updating automatically. Current step: " +
        submitted_operation + ". Do not switch off the printer.", CYAN);
    return;
  }
  if (execution_blocks()) {
    label(actions, "OTA operation running or blocked. Waiting for Core result.");
    return;
  }
  if (transaction.is_object()) {
    if (!id_ok(transaction_id) || !id_ok(tx_package) ||
        (field(core, "activation").is_object() && next_operation() != "reboot")) {
      label(actions, "Core transaction requires inspection.", RED);
      return;
    }
    if (next_operation().empty()) {
      label(actions, "Unknown Core phase. Do not continue here.", RED);
      return;
    }
    add_button(actions, armed == "update" ? "CONFIRM UPDATE" : "Continue Fre3nder update",
               Action::Update, {}, true);
    label(actions, "Confirm once to run remaining Core steps automatically. "
          "Completed steps will not be repeated.", RED);
    return;
  }
  if (field(core, "activation").is_object()) {
    label(actions, "Activation record exists without a transaction. Inspect SSH.", RED);
    return;
  }
  if (!selected_usb.empty())
    add_button(actions, "Upload selected .ota", Action::Upload);
  if (id_ok(selected_package))
    add_button(actions, "Check package", Action::Verify);
  if (preflight.is_object() && str(preflight, "package_id", "") == selected_package &&
      backup_choice_valid("home", home_choice) && backup_choice_valid("sys", sys_choice)) {
    add_button(actions, armed == "update" ? "CONFIRM UPDATE" : "Update Fre3nder",
               Action::Update, {}, true);
    label(actions, "Backup, write, readback, activation and reboot follow automatically. "
          "HOME is preserved; SYS resets on the new boot.", RED);
  }
  if (armed == "update") label(actions, "Tap CONFIRM UPDATE to authorize the complete update.", RED);
}
void OtaPanel::submit(const std::string &operation, const json &payload)
{
  if (execution_blocks()) return;
  armed.clear();
  start(false, operation, [operation, body = payload.dump()] {
    return ManagementClient::ota_action(operation, body);
  });
}
void OtaPanel::take_action(Action action)
{
  if (execution_blocks() || update_active) return;
  const json core = field(snapshot, "core");
  const json transaction = field(core, "transaction");
  if (action == Action::Upload) {
    if (transaction.is_object() || field(core, "activation").is_object() ||
        selected_usb.empty()) return;
    const auto path = usb_root() + "/" + selected_usb;
    clear_selection();
    selected_package.clear();
    check_after_verify = true; // Successful upload already started signature verification.
    start(false, "verify", [path] { return ManagementClient::upload_ota(path); });
  } else if (action == Action::Verify) {
    if (transaction.is_object() || field(core, "activation").is_object() ||
        !id_ok(selected_package)) return;
    clear_selection();
    check_after_verify = true;
    submit("verify", {{"package_id", selected_package}});
  } else if (action == Action::Update) {
    const bool continuing = transaction.is_object();
    const std::string package = continuing ? transaction_package() : selected_package;
    if (!id_ok(package)) return;
    if (continuing) {
      if (!id_ok(str(transaction, "transaction_id", "")) || next_operation().empty() ||
          (field(core, "activation").is_object() && next_operation() != "reboot")) return;
    } else if (!preflight.is_object() || str(preflight, "package_id", "") != package ||
               !backup_choice_valid("home", home_choice) ||
               !backup_choice_valid("sys", sys_choice) ||
               field(core, "activation").is_object()) return;
    if (armed != "update") {
      armed = "update";
      render();
      return;
    }
    armed.clear();
    const json context = continuing ? transaction : preflight;
    update_package = package;
    update_transaction = continuing ? str(transaction, "transaction_id") : "";
    update_version = str(field(context, "package"), "version", "");
    update_target = str(field(context, "target"), "slot", "");
    update_home = home_choice;
    update_sys = sys_choice;
    if (update_version.empty() || (update_target != "A" && update_target != "B")) {
      pause_update("OTA context is incomplete; cannot start update.", true);
      render();
      return;
    }
    update_active = true;
    if (continuing) {
      const std::string next = next_operation();
      const json request = next == "reboot" ? json({{"transaction_id", update_transaction}})
          : json({{"package_id", update_package}, {"transaction_id", update_transaction}});
      submit(next, request);
      if (next == "reboot") reboot_sent = true;
    } else {
      submit("backup-plan", {{"package_id", package},
                             {"home", update_home}, {"sys", update_sys}});
    }
  }
  render();
}
void OtaPanel::handle(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
  lv_obj_t *target = lv_event_get_current_target(event);
  if (target == back_btn) {
    open = false;
    update_active = false; // Do not resume a partially completed update on return.
    check_after_verify = false;
    armed.clear();
    lv_obj_move_background(cont);
    return;
  }
  if (target == refresh_btn) { if (!busy) { armed.clear(); refresh(); } return; }
  const auto it = controls.find(target);
  if (it == controls.end()) return;
  const Control choice = it->second;
  if (choice.action == Action::SelectUsb || choice.action == Action::SelectPackage) {
    if (execution_blocks() || field(field(snapshot, "core"), "transaction").is_object()) return;
    clear_selection();
    if (choice.action == Action::SelectUsb) selected_usb = choice.value;
    else selected_package = choice.value;
    render(); return;
  }
  if (choice.action == Action::HomeUsb || choice.action == Action::HomeNone ||
      choice.action == Action::SysUsb || choice.action == Action::SysHome ||
      choice.action == Action::SysNone) {
    if (execution_blocks()) return;
    armed.clear(); planned = nullptr;
    if (choice.action == Action::HomeUsb) home_choice = "usb";
    if (choice.action == Action::HomeNone) home_choice = "none";
    if (choice.action == Action::SysUsb) sys_choice = "usb";
    if (choice.action == Action::SysHome) sys_choice = "home";
    if (choice.action == Action::SysNone) sys_choice = "none";
    render(); return;
  }
  take_action(choice.action);
}
void OtaPanel::event_callback(lv_event_t *event)
{
  static_cast<OtaPanel *>(event->user_data)->handle(event);
}
void OtaPanel::timer_callback(lv_timer_t *timer)
{
  static_cast<OtaPanel *>(timer->user_data)->poll();
}
