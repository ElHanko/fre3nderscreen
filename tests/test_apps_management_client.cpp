// Targeted Unix transport tests for the native Apps API. Host-only fixture.
#include "../src/management_client.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static ManagementClient::Response respond(const std::string &route,
                                          const std::string &reply,
                                          bool post = false,
                                          const std::string &body = "{\"name\":\"fluidd\"}")
{
  char name[] = "/tmp/fre3nder-apps-client-test-XXXXXX";
  int fd = mkstemp(name);
  assert(fd >= 0);
  close(fd);
  unlink(name);
  int listener = socket(AF_UNIX, SOCK_STREAM, 0);
  assert(listener >= 0);
  sockaddr_un address{};
  address.sun_family = AF_UNIX;
  std::strcpy(address.sun_path, name);
  assert(bind(listener, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == 0);
  assert(listen(listener, 1) == 0);
  std::thread server([&]() {
    int connection = accept(listener, nullptr, nullptr);
    assert(connection >= 0);
    char request[2048];
    ssize_t bytes = recv(connection, request, sizeof(request), 0);
    assert(bytes > 0);
    std::string content(request, bytes);
    while (content.find("\r\n\r\n") == std::string::npos ||
           (post && content.size() < content.find("\r\n\r\n") + 4 + body.size())) {
      bytes = recv(connection, request, sizeof(request), 0);
      assert(bytes > 0);
      content.append(request, bytes);
    }
    assert(content.find((post ? "POST " : "GET ") + route + " HTTP/1.0") != std::string::npos);
    if (post) {
      assert(content.find("Content-Type: application/json") != std::string::npos);
      assert(content.find("Content-Length:") != std::string::npos);
      assert(content.substr(content.find("\r\n\r\n") + 4) == body);
    }
    size_t written = 0;
    while (written < reply.size()) {
      ssize_t count = send(connection, reply.data() + written, reply.size() - written, 0);
      assert(count > 0);
      written += static_cast<size_t>(count);
    }
    close(connection);
  });
  ManagementClient::Response result;
  if (route.find("detail?") != std::string::npos)
    result = ManagementClient::app_detail("fluidd", name);
  else if (post)
    result = ManagementClient::apps_action(route.substr(std::strlen("/fre3nder/api/v1/apps/")), body, name);
  else
    result = ManagementClient::apps_status(name);
  server.join();
  close(listener);
  unlink(name);
  return result;
}

int main()
{
  auto status = respond("/fre3nder/api/v1/apps/status",
      "HTTP/1.0 200 OK\r\nContent-Type: application/json\r\n\r\n{\"ok\":true}");
  assert(status.ok && status.body == "{\"ok\":true}");
  auto detail = respond("/fre3nder/api/v1/apps/detail?name=fluidd",
      "HTTP/1.0 200 OK\r\n\r\n{\"operation\":\"apps-detail\"}");
  assert(detail.ok);
  auto mutation = respond("/fre3nder/api/v1/apps/remove",
      "HTTP/1.0 202 Accepted\r\n\r\n{\"operation\":\"apps-accepted\"}", true);
  assert(mutation.ok && mutation.status == 202);
  auto download = respond("/fre3nder/api/v1/apps/repository/download",
      "HTTP/1.0 202 Accepted\r\n\r\n{\"operation\":\"apps-accepted\"}", true,
      "{\"name\":\"fluidd\",\"release_serial\":2}");
  assert(download.ok && download.status == 202);
  auto denied = respond("/fre3nder/api/v1/apps/status",
      "HTTP/1.0 403 Forbidden\r\n\r\n{}");
  assert(!denied.ok && denied.status == 403 && denied.error == "Management access denied");
  assert(!ManagementClient::app_detail("../unsafe").ok);
  assert(!ManagementClient::apps_action("arbitrary", "{}").ok);
  return 0;
}
