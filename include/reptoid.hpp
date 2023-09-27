#ifndef VIPERFISH_REPTOID_PUBLIC_HPP
#define VIPERFISH_REPTOID_PUBLIC_HPP
#include "market/orderbook/large/consumer.hpp"
#include "binance.hpp"
#include "reptoid/api.hpp"

namespace viperfish::reptoid {

    class LargeBinanceOrderBookConsumer {
    public:

        LargeBinanceOrderBookConsumer();
        virtual ~LargeBinanceOrderBookConsumer();

    protected:
        void on_event(const json&);

        void run_binance_consumers();
        binance::BinanceConsumer* create_ob_diff_consumer(const std::vector<std::string>& symbols);

        viperfish::market::orderbook::large::Consumer* consumer;
        Api* api;
        std::vector<binance::BinanceConsumer*> binance_ob_diff_consumers;
    };
}

#endif