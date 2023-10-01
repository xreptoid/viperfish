#include <iostream>
#include <vector>
#include <unistd.h>
#include <viperfish/reptoid.hpp>

void print_orders(const std::vector<viperfish::market::orderbook::Order>& orders) {
    for (const auto& order: orders) {
        std::cout << order.sprice << ":\t" << order.amount << std::endl;
    }
}

int main() {
    auto context = viperfish::reptoid::orderbook::BinanceSpotContext();
    std::cout << "Context was initialized" << std::endl;

    for (int i = 0; i < 10; ++i) {
        std::cout << "------------------" << std::endl;
        auto asks = context.get_asks("BTCUSDT", 5);
        print_orders(std::vector(asks.rbegin(), asks.rend()));
        std::cout << "-" << std::endl;
        print_orders(context.get_bids("BTCUSDT", 5));
        sleep(3);
    }

    return 0;
}