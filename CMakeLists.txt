project(viperfish)
cmake_minimum_required(VERSION 3.10)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

include_directories(
        include
        src
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

file(GLOB_RECURSE COMMON_CPP_FILES src/*.cpp)

set(BUILD_SHARED_LIBS OFF)

add_library(
        viperfish
        STATIC
        ${COMMON_CPP_FILES}
)

target_link_libraries(
        viperfish
        PUBLIC
        ${CURL_LIBRARIES}
        ${OPENSSL_SSL_LIBRARY}
        ${OPENSSL_CRYPTO_LIBRARY}
        ${ZLIB_LIBRARIES}
        ${Boost_LIBRARIES}
        pthread dl
)


include(GNUInstallDirs)

export(PACKAGE viperfish)
#set_target_properties(viperfish PROPERTIES PUBLIC_HEADER include/viperfish.hpp)
install(DIRECTORY include/viperfish DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS viperfish EXPORT viperfishConfig)
install(EXPORT viperfishConfig DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/viperfish)
