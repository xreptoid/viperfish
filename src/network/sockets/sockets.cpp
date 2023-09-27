#include "viperfish/network/sockets.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <vector>
#include <iostream>
#include <unordered_map>
#include <cmath>

namespace viperfish::network {

    std::unordered_map<std::string, std::vector<std::string> > _hostname2ips_list_cache;
    std::unordered_map<std::string, std::string> _hostname2ip_cache;

    std::vector<std::string> hostname2ips_list(const char* hostname) {
        std::string hostname_s(hostname);

        if (_hostname2ips_list_cache.find(hostname_s) != _hostname2ips_list_cache.end()) {
            return _hostname2ips_list_cache[hostname_s];
        }

        struct hostent* he;
        struct in_addr** addr_list;

        if ( (he = gethostbyname( hostname ) ) == NULL)
        {
            // get the host info
            herror("gethostbyname");
            return {}; // FIXME exception
        }

        addr_list = (struct in_addr**) he->h_addr_list;

        std::vector<std::string> result;
        for (int i = 0; addr_list[i] != NULL; ++i)
        {
            result.push_back(std::string(inet_ntoa(*addr_list[i])));
        }

        return _hostname2ips_list_cache[hostname_s] = result;
    }


    std::string hostname2ip(const char* hostname) {
        std::string hostname_s(hostname);

        if (_hostname2ip_cache.find(hostname_s) != _hostname2ip_cache.end()) {
            return _hostname2ip_cache[hostname_s];
        }

        return _hostname2ip_cache[hostname_s] = hostname2ips_list(hostname)[0];
    }

    int create_and_connect_tcp_socket(
            const std::string& ip,
            int port,
            const std::optional<std::string>& network_interface
    ) {
        int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (s < 0) {
            std::cout << "Error creating socket (err=" << errno << ", " << strerror(errno) << ")" << std::endl;
            return -1;
        }

        if (network_interface.has_value()) {
#if __linux__
            /*
            struct ifreq ifr;

            memset(&ifr, 0, sizeof(ifr));
            snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), (*network_interface).c_str());
             */
            //if (setsockopt(s, SOL_SOCKET, SO_BINDTODEVICE, (void*) &ifr, sizeof(ifr)) < 0) {
            if (
                    setsockopt(
                            s, SOL_SOCKET, SO_BINDTODEVICE,
                            (*network_interface).c_str(), (*network_interface).size()
                    ) < 0
            ) {
                std::cout << "Error binding network interface '" << *network_interface << "'" << std::endl;
            }
#elif __APPLE__
            int ni_index = if_nametoindex((*network_interface).c_str());
            if (ni_index == 0) {
                std::cout << "Error (not found) binding network interface '" << *network_interface << "'" << std::endl;
            }

            if (setsockopt(s, IPPROTO_IP, IP_BOUND_IF, &ni_index, sizeof(ni_index)) == -1) {
                std::cout << "Error (setsockopt) binding network interface '" << *network_interface << "'" << std::endl;
            }
#endif
        }

        struct sockaddr_in sa;
        memset (&sa, 0, sizeof(sa));
        sa.sin_family      = AF_INET;
        sa.sin_addr.s_addr = inet_addr(ip.c_str());
        sa.sin_port        = htons (port);
        socklen_t socklen = sizeof(sa);

        auto connect_ret = connect(s, (struct sockaddr *)&sa, socklen);
        if (connect_ret) {
            std::cout << "Error connecting to server (err=" << errno << ", " << strerror(errno) << ")" << " " << ip << std::endl;
            return -1;
        }

        return s;
    }

    void set_socket_timeout(int sock, double timeout) {
        struct timeval tv;
        tv.tv_sec = std::floor(timeout);
        tv.tv_usec = std::ceil((timeout - tv.tv_sec) * 1000 * 1000);
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    }
}
