// Host LVGL/API fixture. Compilation/execution requires explicit build approval.
#include "../src/timezone_panel.h"

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <thread>
#include <vector>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace {
std::vector<lv_obj_t *> find(lv_obj_t *root, const lv_obj_class_t *type)
{
  std::vector<lv_obj_t *> result;
  if (lv_obj_check_type(root, type)) result.push_back(root);
  for (uint32_t i = 0; i < lv_obj_get_child_count(root); ++i) {
    auto children = find(lv_obj_get_child(root, i), type);
    result.insert(result.end(), children.begin(), children.end());
  }
  return result;
}

lv_obj_t *button(lv_obj_t *root, const char *text)
{
  for (auto *label : find(root, &lv_label_class))
    if (std::strcmp(lv_label_get_text(label), text) == 0) return lv_obj_get_parent(label);
  assert(false);
  return nullptr;
}

bool contains(lv_obj_t *root, const char *text)
{
  for (auto *label : find(root, &lv_label_class))
    if (std::strstr(lv_label_get_text(label), text)) return true;
  return false;
}

void until(const std::function<bool()> &condition)
{
  for (int i = 0; i < 300; ++i) {
    lv_tick_inc(20);
    lv_timer_handler();
    if (condition()) return;
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  assert(condition());
}
} // namespace

int main()
{
  char path[] = "/tmp/fre3nder-timezone-ui-XXXXXX";
  int temporary = mkstemp(path);
  assert(temporary >= 0);
  close(temporary);
  unlink(path);
  int listener = socket(AF_UNIX, SOCK_STREAM, 0);
  assert(listener >= 0);
  sockaddr_un address{};
  address.sun_family = AF_UNIX;
  std::strcpy(address.sun_path, path);
  assert(bind(listener, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == 0);
  assert(listen(listener, 1) == 0);
  setenv("FRE3NDERSCREEN_MANAGEMENT_SOCKET", path, 1);
  std::vector<std::string> requests;
  const std::vector<std::string> responses = {
    R"({"api_version":1,"ok":true,"operation":"system-timezone","timezone":{"active":"Etc/UTC","configured":"Europe/Berlin","reboot_required":true,"available":["America/Indiana/Indianapolis","Etc/UTC","Europe/Berlin"]}})",
    R"({"api_version":1,"ok":true,"operation":"system-timezone","timezone":{"active":"Etc/UTC","configured":"America/Indiana/Indianapolis","reboot_required":true}})",
    R"({"api_version":1,"ok":false,"error":{"code":"not-found"}})",
  };
  std::thread server([&]() {
    for (size_t index = 0; index < responses.size(); ++index) {
      int connection = accept(listener, nullptr, nullptr);
      assert(connection >= 0);
      std::string request;
      char buffer[4096];
      while (true) {
        ssize_t count = recv(connection, buffer, sizeof(buffer), 0);
        assert(count > 0);
        request.append(buffer, count);
        auto end = request.find("\r\n\r\n");
        if (end == std::string::npos) continue;
        auto length = request.find("Content-Length: ");
        if (length == std::string::npos || request.size() >= end + 4 + std::stoul(request.substr(length + 16))) break;
      }
      requests.push_back(request);
      const auto reply = std::string(index == 2 ? "HTTP/1.0 404 Not Found" : "HTTP/1.0 200 OK") +
          "\r\nContent-Type: application/json\r\n\r\n" + responses[index];
      size_t sent = 0;
      while (sent < reply.size()) {
        auto count = send(connection, reply.data() + sent, reply.size() - sent, MSG_NOSIGNAL);
        assert(count > 0);
        sent += count;
      }
      close(connection);
    }
  });

  lv_init();
  auto *display = lv_display_create(272, 480);
  static uint8_t pixels[272 * 32 * 4];
  lv_display_set_buffers(display, pixels, nullptr, sizeof(pixels), LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(display, [](lv_display_t *disp, const lv_area_t *, uint8_t *) { lv_display_flush_ready(disp); });
  {
    TimezonePanel panel;
    panel.foreground();
    auto *root = lv_obj_get_child(lv_screen_active(), -1);
    auto selects = find(root, &lv_dropdown_class);
    assert(selects.size() == 2);
    auto *save = button(root, "Save timezone");
    until([&]() { return !lv_obj_has_state(save, LV_STATE_DISABLED); });
    assert(lv_dropdown_get_selected(selects[0]) == 2);
    assert(std::string(lv_dropdown_get_options(selects[1])) == "Berlin");
    lv_dropdown_set_selected(selects[0], 0);
    lv_obj_send_event(selects[0], LV_EVENT_VALUE_CHANGED, nullptr);
    assert(std::string(lv_dropdown_get_options(selects[1])) == "Indiana/Indianapolis");
    lv_obj_send_event(save, LV_EVENT_CLICKED, nullptr);
    until([&]() { return contains(root, "Timezone saved."); });
    assert(contains(root, "Active: Etc/UTC") && contains(root, "Restart required: Yes"));
    assert(contains(root, "Core has no safe general restart API"));
    panel.foreground(); // Reload against an older Core response.
    until([&]() { return contains(root, "does not support timezone settings"); });
    assert(lv_obj_has_state(save, LV_STATE_DISABLED));
    assert(lv_dropdown_get_option_count(selects[0]) == 0);
  }
  server.join();
  assert(requests.size() == 3);
  assert(requests[0].find("GET /fre3nder/api/v1/system/timezone ") == 0);
  assert(requests[1].find("POST /fre3nder/api/v1/system/timezone ") == 0);
  assert(requests[1].find(R"({"timezone":"America/Indiana/Indianapolis"})") != std::string::npos);
  close(listener);
  unlink(path);
  lv_display_delete(display);
  lv_deinit();
}
