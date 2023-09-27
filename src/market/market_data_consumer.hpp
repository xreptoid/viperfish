#ifndef VIPERFISH_MARKET_DATA_CONSUMER_HPP
#define VIPERFISH_MARKET_DATA_CONSUMER_HPP

#include <functional>
#include <mutex>
#include <vector>
#include <iostream>

#include "./obt.hpp"
#include "./orderbook.hpp"

namespace viperfish::market {

    class MarketDataConsumer {
    public:

        virtual ~MarketDataConsumer() {
            finish();
        }
        
        virtual void add_symbols(const std::vector<std::string>& new_symbols);

        typedef std::function<void(const OrderBookTop&)> orderbook_top_callback_t;
        virtual void add_orderbook_top_callback(const orderbook_top_callback_t& orderbook_top_callback);
        typedef std::function<void(const orderbook::OrderBookDiff&)> ob_diff_callback_t;
        virtual void add_ob_diff_callback(const ob_diff_callback_t& ob_diff_callback);
        typedef std::function<void(const orderbook::OrderBook&)> ob_snapshot_callback_t;
        virtual void add_ob_snapshot_callback(const ob_snapshot_callback_t& ob_snapshot_callback);

        virtual void start() {};
        virtual void finish() {}

    protected:
        std::vector<std::string> symbols;
        virtual void execute_orderbook_top_callbacks(const OrderBookTop&);
        virtual void execute_ob_diff_callbacks(const json&);
        virtual void execute_ob_snapshot_callbacks(const json&);
        std::vector<orderbook_top_callback_t> orderbook_top_callbacks;
        std::mutex orderbook_top_callbacks_mutex;
        std::vector<ob_diff_callback_t> ob_diff_callbacks;
        std::mutex ob_diff_callbacks_mutex;
        std::vector<ob_snapshot_callback_t> ob_snapshot_callbacks;
        std::mutex ob_snapshot_callbacks_mutex;
    };

    class MockedMarketDataConsumer : public MarketDataConsumer {
    public:

        void execute_orderbook_top_callbacks(const OrderBookTop&) override;
    };
}

#endif 
