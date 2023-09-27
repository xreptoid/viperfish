#ifndef VIPERFISH_MARKET_DATA_ORDER_BOOK_LARGE_CONSUMER_HPP
#define VIPERFISH_MARKET_DATA_ORDER_BOOK_LARGE_CONSUMER_HPP
#include <string>
#include "market/orderbook/orderbook.hpp"

namespace viperfish::market::orderbook::large {

    class Event {
    public:
        std::string trigger_symbol;

        Event(const std::string&);
    };

    class Consumer {
    public:
        virtual void on_ob_diff(const market::orderbook::OrderBookDiff&);

    protected:
        virtual void init();
    };
}

#endif