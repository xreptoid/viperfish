#include <string>
#include "binance/consumer.hpp"
#include "strings.hpp"
#include "json.hpp"
#include "timestamp.hpp"
#include "network/http/http.hpp"
#include "gzip/decompress.hpp"
#include "market/orderbook.hpp"
#include "reptoid/api.hpp"

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

SnapshotsUrls get_snapshots_urls() {
    auto data = json::parse(network::http::request_get("https://api-ob.reptoid.com/ob-snapshots/").buf);

    SnapshotsUrls urls;
    for (const auto& [key, value]: data["data"]["ob_paths"]["paths"].items()) {
        urls[key] = value;
    }
    return urls;
}


market::orderbook::OrderBook get_snapshot(const std::string& symbol) {
    auto data = json::parse(network::http::request_get(get_snapshots_urls()[symbol]).buf);
    auto ob = market::orderbook::OrderBook();
    for (const auto& o: data["bids"]) {
        ob.put_order(market::BUY, market::orderbook::Order::create(o[0].get<std::string>(), std::stold(o[1].get<std::string>())));
    }
    for (const auto& o: data["asks"]) {
        ob.put_order(market::SELL, market::orderbook::Order::create(o[0].get<std::string>(), std::stold(o[1].get<std::string>())));
    }
    return ob;
}


std::unordered_map<std::string, market::orderbook::OrderBook> get_all_snapshots() {
    auto result_url = json::parse(network::http::request_get("https://api-ob.reptoid.com/ob-snapshots3/").buf)["data"]["result-url"].get<std::string>();
    std::cout << "result-url " << result_url << std::endl;
    auto resp = network::http::request_get(result_url);
    std::cout << "snapshots file downloaded" << std::endl;
    std::string decompressed_data = gzip::decompress(resp.buf.c_str(), resp.buf.size());
    auto data = json::parse(decompressed_data);

    std::unordered_map<std::string, market::orderbook::OrderBook> snapshots;
    for (const auto& [symbol, snapshot_data]: data.items()) {
        auto snapshot = market::orderbook::OrderBook(
            symbol,
            snapshot_data["ob_snapshot"]["lastUpdateId"].get<std::int64_t>(),
            snapshot_data["local_ts_before"].get<std::int64_t>()
        );
        for (const auto& o: snapshot_data["ob_snapshot"]["bids"]) {
            snapshot.put_order(market::BUY, market::orderbook::Order::create(o[0].get<std::string>(), std::stold(o[1].get<std::string>())));
        }
        for (const auto& o: snapshot_data["ob_snapshot"]["asks"]) {
            snapshot.put_order(market::SELL, market::orderbook::Order::create(o[0].get<std::string>(), std::stold(o[1].get<std::string>())));
        }

        snapshots.insert(std::make_pair(symbol, snapshot));
    }

    return snapshots;
}


std::vector<json> get_ob_diffs(const std::string& symbol, std::int64_t ts_from, std::int64_t ts_to) {
    auto data = json::parse(
        network::http::request_get("https://api-ob.reptoid.com/diffs/",
        network::http::QueryString({
            {"ts_from", std::to_string(ts_from)},
            {"ts_to", std::to_string(ts_to)},
        })
    ).buf);
    auto result_url = data["data"]["result-url"].get<std::string>();

    std::cout << "downloading diffs" << std::endl;
    auto resp = network::http::request_get(result_url);
    std::cout << "decompressing diffs" << std::endl;
    std::string decompressed_data = gzip::decompress(resp.buf.c_str(), resp.buf.size());
    auto diffs_s = split_string(decompressed_data, '\n');
    std::vector<json> diffs;
    std::cout << "parsing diffs" << std::endl;
    for (const auto& diff_s: diffs_s) {
        if (!diff_s.size()) {
            continue;
        }
        diffs.push_back(json::parse(diff_s));
    }
    return diffs;
}


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


    auto api = reptoid::Api();


    //delete consumer;



    return 0;
}