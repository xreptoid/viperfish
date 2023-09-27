#ifndef INCLUDE_VIPERFISH_MARKET_OBT_MANAGER_HPP
#define INCLUDE_VIPERFISH_MARKET_OBT_MANAGER_HPP

#include "./obt.hpp"

#include <string>
#include <functional>
#include <vector>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <mutex>

#define VIPERFISH_OBT_MANAGER_S2OBT_BUFS_N 2

namespace viperfish::market {

    class ObtManager {
    public:

        typedef std::function<void(const std::vector<OrderBookTop>& obts)>& on_obt_callback_t;

        ObtManager(
                const std::vector<std::string>& symbols,
                on_obt_callback_t& on_obt_callback
        );

        void push_obt(const OrderBookTop& obt);

    private:
        std::vector<std::string> symbols;

        on_obt_callback_t on_obt_callback;

        std::vector<std::optional<OrderBookTop>> obts;
        std::unordered_map<std::string, std::optional<OrderBookTop>*> symbol2obt;
        std::unordered_map<std::string, std::mutex> symbol2new_obt_mutex;

        std::mutex symbols_set_for_callback_mutex;
        std::unordered_set<std::string> symbols_set_for_callback;

        void request_callback_execution();
    };
}

#endif 
