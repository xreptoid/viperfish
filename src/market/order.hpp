#ifndef INCLUDE_VIPERFISH_MARKET_ORDER_HPP
#define INCLUDE_VIPERFISH_MARKET_ORDER_HPP

#include <string>

namespace viperfish::market {

    enum OrderSide {
        BUY = 1,
        SELL = 2
    };

    OrderSide reverse(OrderSide);

    std::string order_side2str(OrderSide);
    OrderSide str2order_side(const std::string&);

    namespace orderbook {

        typedef long double fprice_t;
        typedef long double amount_t;

        class Order {
        public:

            std::string sprice;
            fprice_t fprice;
            amount_t amount;

            Order(
                const std::string& sprice,
                fprice_t fprice,
                amount_t amount
            )
                    : sprice(sprice)
                    , fprice(fprice)
                    , amount(amount)
            {}

            Order() = default;
            static Order create(const std::string& sprice, amount_t amount);

            bool operator==(const Order& other) const;
        };
    }
}

#endif 
