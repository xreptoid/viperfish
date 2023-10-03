project(binance-order-book)
cmake_minimum_required(VERSION 3.10)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

include_directories(
        ../../include
        ../../src
        /usr/local/include
)
link_directories(
        /usr/local/lib
)

if(APPLE)
    set(OPENSSL_ROOT_DIR /usr/local/opt/openssl@1.1)
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

file(
        GLOB_RECURSE
        COMMON_CPP_FILES
        ../../src/exchanges/binance/*.cpp
        ../../src/json/*.cpp
        ../../src/market/*.cpp
        ../../src/network/http/*.cpp
        ../../src/network/http/curl.cpp
        ../../src/network/sockets/sockets.cpp
        ../../src/network/ws/ws_boost.cpp
        ../../src/numbers/*.cpp
        ../../src/performance/*.cpp
        ../../src/reptoid/*.cpp
        ../../src/utils/*.cpp
        ../../src/timestamp.cpp
        ../../src/strings.cpp
)

add_executable(
        binance-order-book
        main.cpp
        ${COMMON_CPP_FILES}
)
target_link_libraries(
        binance-order-book
        ${CURL_LIBRARIES}
        ${OPENSSL_SSL_LIBRARY}
        ${OPENSSL_CRYPTO_LIBRARY}
        ${ZLIB_LIBRARIES}
        ${Boost_LIBRARIES}
        pthread dl
)

