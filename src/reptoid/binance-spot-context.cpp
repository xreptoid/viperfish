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

namespace viperfish::reptoid::orderbook {

    BinanceSpotContext::BinanceSpotContext(
        const std::vector<std::string>& symbols_
    ) 
        : consumer(NULL)
        , symbols(symbols_)
    {
        this->init_symbols();
        this->consumer = new viperfish::market::orderbook::large::Consumer(this->symbols);
        this->run_binance_consumers();
        this->api = new Api();

        auto ob_snapshots_future = std::async([this]() { return this->api->get_snapshots(); });
        sleep(15);
        auto ob_diffs_tail = this->api->get_ob_diffs_tail();
        auto ob_snapshots = ob_snapshots_future.get();
        this->consumer->set_ob_snapshots(ob_snapshots);
        this->consumer->set_ob_diffs_tail(ob_diffs_tail);
    }

    BinanceSpotContext::~BinanceSpotContext() {
        for (const auto* consumer: this->binance_ob_diff_consumers) {
            delete consumer;
        }
        delete this->consumer;
        delete this->api;
    }

    long double BinanceSpotContext::get_top_amount(
        const std::string& symbol,
        market::OrderSide side,
        long double price
    ) {
        return consumer->obs_container.get(symbol)->get_top_amount(side, price);
    }

    std::vector<market::orderbook::Order> BinanceSpotContext::get_bids(
        const std::string& symbol,
        int limit
    ) {
        return consumer->obs_container.get(symbol)->get_bids(limit);
    }

    std::vector<market::orderbook::Order> BinanceSpotContext::get_asks(
        const std::string& symbol,
        int limit
    ) {
        return consumer->obs_container.get(symbol)->get_asks(limit);
    }

    std::vector<std::vector<std::string>> get_symbols_batches(const std::vector<std::string>& symbols) {
        if (symbols.empty()) {
            return {};
        }
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

    void BinanceSpotContext::init_symbols() {
        std::unordered_set<std::string> ei_symbols_set;
        auto ei_data = json::parse(network::http::request_get("https://api.binance.com/api/v3/exchangeInfo").buf);
        auto ei = binance::BinanceExchangeInfo(binance::SPOT, ei_data);
        for (const auto& symbol: ei.spot_symbols) {
            ei_symbols_set.insert(symbol.binance());
        }
        if (this->symbols.size()) {
            std::vector<std::string> filtered_symbols;
            std::vector<std::string> ignored_symbols;
            for (const auto& symbol: this->symbols) {
                if (ei_symbols_set.count(symbol)) {
                    filtered_symbols.push_back(symbol);
                } else {
                    ignored_symbols.push_back(symbol);
                }
            }
            if (ignored_symbols.size()) {
                std::cout << "Symbols that not supported by Binance SPOT trading and were ignored:" << std::endl;
                std::cout << "\t\t";
                for (const auto& symbol: ignored_symbols) {
                    std::cout << symbol << " ";
                }
                std::cout << std::endl;
            }
            this->symbols = filtered_symbols;
        } else {
            this->symbols = std::vector(ei_symbols_set.begin(), ei_symbols_set.end());
        }
    }

    void BinanceSpotContext::run_binance_consumers() {
        
        auto symbols_batches = get_symbols_batches(symbols);
        for (const auto& batch: symbols_batches) {
            binance_ob_diff_consumers.push_back(create_ob_diff_consumer(batch));
        }
    }

    binance::BinanceConsumer* BinanceSpotContext::create_ob_diff_consumer(const std::vector<std::string>& symbols) {
        auto consumer = new binance::BinanceConsumer(symbols);
        consumer->set_dns_hosts_max_count(1);
        consumer->set_fast_hosts_count(0);
        consumer->settings.consume_tickers(false);
        consumer->settings.consume_trades(false);
        consumer->settings.consume_ob_diff(true);
        consumer->settings.add_on_event_callback([this](const json& diff) { this->on_event(diff); });
        consumer->run_async();
        consumer->wait_for_initializing();
        return consumer;
    }

    void BinanceSpotContext::on_event(const json& obj) {
        if (obj.find("data") == obj.end()) {
            return;
        }
        auto data = obj["data"];
        if (!ends_with(json_field_get<std::string>(obj, "stream", ""), "@depth@100ms")) {
            return;
        }
        auto symbol = json_field_get<std::string>(data, "s");
        auto ts = json_field_get<std::uint64_t>(data, "E");
        auto first_update_id = json_field_get<std::uint64_t>(data, "U");
        auto final_update_id = json_field_get<std::uint64_t>(data, "u");
        auto ob_diff = market::orderbook::OrderBookDiff(symbol, first_update_id, final_update_id);
        for (const auto& o: data["b"]) {
            ob_diff.put_order(market::BUY, market::orderbook::Order::create(o[0].get<std::string>(), std::stold(o[1].get<std::string>())));
        }
        for (const auto& o: data["a"]) {
            ob_diff.put_order(market::SELL, market::orderbook::Order::create(o[0].get<std::string>(), std::stold(o[1].get<std::string>())));
        }
        this->consumer->push_ob_diff(ob_diff);
    }
}
