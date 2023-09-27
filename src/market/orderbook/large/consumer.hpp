#ifndef VIPERFISH_MARKET_DATA_ORDER_BOOK_LARGE_CONSUMER_HPP
#define VIPERFISH_MARKET_DATA_ORDER_BOOK_LARGE_CONSUMER_HPP

namespace viperfish::market::orderbook::large {

    class Consumer {
    public:

    protected:
        virtual void init();
        virtual void on_event();
    };
}

#endif