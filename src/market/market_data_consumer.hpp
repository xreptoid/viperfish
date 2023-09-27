#ifndef VIPERFISH_MARKET_DATA_CONSUMER_HPP
#define VIPERFISH_MARKET_DATA_CONSUMER_HPP

#include <functional>
#include <mutex>
#include <vector>
#include <iostream>

#include "./obt.hpp"
#include "./order_book.hpp"

namespace viperfish::market {

    class MarketDataConsumer {
    public:

        virtual ~MarketDataConsumer() {
            finish();
        }
        
        virtual void add_symbols(const std::vector<std::string>& new_symbols);

        typedef std::function<void(const OrderBookTop&)> order_book_top_callback_t;
        virtual void add_order_book_top_callback(const order_book_top_callback_t& order_book_top_callback);
        typedef std::function<void(const order_book::OrderBookDiff&)> ob_diff_callback_t;
        virtual void add_ob_diff_callback(const ob_diff_callback_t& ob_diff_callback);
        typedef std::function<void(const order_book::OrderBook&)> ob_snapshot_callback_t;
        virtual void add_ob_snapshot_callback(const ob_snapshot_callback_t& ob_snapshot_callback);

        virtual void start() {};
        virtual void finish() {}

    protected:
        std::vector<std::string> symbols;
        virtual void execute_order_book_top_callbacks(const OrderBookTop&);
        virtual void execute_ob_diff_callbacks(const json&);
        virtual void execute_ob_snapshot_callbacks(const json&);
        std::vector<order_book_top_callback_t> order_book_top_callbacks;
        std::mutex order_book_top_callbacks_mutex;
        std::vector<ob_diff_callback_t> ob_diff_callbacks;
        std::mutex ob_diff_callbacks_mutex;
        std::vector<ob_snapshot_callback_t> ob_snapshot_callbacks;
        std::mutex ob_snapshot_callbacks_mutex;
    };

    class MockedMarketDataConsumer : public MarketDataConsumer {
    public:

        void execute_order_book_top_callbacks(const OrderBookTop&) override;
    };
}

#endif 
