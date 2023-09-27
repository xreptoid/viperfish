#include "binance/consumer.hpp"
#include <thread>
#include <chrono>
#include <mutex>
#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <boost/algorithm/string.hpp>
#include "json.hpp"
#include "network.hpp"
#include "timestamp.hpp"

namespace viperfish::binance {

    BinanceConsumer::BinanceConsumer(
            const std::vector<std::string>& symbols,
            const std::vector<std::string>& hosts
    )
        : symbols(symbols)
        , hosts(hosts)
    {
        for (auto& symbol: this->symbols) {
            boost::algorithm::to_lower(symbol);
        }
    }

    BinanceConsumer::~BinanceConsumer() {
        finish();
    }

    void BinanceConsumer::on_event(const std::string& data, const json& obj, const json& obj_data) {
        std::stringstream key_ss;
        std::string stream = json_field_get<std::string>(obj, "stream", "");
        std::string key;
        if (stream.size() > 25) {
            // user stream
            stream = "u";
            key_ss << stream << '.';
            key_ss << json_field_get<std::string>(obj_data, "e", "") << '.';
            key_ss << json_field_get<std::string>(obj_data, "s", "") << '.';
            key_ss << json_field_get<std::uint64_t>(obj_data, "u", 0) << '.';
            key_ss << json_field_get<std::uint64_t>(obj_data, "T", 0) << '.';
            key_ss << json_field_get<std::uint64_t>(obj_data, "E", 0) << '.';
            key = key_ss.str();
        } else {
            // trades, obt, ...
            key = data;
        }

        {
            std::lock_guard lock(on_event_mutex);
            if (processed_events_keys.find(key) != processed_events_keys.end()) {
                return;
            }
            processed_events_keys.insert(key);
            auto lts = get_current_ts();
            processed_events_queue.push_back(std::make_pair(key, lts));
            while (processed_events_queue.begin()->second < lts - 10 * 1000) {
                processed_events_keys.erase(processed_events_queue.begin()->first);
                processed_events_queue.pop_front();
            }
        }
        for (const auto cb: settings.on_event_callbacks) {
            cb(obj);
        }
    }

    bool BinanceConsumer::accept_callback_for_catched_fill(long long trade_id) {
        std::lock_guard<std::mutex> lock(catched_fill_mutex);
        if (catched_trades.find(trade_id) != catched_trades.end()) {
            return false;
        }
        catched_trades.insert(trade_id);
        return true;
    }

    void BinanceConsumer::connection_fill_callback(long long trade_id, const std::string& data) {
        if (!accept_callback_for_catched_fill(trade_id)) {
            return;
        }
        on_fill_callback(data);
    }

    bool BinanceConsumer::accept_callback_for_catched_order_full_fill(long long order_id) {
        std::lock_guard<std::mutex> lock(catched_order_full_fill_mutex);
        if (catched_order_full_fills.find(order_id) != catched_order_full_fills.end()) {
            return false;
        }
        catched_order_full_fills.insert(order_id);
        return true;
    }

    void BinanceConsumer::connection_order_full_fill_callback(
            long long order_id,
            const std::string& data
    )
    {
        if (!accept_callback_for_catched_order_full_fill(order_id)) {
            return;
        }
        on_full_fill_callback(order_id, data);
    }


    bool BinanceConsumer::accept_callback_for_catched_order_cancel(long long order_id) {
        std::lock_guard<std::mutex> lock(catched_order_cancel_mutex);
        if (catched_order_cancels.find(order_id) != catched_order_cancels.end()) {
            return false;
        }
        catched_order_cancels.insert(order_id);
        return true;
    }

    void BinanceConsumer::connection_order_cancel_callback(long long order_id, const std::string& data) {
        if (!accept_callback_for_catched_order_cancel(order_id)) {
            return;
        }

        for (auto& callback: on_cancel_callbacks) {
            callback(order_id, data);
        }
    }

    void BinanceConsumer::connection_ticker_callback(const std::string& data) {
        on_ticker_callback(data);
    }

    void BinanceConsumer::run() {
        auto hosts = get_new_hosts();
        for (int i = 0; i < hosts.size(); ++i) {
            const auto& host = hosts[i];
            connections.push_back(create_connection(i, host, {}));
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
        }

        if (synchronized_tracking_enabled) {
            for (const auto &con: connections) {
                con->enable_tracking();
            }
        }

        initialized = true;

        auto last_reconnect_ts = get_current_ts();

        while (!need_to_finish) {
            std::int64_t period = 5 * 60 * 1000;
            if (get_current_ts() - last_reconnect_ts < period) {
                std::int64_t sleep_for = period - static_cast<std::int64_t>(get_current_ts() - last_reconnect_ts);
                if (sleep_for > 0) {
                    sleep(sleep_for / 1000 + 1);
                }
                continue;
            }
            last_reconnect_ts = get_current_ts();

            reconnect();
        }
        std::cout << log_prefix() << "main running cycle was finished" << std::endl;

        if (synchronized_tracking_enabled) {
            std::cout << log_prefix() << "disabling tracking..." << std::endl;
            for (const auto& con: connections) {
                con->disable_tracking();
            }
            std::cout << log_prefix() << "disabling tracking OK" << std::endl;
        }

        std::cout << log_prefix() << "deleting connections..." << std::endl;
        for (auto& con: connections) {
            if (synchronized_tracking_enabled) {
                con->finish();
            } else {
                delete con;
            }
        }
        std::cout << log_prefix() << "deleting connections OK" << std::endl;

        if (!synchronized_tracking_enabled) {
            connections.clear();
        }

        initialized = false;
        need_to_finish = false;
    }

    void BinanceConsumer::run_async() {
        thread = new std::thread([this]() { this->run(); });
        if (cpu_group.has_value()) {
            cpu_group->attach(*thread);
        }
    }

    void BinanceConsumer::wait_for_initializing() {
        while (!initialized) {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
    }

    void BinanceConsumer::finish() {
        need_to_finish = true;
        if (thread != NULL) {
            thread->join();
            delete thread;
            thread = NULL;
        }
    }

    std::vector<std::string> BinanceConsumer::get_new_hosts() {
        if (!hosts.empty()) {
            return hosts;
        }
        std::vector<std::string> res_hosts;

        auto dns_hosts = network::hostname2ips_list(hostname.c_str());
        if (dns_hosts_max_count.has_value() && dns_hosts.size() > *dns_hosts_max_count) {
            dns_hosts.erase(dns_hosts.begin() + *dns_hosts_max_count, dns_hosts.end());
        }
        res_hosts.insert(res_hosts.end(), dns_hosts.begin(), dns_hosts.end());

        auto fast_hosts = get_fast_hosts();
        res_hosts.insert(res_hosts.end(), fast_hosts.begin(), fast_hosts.end());
        std::cout
            << log_prefix() << res_hosts.size() << " hosts"
            << " (" << dns_hosts.size() << " dns"
            << ", " << fast_hosts.size() << " fast"
            << ") zone = " << zone
            << std::endl;
        return res_hosts;
    }

    std::vector<std::string> BinanceConsumer::get_fast_hosts() {
        return {};
    }

    BinanceConsumerConnection* BinanceConsumer::create_connection(
            int i_con,
            const std::string& host,
            const std::unordered_set<long long>& open_orders
    ) {
        auto con = new BinanceConsumerConnection(
                host,
                port,
                symbols,
                settings,
                settings.on_event_callbacks.empty()
                        ? std::optional<std::function<void(const std::string&, const json&, const json&)>>()
                        : [this](const std::string& data, const json& obj, const json& obj_data) { on_event(data, obj, obj_data); },
                [this](long long trade_id, const std::string& data) {
                    this->connection_fill_callback(trade_id, data);
                },
                [this](long long order_id, const std::string& data) {
                    this->connection_order_full_fill_callback(order_id, data);
                },
                [this](long long order_id, const std::string& data) {
                    this->connection_order_cancel_callback(order_id, data);
                },
                [this](const std::string& data) {
                    this->connection_ticker_callback(data);
                },
                open_orders
        );
        con->set_name((!_name.empty() ? _name + "-" : "") + std::to_string(i_con));
        if (synchronized_tracking_enabled) {
            con->disable_tracking();
        }
        con->run_async();
        return con;
    }

    void BinanceConsumer::reconnect() {
        auto new_hosts = get_new_hosts();
        std::unordered_set<long long> open_orders;
        for (int i = 0; i < new_hosts.size(); ++i) {
            if (i > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(150));
            }
            auto host = new_hosts[i];
            if (i < connections.size()) {
                connections[i]->finish();
                open_orders = connections[i]->open_orders;
            }
            auto con = create_connection(i, host, open_orders);
            if (i < connections.size()) {
                std::this_thread::sleep_for(std::chrono::seconds(5)); // FIXME
                delete connections[i];
                connections[i] = con;
            }
            else {
                connections.push_back(con);
            }
        }

        while (new_hosts.size() < connections.size()) {
            auto con = connections[connections.size() - 1];
            delete con;
            connections.pop_back();
        }
    }


    void BinanceConsumer::set_on_fill_callback(const std::function<void(const std::string&)>& on_fill_callback) {
        this->on_fill_callback = on_fill_callback;
    }

    void BinanceConsumer::set_on_full_fill_callback(const std::function<void(long long, const std::string&)>& on_full_fill_callback) {
        this->on_full_fill_callback = on_full_fill_callback;
    }

    void BinanceConsumer::set_on_cancel_callback(const std::function<void(long long, const std::string&)>& on_cancel_callback) {
        this->on_cancel_callbacks.push_back(on_cancel_callback);
    }

    void BinanceConsumer::set_on_ticker_callback(const std::function<void(const std::string&)>& on_ticker_callback) {
        this->on_ticker_callback = on_ticker_callback;
    }

    void BinanceConsumer::set_mode(BinanceMode mode_value) {
        mode = mode_value;
        if (mode == FUTURES) {
            this->port = 443;
            this->set_hostname("fstream.binance.com");
        }
        if (mode == FUTURES_DELIVERY) {
            this->port = 443;
            this->set_hostname("dstream.binance.com");
        }
    }
}
