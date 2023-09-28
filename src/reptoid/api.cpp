#include "./api.hpp"
#include "network/http/http.hpp"
#include "strings.hpp"
#include "gzip/decompress.hpp"
#include "timestamp.hpp"

namespace viperfish::reptoid {

    Api::Api() {
        update_endpoints();
    }

    Api::~Api() {}

    void Api::update_endpoints() {
    }

    market::orderbook::large::Snapshots Api::get_snapshots() {
        auto result_url = json::parse(network::http::request_get("https://api-ob.reptoid.com/ob-snapshots3/").buf)["data"]["result-url"].get<std::string>();
        std::cout << "result-url " << result_url << std::endl;
        auto resp = network::http::request_get(result_url);
        std::cout << "snapshots file downloaded" << std::endl;
        std::string decompressed_data = gzip::decompress(resp.buf.c_str(), resp.buf.size());
        auto data = json::parse(decompressed_data);

        market::orderbook::large::Snapshots::ob_map_t snapshots;
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
        return market::orderbook::large::Snapshots(snapshots);
    }

    market::orderbook::large::ObDiffsTail Api::get_ob_diffs_tail() {
        auto ts_from = get_current_ts() - 5 * 60 * 1000;
        auto ts_to = get_current_ts() - 2 * 1000;
        auto data = json::parse(
            network::http::request_get("https://api-ob.reptoid.com/diffs/",
            network::http::QueryString({
                {"ts_from", std::to_string(ts_from)},
                {"ts_to", std::to_string(ts_to)},
            })
        ).buf);
        std::cout << "diffs head received " << data.dump() << std::endl;
        auto result_url = data["data"]["result-url"].get<std::string>();

        std::cout << "downloading diffs" << std::endl;
        auto resp = network::http::request_get(result_url);
        std::cout << "decompressing diffs" << std::endl;
        std::string decompressed_data = gzip::decompress(resp.buf.c_str(), resp.buf.size());
        auto diffs_s = split_string(decompressed_data, '\n');
        std::vector<json> diffs;
        std::cout << "parsing diffs" << std::endl;
        std::vector<market::orderbook::OrderBookDiff> ob_diffs;
        for (const auto& diff_s: diffs_s) {
            if (!diff_s.size()) {
                continue;
            }
            auto ob_diff_data = json::parse(diff_s);
            market::orderbook::OrderBookDiff ob_diff(json_field_get<std::string>(ob_diff_data, "s"));
            for (const auto& o: ob_diff_data["b"]) {
                ob_diff.put_order(market::BUY, market::orderbook::Order::create(o[0].get<std::string>(), std::stold(o[1].get<std::string>())));
            }
            for (const auto& o: ob_diff_data["a"]) {
                ob_diff.put_order(market::SELL, market::orderbook::Order::create(o[0].get<std::string>(), std::stold(o[1].get<std::string>())));
            }        
            ob_diffs.push_back(ob_diff);
        }
        return market::orderbook::large::ObDiffsTail(ob_diffs);
    }
}
