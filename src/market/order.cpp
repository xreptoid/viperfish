#include "market/order.hpp"

#include <cmath>
#include <iostream>

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

    namespace order_book {
        Order Order::create(const std::string& sprice, amount_t amount) {
            fprice_t fprice;
            if (sprice.find('.') != std::string::npos && sprice.back() == '0') {
                auto true_len = sprice.size() - 1;
                while (true_len >= 1 && sprice[true_len  - 1] == '0') {
                    --true_len;
                }
                std::string normalized_sprice = sprice.substr(0, true_len);
                return Order(normalized_sprice, std::stold(normalized_sprice), amount);
            }
            return Order(sprice, std::stold(sprice), amount);
        }

        bool Order::operator==(const Order& other) const {
            return sprice == other.sprice
                    && (fprice - other.fprice) < 1e-12
                    && (amount - other.amount) < 1e-12;
        }
    }
}