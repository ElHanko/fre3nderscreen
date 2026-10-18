// Targeted host transport test, built only with explicit authorization.
#include "../src/management_client.h"

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static ManagementClient::Response respond(const std::string &reply)
{
  char name[] = "/tmp/fre3nder-management-test-XXXXXX";
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
    char request[512];
    ssize_t size = recv(connection, request, sizeof(request), 0);
    assert(size > 0);
    assert(std::string(request, size).find("GET /fre3nder/api/v1/system/details") != std::string::npos);
    size_t offset = 0;
    while (offset < reply.size()) {
      ssize_t sent = send(connection, reply.data() + offset, reply.size() - offset, 0);
      assert(sent > 0);
      offset += static_cast<size_t>(sent);
    }
    close(connection);
  });
  auto result = ManagementClient::system_details(name);
  server.join();
  close(listener);
  unlink(name);
  return result;
}

int main()
{
  auto ok = respond("HTTP/1.0 200 OK\r\nContent-Type: application/json\r\n\r\n{\"ok\":true}");
  assert(ok.ok && ok.body == "{\"ok\":true}");
  auto denied = respond("HTTP/1.0 403 Forbidden\r\n\r\n{}");
  assert(!denied.ok && denied.error == "Management access denied");
  auto malformed = respond("not an HTTP response\n");
  assert(!malformed.ok);
  return 0;
}
