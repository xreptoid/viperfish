#include <string>
#include "binance/consumer.hpp"
#include "reptoid.hpp"



int main() {

    auto consumer = viperfish::reptoid::LargeBinanceOrderBookConsumer();

    std::cout << "Consumer started" << std::endl;



    return 0;
}