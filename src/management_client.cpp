#include "management_client.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/un.h>
#include <unistd.h>

namespace {
constexpr size_t MAX_RESPONSE = 512 * 1024;
constexpr off_t MAX_APP_UPLOAD = 256LL * 1024 * 1024;
constexpr off_t MAX_OTA_UPLOAD = 96LL * 1024 * 1024;
constexpr const char *PREFIX = "/fre3nder/api/v1/apps/";
constexpr const char *OTA_PREFIX = "/fre3nder/api/v1/ota/";

struct Fd {
  int value = -1;
  explicit Fd(int fd) : value(fd) {}
  ~Fd() { if (value >= 0) close(value); }
  Fd(const Fd &) = delete;
  Fd &operator=(const Fd &) = delete;
};

ManagementClient::Response failure(const char *message)
{
  return {false, {}, message, 0};
}

bool send_all(int fd, const char *data, size_t size)
{
  size_t sent = 0;
  while (sent < size) {
    ssize_t count = send(fd, data + sent, size - sent, MSG_NOSIGNAL);
    if (count < 0 && errno == EINTR) continue;
    if (count <= 0) return false;
    sent += static_cast<size_t>(count);
  }
  return true;
}

bool valid_app_name(const std::string &name)
{
  if (name.empty() || name.size() > 64 || name[0] < 'a' || name[0] > 'z') return false;
  for (char c : name)
    if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_')) return false;
  return true;
}

bool valid_filename(const std::string &name, const std::string &suffix)
{
  if (name.size() <= suffix.size() || name.size() > 128 ||
      name.compare(name.size() - suffix.size(), suffix.size(), suffix) != 0) return false;
  for (char c : name)
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
          (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.')) return false;
  return true;
}
}

ManagementClient::Response ManagementClient::system_details(const std::string &socket_path)
{
  return request("GET", "/fre3nder/api/v1/system/details", {}, {}, socket_path);
}

ManagementClient::Response ManagementClient::apps_status(const std::string &socket_path)
{
  return request("GET", std::string(PREFIX) + "status", {}, {}, socket_path);
}

ManagementClient::Response ManagementClient::app_detail(const std::string &name, const std::string &socket_path)
{
  if (!valid_app_name(name)) return failure("Invalid application name");
  return request("GET", std::string(PREFIX) + "detail?name=" + name, {}, {}, socket_path);
}

ManagementClient::Response ManagementClient::apps_action(const std::string &action,
    const std::string &json_body, const std::string &socket_path)
{
  // Explicitly enumerate the supported API, rather than accepting arbitrary routes.
  if (action != "verify" && action != "install" && action != "update" &&
      action != "remove" && action != "display/select" &&
      action != "display/disable" && action != "discard" &&
      action != "acknowledge" && action != "repository/download")
    return failure("Unsupported application operation");
  if (json_body.empty() || json_body.size() > 256)
    return failure("Invalid application operation body");
  return request("POST", std::string(PREFIX) + action, json_body, {}, socket_path);
}

ManagementClient::Response ManagementClient::upload_app(const std::string &absolute_path,
    const std::string &socket_path)
{
  return request("POST", std::string(PREFIX) + "upload", {}, absolute_path, socket_path);
}

ManagementClient::Response ManagementClient::ota_status(const std::string &socket_path)
{
  return request("GET", std::string(OTA_PREFIX) + "status", {}, {}, socket_path);
}

ManagementClient::Response ManagementClient::ota_action(const std::string &action,
    const std::string &json_body, const std::string &socket_path)
{
  if (action != "verify" && action != "preflight" && action != "backup-plan" &&
      action != "backup" && action != "confirm" && action != "write" &&
      action != "readback" && action != "prepare-activation" && action != "reboot")
    return failure("Unsupported OTA operation");
  if (json_body.empty() || json_body.size() > 256)
    return failure("Invalid OTA operation body");
  return request("POST", std::string(OTA_PREFIX) + action, json_body, {}, socket_path);
}

ManagementClient::Response ManagementClient::upload_ota(const std::string &absolute_path,
    const std::string &socket_path)
{
  return request("POST", std::string(OTA_PREFIX) + "upload", {}, absolute_path, socket_path);
}

ManagementClient::Response ManagementClient::request(const std::string &method,
    const std::string &route, const std::string &json_body,
    const std::string &upload_path, const std::string &path)
{
#ifdef SIMULATOR
  const char *override_path = std::getenv("FRE3NDERSCREEN_MANAGEMENT_SOCKET");
  const std::string socket_path = override_path && *override_path ? override_path : path;
#else
  const std::string &socket_path = path;
#endif
  if (socket_path.empty() || socket_path.size() >= sizeof(((sockaddr_un *)nullptr)->sun_path))
    return failure("Invalid Management socket path");

  Fd upload(-1);
  off_t upload_size = 0;
  std::string filename;
  if (!upload_path.empty()) {
    // Fre3nder's USB service mounts exactly one validated runtime volume here.
    // Never accept a caller-selected path outside the mountpoint.
#ifdef SIMULATOR
    const char *usb_env = std::getenv("FRE3NDERSCREEN_USB_ROOT");
    const std::string usb_root = usb_env && *usb_env ? usb_env : "/run/fre3nder/usb";
#else
    const std::string usb_root = "/run/fre3nder/usb";
#endif
    if (usb_root.empty() || usb_root[usb_root.size() - 1] == '/' ||
        upload_path.compare(0, usb_root.size() + 1, usb_root + "/") != 0)
      return failure("Application must come from the USB mount");
    filename = upload_path.substr(usb_root.size() + 1);
    const bool ota_upload = route == std::string(OTA_PREFIX) + "upload";
    const bool app_upload = route == std::string(PREFIX) + "upload";
    if ((!ota_upload && !app_upload) ||
        !valid_filename(filename, ota_upload ? ".ota" : ".fre3app"))
      return failure("Invalid USB package filename");
    Fd dir(open(usb_root.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW));
    if (dir.value < 0) return failure("USB mount unavailable");
    upload.value = openat(dir.value, filename.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (upload.value < 0) return failure("USB package unavailable");
    struct stat metadata {};
    if (fstat(upload.value, &metadata) != 0 || !S_ISREG(metadata.st_mode) ||
        metadata.st_size <= 0 ||
        metadata.st_size > (ota_upload ? MAX_OTA_UPLOAD : MAX_APP_UPLOAD))
      return failure("Invalid USB package or size");
    upload_size = metadata.st_size;
  }

  Fd fd(socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0));
  if (fd.value < 0) return failure("Cannot create Management socket");
  timeval timeout{};
  timeout.tv_sec = 20;
  if (setsockopt(fd.value, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) != 0 ||
      setsockopt(fd.value, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) != 0)
    return failure("Cannot configure Management timeout");

  sockaddr_un address{};
  address.sun_family = AF_UNIX;
  std::memcpy(address.sun_path, socket_path.c_str(), socket_path.size() + 1);
  if (connect(fd.value, reinterpret_cast<sockaddr *>(&address), sizeof(address)) != 0)
    return failure("Management service unavailable");

  std::ostringstream request_header;
  request_header << method << ' ' << route << " HTTP/1.0\r\nHost: localhost\r\nConnection: close\r\n";
  if (!upload_path.empty()) {
    request_header << "Content-Type: application/octet-stream\r\n"
                   << "X-Fre3nder-Filename: " << filename << "\r\n"
                   << "Content-Length: " << upload_size << "\r\n";
  } else if (method == "POST") {
    request_header << "Content-Type: application/json\r\n"
                   << "Content-Length: " << json_body.size() << "\r\n";
  }
  request_header << "\r\n";
  const auto header = request_header.str();
  if (!send_all(fd.value, header.data(), header.size()))
    return failure("Management request failed");
  if (!upload_path.empty()) {
    char buffer[65536];
    off_t remaining = upload_size;
    while (remaining > 0) {
      ssize_t count = read(upload.value, buffer, static_cast<size_t>(
          remaining < static_cast<off_t>(sizeof(buffer)) ? remaining : sizeof(buffer)));
      if (count < 0 && errno == EINTR) continue;
      if (count <= 0 || !send_all(fd.value, buffer, static_cast<size_t>(count)))
        return failure("USB package upload failed");
      remaining -= count;
    }
  } else if (!json_body.empty() && !send_all(fd.value, json_body.data(), json_body.size())) {
    return failure("Management request failed");
  }

  std::string result;
  char buffer[4096];
  while (true) {
    ssize_t count = recv(fd.value, buffer, sizeof(buffer), 0);
    if (count < 0 && errno == EINTR) continue;
    if (count < 0) return failure("Management response timed out");
    if (count == 0) break;
    if (result.size() + static_cast<size_t>(count) > MAX_RESPONSE)
      return failure("Management response exceeds limit");
    result.append(buffer, static_cast<size_t>(count));
  }
  const size_t header_end = result.find("\r\n\r\n");
  const size_t line_end = result.find("\r\n");
  if (header_end == std::string::npos || line_end == std::string::npos ||
      line_end > header_end)
    return failure("Invalid Management HTTP response");
  std::istringstream status_line(result.substr(0, line_end));
  std::string protocol;
  int status = 0;
  status_line >> protocol >> status;
  if ((protocol != "HTTP/1.0" && protocol != "HTTP/1.1") || status < 100 || status > 599)
    return failure("Invalid Management HTTP status");
  const std::string body = result.substr(header_end + 4);
  if (status < 200 || status >= 300) {
    if (status == 401 || status == 403)
      return {false, body, "Management access denied", status};
    return {false, body, "Management request rejected", status};
  }
  return {true, body, {}, status};
}
