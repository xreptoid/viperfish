#include "network/ws_server.hpp"
#include "network/ws_server_body.hpp"

namespace viperfish::network {

    WsServer::~WsServer() {
    }

    void WsServer::run() {
        auto const address = net::ip::make_address("0.0.0.0");
        // The io_context is required for all I/O
        net::io_context ioc{n_threads};

        auto body = std::make_shared<WsServerBody>(ioc, boost_tcp::endpoint{address, static_cast<unsigned short>(port)});
        body->set_on_read_callback(on_read_callback);
        body->run();
        send_all_callback = [&body](const std::string& data) { body->send_all(data); };

        std::vector<std::thread> v;
        v.reserve(n_threads - 1);
        for (int i = 0; i < n_threads - 1; ++i) {
            v.emplace_back([&ioc]{ ioc.run(); });
        }
        ioc.run();
    }
}