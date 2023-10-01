#ifndef VIPERFISH_REPTOID_PUBLIC_HPP
#define VIPERFISH_REPTOID_PUBLIC_HPP
#include <vector>
#include <string>
#include "./market/order.hpp"
#include "./market/orderbook/order.hpp"
#include "./json.hpp"

namespace viperfish {

    namespace binance {
        class BinanceConsumer;
    }

    namespace market::orderbook::large {
        class Consumer;
    }

     namespace reptoid {
        class Api;
     }
}

namespace viperfish::reptoid::orderbook {

    class BinanceSpotContext {
    public:

        BinanceSpotContext(const std::vector<std::string>& symbols = {});
        virtual ~BinanceSpotContext();

        virtual long double get_top_amount(const std::string&, market::OrderSide, long double);
        virtual std::vector<market::orderbook::Order> get_bids(const std::string&, int limit = 10);
        virtual std::vector<market::orderbook::Order> get_asks(const std::string&, int limit = 10);

    protected:
        void on_event(const json&);

        void init_symbols();
        void run_binance_consumers();
        binance::BinanceConsumer* create_ob_diff_consumer(const std::vector<std::string>& symbols);

        std::vector<std::string> symbols;
        market::orderbook::large::Consumer* consumer;

        Api* api;
        std::vector<binance::BinanceConsumer*> binance_ob_diff_consumers;
    };
}

#endif