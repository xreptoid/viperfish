#ifndef INCLUDE_VIPERFISH_BINANCE_HPP
#define INCLUDE_VIPERFISH_BINANCE_HPP

namespace viperfish::binance {

    enum BinanceMode {
        SPOT = 1,
        CROSS_MARGIN = 2,
        ISOLATED_MARGIN = 3,
        FUTURES = 4,
        FUTURES_DELIVERY = 5
    };
}

#endif 
