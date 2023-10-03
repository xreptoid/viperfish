#include "./api.hpp"
#include "network/http/http.hpp"
#include "strings.hpp"
#include "gzip/decompress.hpp"
#include "timestamp.hpp"
#include "json/getter.hpp"

namespace viperfish::reptoid {

    Api::Api() {
        update_endpoints();
    }

    Api::~Api() {}

    void Api::update_endpoints() {
        std::string resp;
        try {
            resp = network::http::request_get("https://www.reptoid.com/api/orderbook/meta").buf;
        } catch (const std::exception& e) {
            std::cout << "Error on fetching meta from server: " << e.what() << std::endl;
            throw e;
        }
        json data;
        try {
            data = json::parse(resp);
            data = json_field_get<json>(data, "data");
        } catch (const std::exception& e) {
            std::cout << "Error on parsing meta JSON: " << e.what() << std::endl;
            std::cout << resp << std::endl;
            throw e;
        }
        try {
            auto welcome_msg = json_field_get<std::string>(data["welcome"], "msg");
            std::cout << "Reptoid API: " << welcome_msg << std::endl;
        } catch(const std::exception& e) {
            //
        }
        this->snapshots_url = json_field_get<std::string>(data, "snapshotsUrl");
        this->ob_diffs_tail_url = json_field_get<std::string>(data, "obDiffsTailUrl");
    }

    market::orderbook::large::Snapshots Api::get_snapshots() {
        std::string resp;
        std::string result_url;
        int max_retries = 5;
        for (int i = 0; i < max_retries && result_url.empty(); ++i) {
            try {
                resp = network::http::request_get(this->snapshots_url).buf;
            } catch (const std::exception& e) {
                std::cout << "Error on fetching snapshots meta from server: " << e.what() << std::endl;
                throw e;
            }
            try {
                result_url = json::parse(resp)["data"]["result-url"].get<std::string>();
            } catch (const std::exception& e) {
                std::cout << "Error on getting snapshots url: " << e.what() << std::endl;
                std::cout << resp << std::endl;
                if (i < max_retries - 1) {
                    std::cout << "Retry in 1 sec" << std::endl;
                    sleep(1);
                } else {
                    throw e;
                }
            }
        }

        try {
            resp = network::http::request_get(result_url).buf;
        } catch (const std::exception& e) {
            std::cout << "Error on downloading snapshot: " << e.what() << std::endl;
            throw e;
        }
        std::string decompressed_data;
        try {
            decompressed_data = gzip::decompress(resp.c_str(), resp.size());
        } catch (const std::exception& e) {
            std::cout << "Error on decompressing snapshot: " << e.what() << std::endl;
            throw e;
        }
        json data;
        try {
            data = json::parse(decompressed_data);
        } catch (const std::exception& e) {
            std::cout << "Error on parsing snapshot JSON: " << e.what() << std::endl;
            std::cout << resp << std::endl;
            throw e;
        }

        market::orderbook::large::Snapshots::ob_map_t snapshots;
        for (const auto& [symbol, snapshot_data]: data.items()) {
            json snapshot_inner_data;
            std::uint64_t last_update_id;
            std::vector<json> bids;
            std::vector<json> asks;
            try {
                snapshot_inner_data = json_field_get<json>(snapshot_data, "ob_snapshot");
                last_update_id = json_field_get<std::int64_t>(snapshot_inner_data, "lastUpdateId");
                bids = json_field_get<std::vector<json>>(snapshot_inner_data, "bids");
                asks = json_field_get<std::vector<json>>(snapshot_inner_data, "asks");
            } catch (const std::exception& e) {
                std::cout << "Error on getting fields from snapshot JSON: " << e.what() << std::endl;
                std::cout << symbol << " " << snapshot_data.dump() << std::endl;
                throw e;
            }
            auto snapshot = market::orderbook::OrderBook(
                symbol,
                last_update_id
                //json_field_get<std::int64_t>(snapshot_data, "local_ts_before")
            );
            for (const auto& o: bids) {
                snapshot.put_order(
                    market::BUY,
                    market::orderbook::Order::create(o[0].get<std::string>(), std::stold(o[1].get<std::string>()))
                );
            }
            for (const auto& o: asks) {
                snapshot.put_order(
                    market::SELL,
                    market::orderbook::Order::create(o[0].get<std::string>(), std::stold(o[1].get<std::string>()))
                );
            }

            snapshots.insert(std::make_pair(symbol, snapshot));
        }
        return market::orderbook::large::Snapshots(snapshots);
    }

    market::orderbook::large::ObDiffsTail Api::get_ob_diffs_tail() {
        // FIXME
        auto ts_from = get_current_ts() - 10 * 60 * 1000;
        auto ts_to = get_current_ts() - 2 * 1000;
        std::string resp;
        try {
            resp = network::http::request_get(
                this->ob_diffs_tail_url,
                network::http::QueryString({
                    {"ts_from", std::to_string(ts_from)},
                    {"ts_to", std::to_string(ts_to)},
                })
            ).buf;
        } catch (const std::exception& e) {
            std::cout << "Error on fetching orderbook diff tail meta: " << e.what() << std::endl;
            throw e;
        }

        std::string result_url;
        try {
            result_url = json::parse(resp)["data"]["result-url"].get<std::string>();
        } catch (const std::exception& e) {
            std::cout << "Error on getting orderbook diff tail url: " << e.what() << std::endl;
            std::cout << resp << std::endl;
            throw e;
        }

        try {
            resp = network::http::request_get(result_url).buf;
        } catch (const std::exception& e) {
            std::cout << "Error on downloading orderbook diff tail: " << e.what() << std::endl;
            throw e;
        }
        std::string decompressed_data;
        try {
            decompressed_data = gzip::decompress(resp.c_str(), resp.size());
        } catch (const std::exception& e) {
            std::cout << "Error on decompressing orderbook diff tail: " << e.what() << std::endl;
            throw e;
        }

        auto diffs_s = split_string(decompressed_data, '\n');
        std::vector<json> diffs;
        std::vector<market::orderbook::OrderBookDiff> ob_diffs;
        for (size_t i = 0; i < diffs_s.size() - 1; ++i) {  // skip last line because of probability of incompleteness
            auto diff_s = diffs_s[i];
            if (!diff_s.size()) {
                continue;
            }
            json ob_diff_data;
            try {
                ob_diff_data = json::parse(diff_s);
            } catch (const std::exception& e) {
                std::cout << "Error on parsing orderbook diff item: " << e.what() << std::endl;
                std::cout << diff_s << std::endl;
            }
            auto symbol = json_field_get<std::string>(ob_diff_data, "s");
            auto first_update_id = json_field_get<std::uint64_t>(ob_diff_data, "U");
            auto final_update_id = json_field_get<std::uint64_t>(ob_diff_data, "u");
            market::orderbook::OrderBookDiff ob_diff(symbol, first_update_id, final_update_id);
            for (const auto& o: json_field_get<std::vector<json>>(ob_diff_data, "b")) {
                ob_diff.put_order(market::BUY, market::orderbook::Order::create(o[0].get<std::string>(), std::stold(o[1].get<std::string>())));
            }
            for (const auto& o: json_field_get<std::vector<json>>(ob_diff_data, "a")) {
                ob_diff.put_order(market::SELL, market::orderbook::Order::create(o[0].get<std::string>(), std::stold(o[1].get<std::string>())));
            }        
            ob_diffs.push_back(ob_diff);
        }
        return market::orderbook::large::ObDiffsTail(ob_diffs);
    }
}
