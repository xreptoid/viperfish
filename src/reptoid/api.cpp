#include "./api.hpp"
#include "network/http/http.hpp"
#include "strings.hpp"
#include "gzip/decompress.hpp"

namespace viperfish::reptoid {

    Api::Api() {
        update_endpoints();
    }

    Api::~Api() {}

    orderbook::Snapshots Api::get_snapshots() {
        auto result_url = json::parse(network::http::request_get("https://api-ob.reptoid.com/ob-snapshots3/").buf)["data"]["result-url"].get<std::string>();
        std::cout << "result-url " << result_url << std::endl;
        auto resp = network::http::request_get(result_url);
        std::cout << "snapshots file downloaded" << std::endl;
        std::string decompressed_data = gzip::decompress(resp.buf.c_str(), resp.buf.size());
        auto data = json::parse(decompressed_data);

        orderbook::Snapshots::ob_map_t snapshots;
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
        return orderbook::Snapshots(snapshots);
    }

    orderbook::Diffs Api::get_ob_diffs_tail(std::int64_t ts_from, std::int64_t ts_to) {
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
        //return diffs;
        return orderbook::Diffs();
    }

    void Api::update_endpoints() {
    }
}
