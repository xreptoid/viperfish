project(viperfish)
cmake_minimum_required(VERSION 3.10)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

include_directories(
        include
        /usr/local/include
)
link_directories(
        /usr/local/lib
)

if(APPLE)
    set(OPENSSL_ROOT_DIR /usr/local/opt/openssl@1.1)
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)


find_package(CURL REQUIRED)
find_package(Poco REQUIRED Crypto)
find_package(Poco REQUIRED Foundation)
find_package(OpenSSL REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

file(GLOB_RECURSE COMMON_CPP_FILES src/*.cpp)
file(GLOB_RECURSE LIBTEST_CPP_FILES src_tests/*.cpp)

add_library(
        viperfish
        ${COMMON_CPP_FILES}
)

target_link_libraries(
        viperfish
        ${CURL_LIBRARIES}
        Poco::Foundation
        Poco::Crypto
)

file(GLOB_RECURSE TESTS_CPP_FILES tests/*_test.cpp)
add_library(
        viperfish_tests
        ${LIBTEST_CPP_FILES}
        ${COMMON_CPP_FILES}
)
target_link_libraries(
        viperfish_tests
        ${CURL_LIBRARIES}
        Poco::Foundation
        Poco::Crypto
)

add_executable(
        run_viperfish_tests
        tests/main.cpp
        ${TESTS_CPP_FILES}
        ${LIBTEST_CPP_FILES}
        ${COMMON_CPP_FILES}
)
target_link_libraries(
        run_viperfish_tests
        ${CURL_LIBRARIES}
        Poco::Foundation
        Poco::Crypto
)

include(GNUInstallDirs)

# installation
export(PACKAGE viperfish)
set_target_properties(viperfish PROPERTIES PUBLIC_HEADER include/viperfish.hpp)
install(DIRECTORY include/viperfish DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS viperfish EXPORT viperfishConfig)
install(EXPORT viperfishConfig DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/viperfish)

export(PACKAGE viperfish_tests)
set_target_properties(viperfish_tests PROPERTIES PUBLIC_HEADER include/viperfish_tests.hpp)
install(DIRECTORY include/viperfish_tests DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS viperfish_tests EXPORT viperfish_testsConfig)
install(EXPORT viperfish_testsConfig DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/viperfish_tests)
