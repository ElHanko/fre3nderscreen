#include "system_panel.h"

#include "management_client.h"
#include "hv/json.hpp"

#include <cstdint>
#include <iomanip>
#include <sstream>

namespace {
using nlohmann::json;
constexpr uint32_t BG = 0x080B0D;
constexpr uint32_t CARD = 0x11171B;
constexpr uint32_t BORDER = 0x25323A;
constexpr uint32_t CYAN = 0x00E5FF;
constexpr uint32_t WHITE = 0xFFFFFF;

json member(const json &parent, const char *name)
{
  if (!parent.is_object() || !parent.contains(name)) return nullptr;
  return parent.at(name);
}

std::string value(const json &parent, const char *name)
{
  json item = member(parent, name);
  if (item.is_string()) return item.get<std::string>();
  if (item.is_number_integer()) return std::to_string(item.get<int64_t>());
  if (item.is_number_float()) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(1) << item.get<double>();
    return stream.str();
  }
  if (item.is_boolean()) return item.get<bool>() ? "Yes" : "No";
  return "Unavailable";
}

std::string megabytes(const json &parent, const char *name)
{
  json item = member(parent, name);
  if (!item.is_number_unsigned() && !item.is_number_integer()) return "Unavailable";
  if (item.is_number_integer() && item.get<int64_t>() < 0) return "Unavailable";
  std::ostringstream stream;
  stream << std::fixed << std::setprecision(1)
         << (item.get<double>() / (1024.0 * 1024.0)) << " MiB";
  return stream.str();
}

std::string selected(const json &parent, const char *name)
{
  json entry = member(parent, name);
  if (!entry.is_object()) return "Unavailable";
  const auto status = value(entry, "status");
  if (status == "none") return "None";
  if (status == "selected") return value(entry, "name");
  return "Unavailable";
}

std::string compact_utc_time(const std::string &iso)
{
  // Management API guarantees UTC ISO 8601; show only minute precision on
  // the narrow display. Preserve unknown formats verbatim.
  if (iso.size() < 16 || iso[4] != '-' || iso[7] != '-' || iso[10] != 'T' ||
      iso[13] != ':') return iso;
  return iso.substr(8, 2) + "." + iso.substr(5, 2) + "." + iso.substr(0, 4) +
         " " + iso.substr(11, 5) + " UTC";
}

std::string render(const std::string &body)
{
  try {
    const json response = json::parse(body);
    if (!response.is_object() || member(response, "ok") != true ||
        member(response, "operation") != "system-details")
      return "Management returned an invalid response.";
    const json info = member(response, "system");
    if (!info.is_object()) return "System information is unavailable.";
    const json platform = member(info, "platform");
    const json runtime = member(info, "runtime");
    const json memory = member(info, "memory");
    const json boot = member(info, "boot");
    const json applications = member(info, "applications");
    const json maintenance = member(info, "maintenance");
    std::ostringstream text;
    text << "PLATFORM\n"
         << "Fre3nder: " << value(platform, "version") << "\n"
         << "Hardware: " << value(platform, "hardware_model") << "\n"
         << "CPU: " << value(platform, "cpu_model") << "\n"
         << "Architecture: " << value(platform, "architecture") << "\n"
         << "Kernel: " << value(platform, "kernel_version") << "\n"
         << "Hostname: " << value(platform, "hostname") << "\n\n"
         << "RUNTIME\n"
         << "Uptime: " << value(runtime, "uptime_seconds") << " seconds\n"
         << "Time: " << compact_utc_time(value(runtime, "system_time")) << "\n\n"
         << "MEMORY\n"
         << "Used: " << megabytes(memory, "used_bytes") << "\n"
         << "Available: " << megabytes(memory, "available_bytes") << "\n"
         << "Total: " << megabytes(memory, "total_bytes") << "\n"
         << "Swap used: " << megabytes(memory, "swap_used_bytes") << "\n\n"
         << "BOOT\n"
         << "Slot: " << value(boot, "active_slot") << "\n"
         << "RootFS: " << value(boot, "active_rootfs") << "\n"
         << "Status: " << value(boot, "root_status") << "\n\n";

    json storage = member(info, "storage");
    text << "STORAGE\n";
    if (storage.is_array()) {
      for (const auto &disk : storage) {
        text << value(disk, "role") << ": " << megabytes(disk, "used_bytes")
             << " / " << megabytes(disk, "total_bytes") << "\n"
             << "Free: " << megabytes(disk, "available_bytes") << "\n";
      }
    } else {
      text << "Unavailable\n";
    }

    const json network = member(info, "network");
    const json route = member(network, "default_ipv4_route");
    json interfaces = member(network, "interfaces");
    text << "\nNETWORK\n";
    if (interfaces.is_array()) {
      for (const auto &iface : interfaces) {
        text << value(iface, "name") << " (" << value(iface, "kind") << "): "
             << value(iface, "state") << "\n";
        const json addresses = member(iface, "addresses");
        if (addresses.is_array()) {
          for (const auto &address : addresses)
            if (address.is_string()) text << "  " << address.get<std::string>() << "\n";
        }
      }
    } else {
      text << "Unavailable\n";
    }
    if (route.is_object())
      text << "Default route: " << value(route, "interface") << " via "
           << value(route, "gateway") << "\n";

    json services = member(info, "services");
    text << "\nSERVICES\n";
    if (services.is_array()) {
      for (const auto &service : services)
        text << value(service, "name") << ": " << value(service, "status") << "\n";
    } else {
      text << "Unavailable\n";
    }

    text << "\nAPPLICATIONS\n"
         << "Installed: " << value(applications, "installed_count") << "\n"
         << "Web: " << selected(applications, "web_frontend") << "\n"
         << "Display: " << selected(applications, "display_frontend") << "\n"
         << "Display state: " << value(applications, "display_runtime_status") << "\n\n"
         << "MAINTENANCE\n"
         << "Enabled: " << value(maintenance, "enabled") << "\n"
         << "Web: " << value(maintenance, "web_status") << "\n";

    json usb = member(info, "usb");
    text << "\nUSB\n";
    if (usb.is_array()) {
      if (usb.empty()) text << "No devices\n";
      for (const auto &device : usb) {
        text << value(device, "port") << ": " << value(device, "product")
             << "\nDriver: " << value(device, "driver") << "\n";
      }
    } else {
      text << "Unavailable\n";
    }
    return text.str();
  } catch (const json::exception &) {
    return "System information has an invalid format.";
  }
}

void button_style(lv_obj_t *object)
{
  lv_obj_set_style_bg_color(object, lv_color_hex(CARD), 0);
  lv_obj_set_style_border_width(object, 1, 0);
  lv_obj_set_style_border_color(object, lv_color_hex(BORDER), 0);
  lv_obj_set_style_radius(object, 8, 0);
  lv_obj_set_style_shadow_width(object, 0, 0);
}
} // namespace

SystemPanel::SystemPanel()
  : cont(lv_obj_create(lv_scr_act()))
  , back_btn(nullptr)
  , refresh_btn(nullptr)
  , info_label(nullptr)
  , timer(nullptr)
{
  lv_obj_move_background(cont);
  lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
  lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(cont, 8, 0);
  lv_obj_set_style_border_width(cont, 0, 0);
  lv_obj_set_style_bg_color(cont, lv_color_hex(BG), 0);

  lv_obj_t *header = lv_obj_create(cont);
  lv_obj_set_size(header, LV_PCT(100), 42);
  lv_obj_set_style_bg_opa(header, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(header, 0, 0);
  lv_obj_set_style_pad_all(header, 0, 0);

  back_btn = lv_btn_create(header);
  button_style(back_btn);
  lv_obj_set_size(back_btn, 40, 34);
  lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 0, 0);
  lv_obj_add_event_cb(back_btn, event_callback, LV_EVENT_CLICKED, this);
  lv_obj_t *back_text = lv_label_create(back_btn);
  lv_label_set_text(back_text, LV_SYMBOL_LEFT);
  lv_obj_center(back_text);

  lv_obj_t *title = lv_label_create(header);
  lv_label_set_text(title, "System");
  lv_obj_set_style_text_color(title, lv_color_hex(WHITE), 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
  lv_obj_center(title);

  refresh_btn = lv_btn_create(header);
  button_style(refresh_btn);
  lv_obj_set_size(refresh_btn, 40, 34);
  lv_obj_align(refresh_btn, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_obj_add_event_cb(refresh_btn, event_callback, LV_EVENT_CLICKED, this);
  lv_obj_t *refresh_text = lv_label_create(refresh_btn);
  lv_label_set_text(refresh_text, LV_SYMBOL_REFRESH);
  lv_obj_set_style_text_color(refresh_text, lv_color_hex(CYAN), 0);
  lv_obj_center(refresh_text);

  lv_obj_t *content = lv_obj_create(cont);
  lv_obj_set_size(content, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_pad_all(content, 10, 0);
  lv_obj_set_style_border_width(content, 1, 0);
  lv_obj_set_style_border_color(content, lv_color_hex(BORDER), 0);
  lv_obj_set_style_bg_color(content, lv_color_hex(CARD), 0);
  lv_obj_set_style_radius(content, 8, 0);
  lv_obj_align(content, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_height(content, lv_disp_get_ver_res(NULL) - 58);
  lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLLABLE);
  info_label = lv_label_create(content);
  lv_obj_set_width(info_label, LV_PCT(100));
  lv_label_set_long_mode(info_label, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_color(info_label, lv_color_hex(WHITE), 0);
  lv_obj_set_style_text_font(info_label, &lv_font_montserrat_12, 0);
  lv_label_set_text(info_label, "Open this page to load system information.");

  timer = lv_timer_create(timer_callback, 100, this);
}

SystemPanel::~SystemPanel()
{
  if (timer) lv_timer_del(timer);
  if (worker.joinable()) worker.join();
  lv_obj_del(cont);
}

void SystemPanel::foreground()
{
  lv_obj_move_foreground(cont);
  refresh();
}

void SystemPanel::refresh()
{
  if (worker.joinable()) {
    if (!ready.load(std::memory_order_acquire)) return;
    worker.join();
  }
  ready.store(false, std::memory_order_release);
  lv_label_set_text(info_label, "Loading system information...");
  worker = std::thread([this]() {
    const auto response = ManagementClient::system_details();
    const std::string text = response.ok ? render(response.body) : response.error;
    {
      std::lock_guard<std::mutex> guard(result_mutex);
      result_text = text;
    }
    ready.store(true, std::memory_order_release);
  });
}

void SystemPanel::poll()
{
  if (!ready.exchange(false, std::memory_order_acq_rel)) return;
  std::string text;
  {
    std::lock_guard<std::mutex> guard(result_mutex);
    text.swap(result_text);
  }
  if (worker.joinable()) worker.join();
  lv_label_set_text(info_label, text.c_str());
}

void SystemPanel::event_callback(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
  auto *self = static_cast<SystemPanel *>(event->user_data);
  lv_obj_t *target = lv_event_get_current_target(event);
  if (target == self->back_btn) lv_obj_move_background(self->cont);
  if (target == self->refresh_btn) self->refresh();
}

void SystemPanel::timer_callback(lv_timer_t *timer)
{
  static_cast<SystemPanel *>(timer->user_data)->poll();
}
