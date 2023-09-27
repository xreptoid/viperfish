#include <string>
#include "binance/consumer.hpp"
#include "strings.hpp"
#include "json.hpp"
#include "timestamp.hpp"
#include "network/http/http.hpp"
#include "gzip/decompress.hpp"
#include "market/orderbook/orderbook.hpp"
#include "reptoid.hpp"

using namespace viperfish;

void ob_diff_raw_callback(const json& obj) {
    if (obj.find("data") == obj.end()) {
        return;
    }
    auto data = obj["data"];
    if (!ends_with(json_field_get<std::string>(obj, "stream", ""), "@depth")) {
        return;
    }
    auto ts = json_field_get<std::uint64_t>(data, "E");
    //stream_writer.push(std::make_shared<json>(data), ts);
}

binance::BinanceConsumer* create_ob_diff_consumer(const std::vector<std::string>& symbols) {
    auto consumer = new binance::BinanceConsumer(symbols);
    //consumer->set_mode(mode);
    consumer->set_dns_hosts_max_count(1);
    consumer->set_fast_hosts_count(0);
    //consumer->set_zone(zone);
    consumer->settings.consume_tickers(false);
    consumer->settings.consume_trades(false);
    consumer->settings.consume_ob_diff(true, false); // ob diff without @100ms
    //consumer->settings.add_on_ob_diff_callback(ob_diff_raw_callback);
    consumer->settings.add_on_event_callback(ob_diff_raw_callback);
    consumer->run_async();
    consumer->wait_for_initializing();
    return consumer;
}


typedef std::unordered_map<std::string, std::string> SnapshotsUrls;


int main() {

    //std::cout << get_snapshots_urls().size() << std::endl;
    std::string symbol = "SOLEUR";

    /*
    auto* consumer = create_ob_diff_consumer({symbol});

    auto data = request_get(get_snapshots_urls()[symbol]);
    auto lastUpdateId = data["lastUpdateId"].get<std::uint64_t>();
    auto ob = market::orderbook::OrderBook();
    for (const auto& o: data["bids"]) {
        ob.put_order(market::BUY, market::orderbook::Order::create(o[0].get<std::string>(), std::stold(o[1].get<std::string>())));
    }
    for (const auto& o: data["asks"]) {
        ob.put_order(market::SELL, market::orderbook::Order::create(o[0].get<std::string>(), std::stold(o[1].get<std::string>())));
    }
    */

    /*
    auto snapshots = get_all_snapshots();
    std::cout << "snapshots: " << snapshots.size() << std::endl;

    auto diffs = get_ob_diffs(symbol, get_current_ts() - 7 * 60 * 1000, get_current_ts() - 2 * 1000);
    std::cout << "diffs: " << diffs.size() << std::endl;
    */


    //auto api = reptoid::Api();


    //delete consumer;



    return 0;
}