#include <iostream>
#include <viperfish/reptoid.hpp>

int main() {
    auto consumer = viperfish::reptoid::LargeBinanceOrderBookConsumer();
    std::cout << "Consumer started" << std::endl;
    return 0;
}