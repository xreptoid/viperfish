#include "viperfish/binance/consumer.hpp"

#include <thread>
#include <chrono>
#include <mutex>
#include <iostream>

#include <boost/algorithm/string.hpp>

#include "viperfish/network/ws.hpp"
#include "viperfish/network/sockets.hpp"
#include "viperfish/json.hpp"
#include "viperfish/strings.hpp"
#include "viperfish/timestamp.hpp"


namespace viperfish::binance {

    BinanceConsumerConnection::BinanceConsumerConnection(
            const std::string& host,
            int port,
            const std::vector<std::string>& symbols,
            const BinanceConsumerThroughSettings& settings,
            const std::optional<std::function<void(const std::string&,const json&, const json&)>>& on_event_callback,
            const std::function<void(long long, const std::string&)>& on_fill_callback,
            const std::function<void(long long, const std::string&)>& on_full_fill_callback,
            const std::function<void(long long, const std::string&)>& on_cancel_callback,
            const std::function<void(const std::string&)>& on_ticker_callback,
            const std::unordered_set<long long>& open_orders
    )
            : host(host)
            , port(port)
            , symbols(symbols)
            , settings(settings)
            , on_event_callback(on_event_callback)
            , on_fill_callback(on_fill_callback)
            , on_full_fill_callback(on_full_fill_callback)
            , on_cancel_callback(on_cancel_callback)
            , on_ticker_callback(on_ticker_callback)
            , open_orders(open_orders)
    {
        for (auto& symbol: symbols) {
            std::string new_s(symbol);
            boost::algorithm::to_upper(new_s);
            this->symbols_set.insert(new_s);
        }
    }

    void BinanceConsumerConnection::run_async() {
        thread = new std::thread([this]() { this->run(); });
    }

    void BinanceConsumerConnection::run() {
        std::vector<std::string> streams;
        for (auto& symbol: symbols) {
            if (settings.consuming_trades_enabled) {
                streams.push_back(symbol + "@trade");
            }
            if (settings.consuming_agg_trades_enabled) {
                streams.push_back(symbol + "@aggTrade");
            }
            if (settings.consuming_tickers_enabled) {
                streams.push_back(symbol + "@bookTicker");
            }
            if (settings.consuming_ob_diff_enabled) {
                streams.push_back(symbol + "@depth" + (settings.ob_diff_ms100 ? "@100ms" : ""));
                streams.push_back(symbol + "@depth20");  // TODO @100ms??
            }
        }
        ws_client = new network::WsClient(
                host,
                port,
                get_streams_param(streams),
                [this](const std::string& data) { return this->callback(data); }
        );
        ws_client->set_name(_name);
        ws_client->run();
    }

    void BinanceConsumerConnection::finish() {
        if (ws_client != NULL) {
            ws_client->finish();
        }
        if (thread != NULL) {
            thread->join();
            delete thread;
            thread = NULL;
        }
        if (ws_client != NULL) {
            delete ws_client;
            ws_client = NULL;
        }
    }

    BinanceConsumerConnection::~BinanceConsumerConnection() {
        finish();
    }

    void BinanceConsumerConnection::callback(const std::string& data) {
        auto init_ts = get_current_ts_micro();
        auto obj = json::parse(data);
        if (obj["stream"].is_null()) {
            std::cout << log_prefix() << "stream field is null. Data:\n\t\t" << data << std::endl;
            return;
        }
        auto stream = json_field_get<std::string>(obj, "stream");
        auto obj_data = json_field_get<json>(obj, "data");
        std::string dataType = "";
        if (obj_data.find("e") != obj_data.end() && obj_data["e"].is_string()) {
            dataType = json_field_get<std::string>(obj_data, "e");
        }

        if (on_event_callback.has_value()) {
            (*on_event_callback)(data, obj, obj_data);
        }

        if (dataType == "executionReport" || dataType == "ORDER_TRADE_UPDATE") {
            process_execution_report(obj["data"], data);
            return;
        }

        if (dataType == "trade") {
            process_trade(obj["data"], data);
            return;
        }

        if (
                dataType == "bookTicker"
                || stream.substr(stream.size() - 10, 10) == "bookTicker"
        )
        {
            if (!settings.on_obt_callbacks.empty()) {
                auto obt = market::OrderBookTop::create(
                        obj["data"]["s"],
                        market::order_book::Order::create(obj["data"]["b"], std::stod(obj["data"]["B"].get<std::string>())),
                        market::order_book::Order::create(obj["data"]["a"], std::stod(obj["data"]["A"].get<std::string>())),
                        init_ts,
                        obj["data"]["u"]
                );
                for (const auto& on_obt_callback: settings.on_obt_callbacks) {
                    on_obt_callback(obt);
                }
                return;
            }

            // use legacy instead
            if (settings.on_obt_callbacks.empty()) {
                obj["lts"] = init_ts;
                auto data1 = obj.dump();
                process_ticker(obj["data"], data1);
            }
            return;
        }

        if (dataType == "depthUpdate") {
            if (!settings.on_ob_diff_callbacks.empty()) {
                for (const auto& on_ob_diff_callback: settings.on_ob_diff_callbacks) {
                    on_ob_diff_callback(obj["data"]);
                }
                return;
            }
            return;
        }

        if (ends_with(stream, "@depth20") || ends_with(stream, "@depth20@100ms")) {
            if (!settings.on_ob_snapshot_callbacks.empty()) {
                for (const auto& on_ob_snapshot_callback: settings.on_ob_snapshot_callbacks) {
                    on_ob_snapshot_callback(obj);
                }
                return;
            }
            return;
        }

        if (dataType == "outboundAccountPosition") {
            if (settings.on_balance_callbacks.empty()) {
                return;
            }

            std::unordered_map<std::string, double> currency2amount;
            for (auto& balance_row: obj["data"]["B"]) {
                std::string currency = balance_row["a"];
                double amount_free = std::stod(balance_row["f"].get<std::string>());
                double amount_locked = std::stod(balance_row["l"].get<std::string>());
                currency2amount[currency] = amount_free + amount_locked;
            };

            for (const auto& on_balance_callback: settings.on_balance_callbacks) {
                on_balance_callback(currency2amount);
            }

            return;
        }
    }

    void BinanceConsumerConnection::process_trade(json obj, const std::string& data) {
        auto ts = get_current_ts();
        if (obj.find("a") == obj.end()) {
            return;
        }
        auto trade_id = json_field_get<long long>(obj, "t");
        auto bid_order_id = json_field_get<long long>(obj, "b");
        auto ask_order_id = json_field_get<long long>(obj, "a");
        if (
                open_orders.find(bid_order_id) != open_orders.end()
                || open_orders.find(ask_order_id) != open_orders.end()
        )
        {
            on_fill_callback(trade_id, data);
        }

        auto server_ts = json_field_get<long long>(obj, "T");
        auto diff = (long long)ts - (long long)server_ts;
    }

    void BinanceConsumerConnection::process_ticker(json obj, const std::string& data) {
        on_ticker_callback(data);
    }

    void BinanceConsumerConnection::process_execution_report(json obj, const std::string& data) {
        auto data_type = json_field_get<std::string>(obj, "e");
        if (data_type == "ORDER_TRADE_UPDATE") {
            obj = obj["o"];
        }
        //std::cout << log_prefix() << "process_execution_report:" << obj << std::endl;
        auto status = json_field_get<std::string>(obj, "x");
        if (status == "TRADE") {
            process_fill(obj, data);
            return;
        }
        auto big_status = json_field_get<std::string>(obj, "X");
        if (big_status == "NEW") {
            process_new_order(obj);
            return;
        }
        if (big_status == "CANCELED") {
            process_canceled_order(obj, data);
            return;
        }
    }

    void BinanceConsumerConnection::process_fill(json obj, const std::string& data) {
        auto fill = BinanceFill::parse(obj);
        auto ts = get_current_ts();
        if (symbols_set.find(fill.symbol) != symbols_set.end()) {
            on_fill_callback(fill.trade_id, data);
        }
        if (fill.status == "FILLED") {
            on_full_fill_callback(fill.order_id, data);
        }
    }

    void BinanceConsumerConnection::process_new_order(json obj) {
        auto symbol = json_field_get<std::string>(obj, "s");
        if (symbols_set.find(symbol) == symbols_set.end()) {
            return;
        }
        auto order_id = json_field_get<long long>(obj, "i");
        //std::cout << log_prefix() << "process_new_order: new order " << symbol << " " << order_id << std::endl;
        open_orders.insert(order_id);
    }

    void BinanceConsumerConnection::process_canceled_order(json obj, const std::string& data) {
        auto symbol = json_field_get<std::string>(obj, "s");
        auto order_id = json_field_get<long long>(obj, "i");
        on_cancel_callback(order_id, data);
    }

    std::string BinanceConsumerConnection::get_streams_param(const std::vector<std::string>& streams) {
        std::string streams_param = "/stream";
        bool is_first_stream = true;
        for (auto stream: streams) {
            if (is_first_stream) {
                streams_param.append("?streams=");
                is_first_stream = false;
            } else {
                streams_param.append("/");
            }
            streams_param.append(stream);
        }
        return streams_param;
    }
}

