#include "network/ws.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include "network/sockets.hpp"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using boost_tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


namespace viperfish::network {

    BoostWsClient::BoostWsClient(
            const std::string &host,
            int port,
            const std::string &path,
            const std::function<void(const std::string &)> callback_func
    )
            : host(host)
            , port(port)
            , path(path)
            , callback_func(callback_func)
    {}

    BoostWsClient::~BoostWsClient() {
        finish();
        std::lock_guard lock(finish_mutex);
    }

    void
    BoostWsClient::run() {
        auto ips = hostname2ips_list(host.c_str());
        for (int i_retry = 0; i_retry < 5; ++i_retry) {
            std::lock_guard lock(finish_mutex);
            try {
                net::io_context ioc;
                //ssl::context ctx{ssl::context::tlsv11_client};
                ssl::context ctx{ssl::context::tls_client};
                boost_tcp::resolver resolver{ioc};
                auto wss = websocket::stream<beast::ssl_stream<boost_tcp::socket>>{ioc, ctx};
                ws = &wss;

                std::string current_host = host;
                if (i_retry > 0 && !ips.empty()) {
                    current_host = ips[(i_retry - 1) % ips.size()];
                }

                // Look up the domain name
                auto const results = resolver.resolve(current_host, std::to_string(port));

                // Make the connection on the IP address we get from a lookup
                auto ep = net::connect(get_lowest_layer(*ws), results);

                /*
                std::cout
                        << "BoostWsClient: Start listening "
                        << "wss://" << host << ":" << port
                        << path
                        << std::endl;
                */

                if (!SSL_set_tlsext_host_name(ws->next_layer().native_handle(), host.c_str())) {
                    throw beast::system_error(
                            beast::error_code(
                                    static_cast<int>(::ERR_get_error()),
                                    net::error::get_ssl_category()),
                            "Failed to set SNI Hostname");
                }

                try {
                    ws->next_layer().handshake(ssl::stream_base::client);
                    ws->handshake(host + ":" + std::to_string(ep.port()), path);
                }
                catch (std::exception const &e) {
                    std::cout << log_prefix() << "Error on handshake " << current_host << ":" << port << " - " << e.what() << std::endl;
                    if (!need_to_finish) {
                        std::cout << log_prefix() << "Retry: " << i_retry << std::endl;
                        sleep(2);
                        continue;
                    }
                }

                boost::system::error_code ec;
                beast::flat_buffer buffer;
                while (!need_to_finish) {
                    buffer.clear();
                    ws->read(buffer, ec);
                    if (ec) {
                        //std::cout << "Error on reading: " << ec << std::endl;
                        break;
                    }
                    callback_func(boost::beast::buffers_to_string(buffer.data()));
                }

                if (!need_to_finish) {
                    // TODO
                    std::cout << log_prefix() << "Error on reading: " << ec << std::endl;
                    std::cout << log_prefix() << "NEED TO RECONNECT" << std::endl;
                    ws->next_layer().next_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
                    //ws = NULL;
                    //return;
                }

                ws->close(websocket::close_code::normal, ec);
            }
            catch (std::exception const &e) {
                std::cout << log_prefix() << "Error on " << host << ":" << port << " - " << e.what() << std::endl;
            }
            ws = NULL;

            return;
        }
    }

    void
    BoostWsClient::finish()
    {
        need_to_finish = true;
        if (ws != NULL) {
            boost::system::error_code ec;
            ws->next_layer().next_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        }
    }

    void
    BoostWsClient::send(const std::string& data)
    {
        //boost::beast::multi_buffer b;
        //boost::asio::buffer_copy(b.prepare(data.size()), );
        ws->write(boost::asio::buffer(data));
    }

    // HTTP TODO make single

    BoostWsNoSslClient::BoostWsNoSslClient(
            const std::string &host,
            int port,
            const std::string &path,
            const std::function<void(const std::string &)> callback_func
    )
            : host(host)
            , port(port)
            , path(path)
            , callback_func(callback_func)
    {}

    BoostWsNoSslClient::~BoostWsNoSslClient() {
        finish();
        std::lock_guard lock(finish_mutex);
    }

    void
    BoostWsNoSslClient::run() {
        auto ips = hostname2ips_list(host.c_str());
        for (int i_retry = 0; i_retry < 5; ++i_retry) {
            std::lock_guard lock(finish_mutex);
            try {
                net::io_context ioc;
                boost_tcp::resolver resolver{ioc};
                auto wss = websocket::stream<boost_tcp::socket>{ioc};
                ws = &wss;

                std::string current_host = host;
                if (i_retry > 0 && !ips.empty()) {
                    current_host = ips[(i_retry - 1) % ips.size()];
                }

                // Look up the domain name
                auto const results = resolver.resolve(current_host, std::to_string(port));

                // Make the connection on the IP address we get from a lookup
                auto ep = net::connect(ws->next_layer(), results);

                try {
                    ws->handshake(host + ":" + std::to_string(ep.port()), path);
                }
                catch (std::exception const &e) {
                    std::cout << log_prefix() << "Error on handshake " << current_host << ":" << port << " - " << e.what() << std::endl;
                    if (!need_to_finish) {
                        std::cout << log_prefix() << "Retry: " << i_retry << std::endl;
                        sleep(2);
                        continue;
                    }
                }

                for (const auto& pre_send_data: pre_send) {
                    send(pre_send_data);
                }

                boost::system::error_code ec;
                beast::flat_buffer buffer;
                while (!need_to_finish) {
                    buffer.clear();
                    ws->read(buffer, ec);
                    if (ec) {
                        //std::cout << "Error on reading: " << ec << std::endl;
                        break;
                    }
                    callback_func(boost::beast::buffers_to_string(buffer.data()));
                }

                if (!need_to_finish) {
                    // TODO
                    std::cout << log_prefix() << "Error on reading: " << ec << std::endl;
                    std::cout << log_prefix() << "NEED TO RECONNECT" << std::endl;
                    ws->next_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
                    //ws = NULL;
                    //return;
                }

                ws->close(websocket::close_code::normal, ec);
            }
            catch (std::exception const &e) {
                std::cout << log_prefix() << "Error on " << host << ":" << port << " - " << e.what() << std::endl;
            }
            ws = NULL;
            pre_send.clear();

            return;
        }
    }

    void
    BoostWsNoSslClient::finish()
    {
        need_to_finish = true;
        if (ws != NULL) {
            try {
                boost::system::error_code ec;
                ws->close(websocket::close_code::normal, ec);
            }
            catch (std::exception const &e) {
                std::cout << log_prefix() << "Error on finishing" << host << ":" << port << " - " << e.what()
                        << ". Trying to kill" << std::endl;
                try {
                    boost::system::error_code ec;
                    ws->next_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
                }
                catch (std::exception const &e) {
                    std::cout << log_prefix() << "Error on killing" << host << ":" << port << " - " << e.what() << std::endl;
                }
            }
        }
    }

    void
    BoostWsNoSslClient::send(const std::string& data)
    {
        //boost::beast::multi_buffer b;
        //boost::asio::buffer_copy(b.prepare(data.size()), );
        if (ws == NULL) {
            pre_send.push_back(data);
            return;
        }
        ws->write(net::buffer(data));
    }
}