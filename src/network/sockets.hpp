#ifndef VIPERFISH_SOCKETS_HPP
#define VIPERFISH_SOCKETS_HPP
#include <sys/un.h>
#include <string>
#include <vector>
#include <optional>
#include <thread>

namespace viperfish::network {

    std::vector<std::string> hostname2ips_list(const char* hostname);

    std::string hostname2ip(const char* hostname);

    int create_and_connect_tcp_socket(
            const std::string& ip,
            int port = 443,
            const std::optional<std::string>& network_interface = {}
    );

    void set_socket_timeout(int sock, double timeout);
}
#endif 
