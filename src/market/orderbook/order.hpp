#ifndef VIPERFISH_ORDERBOOK_ORDER_HPP
#define VIPERFISH_ORDERBOOK_ORDER_HPP
#include <string>

namespace viperfish::market::orderbook {

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

#endif 
