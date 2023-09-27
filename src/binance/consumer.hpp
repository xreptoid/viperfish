#ifndef VIPERFISH_BINANCE_CONSUMER_HPP
#define VIPERFISH_BINANCE_CONSUMER_HPP
#include <string>
#include <vector>
#include <list>
#include <set>
#include <iostream>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <functional>
#include <optional>

#include "json.hpp"
#include "network/ws.hpp"
#include "./mode.hpp"
#include "market/obt.hpp"
#include "utils/name_mixture.hpp"
#include "performance.hpp"

namespace viperfish::binance {

    class BinanceConsumerThroughSettings {
        friend class BinanceConsumerConnection;
        friend class BinanceConsumer;

    public:
        BinanceConsumerThroughSettings() = default;

        typedef std::function<void(const json&)> on_event_callback_t;
        void add_on_event_callback(const on_event_callback_t& callback) { on_event_callbacks.push_back(callback); }

        typedef std::function<void(const market::OrderBookTop&)> on_obt_callback_t;
        void add_on_obt_callback(const on_obt_callback_t& callback) { on_obt_callbacks.push_back(callback); }

        typedef std::function<void(const json&)> on_ob_diff_callback_t;
        void add_on_ob_diff_callback(const on_ob_diff_callback_t& callback) { on_ob_diff_callbacks.push_back(callback); }

        typedef std::function<void(const json&)> on_ob_snapshot_callback_t;
        void add_on_ob_snapshot_callback(const on_ob_snapshot_callback_t& callback) { on_ob_snapshot_callbacks.push_back(callback); }

        typedef std::function<void(const std::unordered_map<std::string, double>&)> on_balance_callback_t;
        void add_on_balance_callback(const on_balance_callback_t& callback) { on_balance_callbacks.push_back(callback); }

        void consume_trades(bool value = true) { consuming_trades_enabled = value; }
        void consume_agg_trades(bool value = true) { consuming_agg_trades_enabled = value; }
        void consume_tickers(bool value = true) { consuming_tickers_enabled = value; }
        void consume_ob_diff(bool value = true, bool ms100 = true) { consuming_ob_diff_enabled = value; ob_diff_ms100 = ms100; }

    protected:
        std::vector<on_event_callback_t> on_event_callbacks;
        std::vector<on_obt_callback_t> on_obt_callbacks;
        std::vector<on_ob_diff_callback_t> on_ob_diff_callbacks;
        std::vector<on_ob_snapshot_callback_t> on_ob_snapshot_callbacks;
        std::vector<on_balance_callback_t> on_balance_callbacks;

        bool consuming_trades_enabled = true;
        bool consuming_agg_trades_enabled = false;
        bool consuming_tickers_enabled = true;
        bool consuming_ob_diff_enabled = false;
        bool ob_diff_ms100 = true;
    };

    class BinanceFill {
    public:
        std::string symbol;
        std::string type;
        std::string status;
        long double filled_amount;
        long double price;
        long long trade_id;
        long long order_id;
        long long ts;
        long long local_ts;

        BinanceFill(
                const std::string& symbol,
                const std::string& type,
                const std::string& status,
                long double filled_amount,
                long double price,
                long long trade_id,
                long long order_id,
                long long ts,
                long long local_ts
        )
                : symbol(symbol)
                , type(type)
                , status(status)
                , filled_amount(filled_amount)
                , price(price)
                , trade_id(trade_id)
                , order_id(order_id)
                , ts(ts)
                , local_ts(local_ts)
        {}


        static BinanceFill parse(const json&);
    };

    class BinanceConsumerConnection : public utils::NameMixture {
    public:

        std::string host;
        int port;

        std::unordered_set<long long> open_orders;

        BinanceConsumerConnection(
                const std::string& host,
                int port,
                const std::vector<std::string>& symbols,
                const BinanceConsumerThroughSettings& settings,
                const std::optional<std::function<void(const std::string&, const json&, const json&)>>& on_event_callback,
                const std::function<void(long long, const std::string&)>& on_fill_callback,
                const std::function<void(long long, const std::string&)>& on_full_fill_callback,
                const std::function<void(long long, const std::string&)>& on_cancel_callback,
                const std::function<void(const std::string&)>& on_ticker_callback,
                const std::unordered_set<long long>& open_orders
        );

        virtual ~BinanceConsumerConnection();

        void run_async();
        void run();
        void finish();
        std::string _get_app_name() const override { return "binance-consumer-con"; }
        void enable_tracking() { tracking_enabled = true; }
        void disable_tracking() { tracking_enabled = false; }

    private:
        void callback(const std::string& data);
        void process_trade(json obj, const std::string& data);
        void process_ticker(json obj, const std::string& data);
        void process_execution_report(json obj, const std::string& data);
        void process_fill(json obj, const std::string& data);
        void process_new_order(json obj);
        void process_canceled_order(json obj, const std::string& data);

        static std::string get_streams_param(const std::vector<std::string>& streams);
        std::vector<std::string> symbols;
        std::set<std::string> symbols_set;
        std::optional<std::function<void(const std::string&, const json&, const json&)>> on_event_callback;
        std::function<void(long long, const std::string&)> on_fill_callback;
        std::function<void(long long, const std::string&)> on_full_fill_callback;
        std::function<void(long long, const std::string&)> on_cancel_callback;
        std::function<void(const std::string&)> on_ticker_callback;

        network::WsClient* ws_client;
        std::thread* thread = NULL;

        volatile bool tracking_enabled = false;

        BinanceConsumerThroughSettings settings;
    };

    class BinanceConsumer : public utils::NameMixture {
    public:

        std::vector<BinanceConsumerConnection*> connections;
        BinanceConsumerThroughSettings settings;

        BinanceConsumer(
                const std::vector<std::string>& symbols,
                const std::vector<std::string>& hosts = {}
        );
        
        virtual ~BinanceConsumer();

        void run();
        void run_async();
        void wait_for_initializing();
        void finish();

        void set_cpu_group(const std::optional<performance::CpuGroup>& cpu_group) { this->cpu_group = cpu_group; }
        std::string _get_app_name() const override { return "binance-consumer"; }
        void enable_synchronized_tracking() {synchronized_tracking_enabled = true;}
        void set_on_fill_callback(const std::function<void(const std::string&)>& on_fill_callback);
        void set_on_full_fill_callback(const std::function<void(long long, const std::string&)>&);
        void set_on_cancel_callback(const std::function<void(long long, const std::string&)>&);
        void set_on_ticker_callback(const std::function<void(const std::string&)>&);
        void set_hostname(const std::string& hostname) { this->hostname = hostname; }
        void set_mode(BinanceMode mode_value);
        void set_port(int port) { this->port = port; }
        void set_zone(const std::string& zone) { this->zone = zone; }
        void set_dns_hosts_max_count(const std::optional<int>& value) { dns_hosts_max_count = value; }
        void set_fast_hosts_count(int value) { fast_hosts_count = value; }

    private:
        void on_event(const std::string&, const json& obj, const json& obj_data);

        bool accept_callback_for_catched_fill(long long trade_id);
        void connection_fill_callback(long long trade_id, const std::string& data);

        bool accept_callback_for_catched_order_full_fill(long long order_id);
        void connection_order_full_fill_callback(long long order_id, const std::string& data);

        bool accept_callback_for_catched_order_cancel(long long order_id);
        void connection_order_cancel_callback(long long order_id, const std::string& data);

        void connection_ticker_callback(const std::string& data);

        std::vector<std::string> get_new_hosts();
        std::vector<std::string> get_fast_hosts();
        BinanceConsumerConnection* create_connection(
                int i_con,
                const std::string& host,
                const std::unordered_set<long long>& open_orders
        );
        void reconnect();

        std::vector<std::string> symbols;
        std::string name;

        std::mutex on_event_mutex;
        std::unordered_set<std::string> processed_events_keys = {};
        std::list<std::pair<std::string, std::uint64_t>> processed_events_queue = {};

        std::function<void(const std::string&)> on_fill_callback = [](const auto& data) {};
        std::function<void(long long, const std::string&)> on_full_fill_callback =
                [](long long, const auto& data) {};

        std::vector<std::function<void(long long, const std::string&)>> on_cancel_callbacks = {};
        std::function<void(const std::string&)> on_ticker_callback =
                [](const auto& data) {};

        std::vector<std::string> hosts;
        std::string hostname = "stream.binance.com";
        int port = 9443;
        std::string zone = "1a";
        BinanceMode mode = BinanceMode::SPOT;

        std::thread* thread = NULL;
        std::optional<performance::CpuGroup> cpu_group;
        volatile bool initialized = false;
        volatile bool need_to_finish = false;
        volatile bool synchronized_tracking_enabled = false;

        std::optional<int> dns_hosts_max_count = {};
        int fast_hosts_count = 12;

        std::mutex catched_fill_mutex;
        std::unordered_set<long long> catched_trades;
        std::mutex catched_order_cancel_mutex;
        std::unordered_set<long long> catched_order_cancels;
        std::mutex catched_order_full_fill_mutex;
        std::unordered_set<long long> catched_order_full_fills;
    };
}

#endif 
