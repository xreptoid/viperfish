#include "viperfish/reptoid.hpp"
#include <future>
#include "./api.hpp"
#include "network/http/http.hpp"
#include "market/order.hpp"
#include "binance/exchange_info.hpp"
#include "binance/consumer.hpp"
#include "market/order.hpp"
#include "market/orderbook/large/consumer.hpp"
#include "json/getter.hpp"

namespace viperfish::reptoid {

    LargeBinanceOrderBookConsumer::LargeBinanceOrderBookConsumer() 
        : consumer(NULL)
    {
        this->consumer = new viperfish::market::orderbook::large::Consumer();
        this->run_binance_consumers();
        this->api = new Api();

        auto ob_snapshots_future = std::async([this]() { return this->api->get_snapshots(); });
        auto ob_diffs_tail = this->api->get_ob_diffs_tail();
        auto ob_snapshots = ob_snapshots_future.get();
        this->consumer->set_ob_snapshots(ob_snapshots);
        this->consumer->set_ob_diffs_tail(ob_diffs_tail);
    }

    LargeBinanceOrderBookConsumer::~LargeBinanceOrderBookConsumer() {
        for (const auto* consumer: this->binance_ob_diff_consumers) {
            delete consumer;
        }
        delete this->consumer;
        delete this->api;
    }

    long double LargeBinanceOrderBookConsumer::get_top_amount(
        const std::string& symbol,
        market::OrderSide side,
        long double price
    ) {
        return consumer->obs_container.get(symbol)->get_top_amount(side, price);
    }

    std::vector<std::vector<std::string>> get_symbols_batches(const std::vector<std::string>& symbols) {
        int max_batch_size = 300;
        auto n_batches = (symbols.size() + max_batch_size - 1) / max_batch_size;
        auto batch_size = (symbols.size() + n_batches - 1) / n_batches;
        std::vector<std::vector<std::string>> batches;
        for (int i = 0; i < symbols.size(); ++i) {
            int i_batch = i / batch_size;
            if (i_batch >= batches.size()) {
                batches.emplace_back();
            }
            batches.back().push_back(symbols[i]);
        }
        return batches;
    }

    void LargeBinanceOrderBookConsumer::run_binance_consumers() {
        auto ei_data = json::parse(network::http::request_get("https://api.binance.com/api/v3/exchangeInfo").buf);
        auto ei = binance::BinanceExchangeInfo(binance::SPOT, ei_data);
        std::vector<std::string> symbols;
        for (const auto& symbol: ei.spot_symbols) {
            symbols.push_back(symbol.binance());
        }
        auto symbols_batches = get_symbols_batches(symbols);
        for (const auto& batch: symbols_batches) {
            binance_ob_diff_consumers.push_back(create_ob_diff_consumer(batch));
        }
    }

    binance::BinanceConsumer* LargeBinanceOrderBookConsumer::create_ob_diff_consumer(const std::vector<std::string>& symbols) {
        auto consumer = new binance::BinanceConsumer(symbols);
        consumer->set_dns_hosts_max_count(1);
        consumer->set_fast_hosts_count(0);
        consumer->settings.consume_tickers(false);
        consumer->settings.consume_trades(false);
        consumer->settings.consume_ob_diff(true, false); // ob diff without @100ms
        consumer->settings.add_on_event_callback([this](const json& diff) { this->on_event(diff); });
        consumer->run_async();
        consumer->wait_for_initializing();
        return consumer;
    }

    void LargeBinanceOrderBookConsumer::on_event(const json& obj) {
        if (obj.find("data") == obj.end()) {
            return;
        }
        auto data = obj["data"];
        if (!ends_with(json_field_get<std::string>(obj, "stream", ""), "@depth")) {
            return;
        }
        auto symbol = json_field_get<std::string>(data, "s");
        auto ts = json_field_get<std::uint64_t>(data, "E");
        auto ob_diff = market::orderbook::OrderBookDiff(symbol);
        for (const auto& o: data["b"]) {
            ob_diff.put_order(market::BUY, market::orderbook::Order::create(o[0].get<std::string>(), std::stold(o[1].get<std::string>())));
        }
        for (const auto& o: data["a"]) {
            ob_diff.put_order(market::SELL, market::orderbook::Order::create(o[0].get<std::string>(), std::stold(o[1].get<std::string>())));
        }
        this->consumer->push_ob_diff(ob_diff);
    }
}
