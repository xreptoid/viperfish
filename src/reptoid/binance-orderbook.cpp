#include "./binance-orderbook.hpp"
#include "market/order.hpp"

namespace viperfish::reptoid {

    LargeBinanceOrderBookConsumer::LargeBinanceOrderBookConsumer() 
        : consumer(NULL)
    {
        this->consumer = new viperfish::market::orderbook::large::Consumer();
        this->run_binance_consumers();
        this->api = new Api();
        auto snapshots = this->api->get_snapshots();
        auto diffs = this->api->get_ob_diffs_tail(0, 0);
        // pass snapshot + diffs to Consumer
    }

    LargeBinanceOrderBookConsumer::~LargeBinanceOrderBookConsumer() {
        for (const auto* consumer: this->binance_ob_diff_consumers) {
            delete consumer;
        }
        delete this->consumer;
        delete this->api;
    }

    std::vector<std::vector<std::string>> get_symbols_batches(const std::vector<std::string>& symbols) {
        int max_batch_size = 300;
        auto n_batches = (symbols.size() + max_batch_size - 1) / max_batch_size;
        auto batch_size = (symbols.size() + n_batches - 1) / n_batches;
        std::cout << n_batches << " symbols batches, " << batch_size << " symbols in batch" << std::endl;
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
        auto ei;
        auto symbols = ei;
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
