#ifndef INCLUDE_VIPERFISH_MARKET_OBT_HPP
#define INCLUDE_VIPERFISH_MARKET_OBT_HPP

#include <string>
#include <optional>

#include "json.hpp"

#include "./order.hpp"


namespace viperfish::market::orderbook {

    class OrderBookTop {
    public:
        std::string symbol;

        Order bid_o;
        Order ask_o;

        double bid;
        amount_t bid_amount;
        double ask;
        amount_t ask_amount;

        std::uint64_t local_ts;
        std::optional<std::uint64_t> update_id;

        OrderBookTop(
                const std::string& symbol,
                const Order& bid_o,
                const Order& ask_o,
                double bid,
                amount_t bid_amount,
                double ask,
                amount_t ask_amount,
                std::uint64_t local_ts,
                const std::optional<std::uint64_t> update_id = {}
        )
                : symbol(symbol)
                , bid_o(bid_o)
                , ask_o(ask_o)
                , bid(bid)
                , bid_amount(bid_amount)
                , ask(ask)
                , ask_amount(ask_amount)
                , local_ts(local_ts)
                , update_id(update_id)
        {}

        static OrderBookTop create(
            const std::string& symbol,
            const Order& bid,
            const Order& ask,
            std::uint64_t local_ts,
            const std::optional<std::uint64_t> update_id = {}
        );

        virtual viperfish::json get_json() const;

        virtual long double get_price(OrderSide side) { return side == BUY ? bid : ask; }
        virtual long double get_rev_price(OrderSide side) { return side == SELL ? bid : ask; }
    };
}

namespace viperfish::market {

    typedef orderbook::OrderBookTop OrderBookTop;
}

#endif 
