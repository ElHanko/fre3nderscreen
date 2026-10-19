// Host-only transport checks for the existing Fre3nder OTA HTTP/Unix API.
#include "../src/management_client.h"
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <string>
#include <thread>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static ManagementClient::Response respond(const std::string &route,
                                          const std::string &reply, bool post)
{
  char name[] = "/tmp/fre3nder-ota-client-XXXXXX";
  int file = mkstemp(name);
  assert(file >= 0);
  close(file);
  unlink(name);
  int listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  assert(listen_fd >= 0);
  sockaddr_un address{};
  address.sun_family = AF_UNIX;
  std::strcpy(address.sun_path, name);
  assert(bind(listen_fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == 0);
  assert(listen(listen_fd, 1) == 0);
  std::thread server([&] {
    int connection = accept(listen_fd, nullptr, nullptr);
    assert(connection >= 0);
    char buf[2048];
    ssize_t n = recv(connection, buf, sizeof(buf), 0);
    assert(n > 0);
    std::string request(buf, n);
    assert(request.find((post ? "POST " : "GET ") + route + " HTTP/1.0") != std::string::npos);
    if (post) assert(request.find("Content-Type: application/json") != std::string::npos);
    assert(send(connection, reply.data(), reply.size(), MSG_NOSIGNAL) == (ssize_t)reply.size());
    close(connection);
  });
  ManagementClient::Response answer = post ?
      ManagementClient::ota_action("backup-plan",
          "{\"package_id\":\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\",\"home\":\"usb\",\"sys\":\"home\"}", name)
      : ManagementClient::ota_status(name);
  server.join();
  close(listen_fd);
  unlink(name);
  return answer;
}

int main()
{
  auto status = respond("/fre3nder/api/v1/ota/status",
      "HTTP/1.0 200 OK\r\n\r\n{\"ok\":true}", false);
  assert(status.ok && status.body == "{\"ok\":true}");
  auto accepted = respond("/fre3nder/api/v1/ota/backup-plan",
      "HTTP/1.0 202 Accepted\r\n\r\n{\"operation\":\"ota-accepted\"}", true);
  assert(accepted.ok && accepted.status == 202);
  assert(!ManagementClient::ota_action("raw-write", "{}").ok);
  assert(!ManagementClient::ota_action("write", "").ok);
  assert(!ManagementClient::upload_ota("/etc/passwd").ok);
  return 0;
}
