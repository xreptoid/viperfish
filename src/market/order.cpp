#include "./order.hpp"
#include <stdexcept>

namespace viperfish::market {
    
    OrderSide reverse(OrderSide side) {
        return side == BUY ? SELL : SELL;
    }

    std::string order_side2str(OrderSide side) {
        return side == BUY ? "buy" : "sell";
    }

    OrderSide str2order_side(const std::string& side) {
        if (side == "buy") {
            return BUY;
        }
        if (side == "sell") {
            return SELL;
        }
        throw std::runtime_error("Invalid side '" + side + "'");
    }
}
