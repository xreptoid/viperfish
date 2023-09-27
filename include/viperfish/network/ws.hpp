#ifndef VIPERFISH_WS_HPP
#define VIPERFISH_WS_HPP

#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>

#include <string>
#include <functional>
#include <thread>
#include <mutex>

#include "viperfish/utils/name_mixture.hpp"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using boost_tcp = boost::asio::ip::tcp;


namespace viperfish::network {

    /*

    class WsppWsClient {
    public:

        const std::string host;
        const int port;
        const std::string path;
        const std::function<void(const std::string&)> callback_func;

        WsppWsClient(
                const std::string& host,
                int port,
                const std::string& path,
                std::function<void(const std::string&)> callback_func
        );

        void run();

        void finish() {
            need_to_finish = true;
        }

    private:

        volatile bool need_to_finish = false;
    };

     */

    class BoostWsClient : public utils::NameMixture {
    public:

        const std::string host;
        const int port;
        const std::string path;
        const std::function<void(const std::string&)> callback_func;

        BoostWsClient(
                const std::string& host,
                int port,
                const std::string& path,
                std::function<void(const std::string&)> callback_func
        );

        virtual
        ~BoostWsClient();

        std::string _get_app_name() const override { return "ws"; }
        void run();
        void finish();
        void send(const std::string& data);

    private:

        websocket::stream<beast::ssl_stream<boost_tcp::socket>>* ws = NULL;
        volatile bool need_to_finish = false;
        std::mutex finish_mutex;
    };

    class BoostWsNoSslClient : public utils::NameMixture {
    public:

        const std::string host;
        const int port;
        const std::string path;
        const std::function<void(const std::string&)> callback_func;

        BoostWsNoSslClient(
                const std::string& host,
                int port,
                const std::string& path,
                std::function<void(const std::string&)> callback_func
        );

        virtual
        ~BoostWsNoSslClient();

        std::string _get_app_name() const override { return "ws"; }
        void run();
        void finish();
        void send(const std::string& data);

    private:
        websocket::stream<boost_tcp::socket>* ws = NULL;
        volatile bool need_to_finish = false;
        std::mutex finish_mutex;
        std::vector<std::string> pre_send;
    };

    //typedef WsppWsClient WsClient;
    typedef BoostWsClient WsClient;
    //typedef PureWsClient WsClient;
}

#endif 
