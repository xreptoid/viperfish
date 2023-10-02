# viperfish - C++ library for algorithmic trading
Currently it supports only full orderbook for Binance Spot market.
You can subscribe for whole market orderbooks in realtime without Binance REST limits exceeding.


## Example

```cpp
auto context = viperfish::reptoid::orderbook::BinanceSpotContext();

// You can retrieve an order book for any Binance Spot symbol immediately after the object is created.
// Here is an example of getting top 5 bid/ask orders:
auto bids = context.get_bids("BTCUSDT", 5);
auto asks = context.get_asks("BTCUSDT", 5);

// Calculate the total amount for bid orders at a specified depth.
// For example, if the market price is 29700, calculating for bids from 29700 to 29200:
auto total_amount = context.get_top_amount("BTCUSDT", viperfish::market::BUY, bids[0].fprice - 500);
```

## Installing
### Debian
Currently only arm64 (aarch64) is supported. Tested on Ubuntu 20.x/22.x.

**Add Reptoid repo**
```bash
curl -fsSL https://deb.reptoid.com/gpg-key \
    | sudo gpg --dearmor -o /usr/share/keyrings/reptoid.gpg
echo "deb [signed-by=/usr/share/keyrings/reptoid.gpg] https://deb.reptoid.com focal main" \
    | sudo tee /etc/apt/sources.list.d/reptoid.list

sudo apt update
```

**Install viperfish**
```
sudo apt install viperfish
```

### Building from source on Linux

```
sudo apt install -y \
    clang-12 cmake
    libssl-dev libcurl4 libcurl4-openssl-dev \
    libboost-dev libboost-filesystem-dev libboost-thread-dev
```

```bash
cmake .
make viperfish
sudo make install
```

### Building from source on OS X
TODO

## Importing with CMake
CMakeLists.txt
```cmake
project(sample)
cmake_minimum_required(VERSION 3.10)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(OpenSSL REQUIRED)
include_directories(
        ${OPENSSL_INCLUDE_DIR}
)

find_package(CURL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Boost COMPONENTS thread system filesystem REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_executable(
        main
        main.cpp
)
target_link_libraries(
        main
        viperfish
        ${CURL_LIBRARIES}
        ${OPENSSL_SSL_LIBRARY}
        ${OPENSSL_CRYPTO_LIBRARY}
        ${ZLIB_LIBRARIES}
        ${Boost_LIBRARIES}
        pthread dl
)
```

```cpp
#include <iostream>
#include <viperfish/reptoid.hpp>

int main() {
    auto context = viperfish::reptoid::orderbook::BinanceSpotContext();
    std::cout << "Context was initialized" << std::endl;
    return 0;
}
```
