#ifndef VIPERFISH_MARKET_ORDER_HPP
#define VIPERFISH_MARKET_ORDER_HPP
#include <string>

namespace viperfish::market {

    enum OrderSide {
        BUY = 1,
        SELL = 2
    };

    OrderSide reverse(OrderSide);

    std::string order_side2str(OrderSide);
    OrderSide str2order_side(const std::string&);
}

#endif 
