#ifndef FRE3NDER_MANAGEMENT_CLIENT_H
#define FRE3NDER_MANAGEMENT_CLIENT_H

#include <string>

// Transport for Fre3nder Core's existing Unix-socket HTTP/JSON API.
// No LVGL, Moonraker, TCP listener, or platform logic lives here.
class ManagementClient {
 public:
  struct Response {
    bool ok = false;
    std::string body;
    std::string error;
    int status = 0;
  };

  static Response system_details(const std::string &socket_path =
      "/run/fre3nder-management/api.sock");
  static Response apps_status(const std::string &socket_path =
      "/run/fre3nder-management/api.sock");
  static Response app_detail(const std::string &name, const std::string &socket_path =
      "/run/fre3nder-management/api.sock");
  static Response apps_action(const std::string &action, const std::string &json_body,
      const std::string &socket_path = "/run/fre3nder-management/api.sock");
  static Response upload_app(const std::string &absolute_path,
      const std::string &socket_path = "/run/fre3nder-management/api.sock");
  static Response ota_status(const std::string &socket_path =
      "/run/fre3nder-management/api.sock");
  static Response ota_action(const std::string &action, const std::string &json_body,
      const std::string &socket_path = "/run/fre3nder-management/api.sock");
  static Response upload_ota(const std::string &absolute_path,
      const std::string &socket_path = "/run/fre3nder-management/api.sock");

 private:
  static Response request(const std::string &method, const std::string &route,
      const std::string &json_body, const std::string &upload_path,
      const std::string &socket_path);
};

#endif
