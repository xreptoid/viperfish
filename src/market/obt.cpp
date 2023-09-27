#include "market/obt.hpp"


namespace viperfish::market::orderbook {

    OrderBookTop OrderBookTop::create(
        const std::string& symbol,
        const Order& bid,
        const Order& ask,
        std::uint64_t local_ts,
        const std::optional<std::uint64_t> update_id
    ) {
        return OrderBookTop(
            symbol,
            bid,
            ask,
            bid.fprice,
            bid.amount,
            ask.fprice,
            ask.amount,
            local_ts,
            update_id
        );
    }

    viperfish::json OrderBookTop::get_json() const {
        viperfish::json data = {
            {"bid", bid},
            {"bid_amount", bid_amount},
            {"ask", ask},
            {"ask_amount", ask_amount},
            {"local_ts", local_ts},
        };
        if (update_id.has_value()) {
            data["update_id"] = *update_id;
        }
        return data;
    }
}
