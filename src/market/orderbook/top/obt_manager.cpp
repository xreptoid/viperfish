#include "./obt_manager.hpp"

namespace viperfish::market {

    ObtManager::ObtManager(
            const std::vector<std::string>& symbols,
            on_obt_callback_t& on_obt_callback
    )
            : symbols(symbols)
            , on_obt_callback(on_obt_callback)
    {
        for (const auto& symbol: symbols) {
            obts.emplace_back();
            auto* placed_obt = &obts.back();
            symbol2obt[symbol] = placed_obt;
            //symbol2obt[symbol] = std::mutex();
        }
    }

    void ObtManager::push_obt(const OrderBookTop& obt) {
        auto it = symbol2obt.find(obt.symbol);
        if (it == symbol2obt.end()) {
            // unsupported symbol
            return;
        }
        bool is_new = false;
        auto* placed_obt = it->second;
        if ((*placed_obt)->update_id.has_value() && obt.update_id.has_value()) {
            is_new = *obt.update_id > *(*placed_obt)->update_id;
        }

        if (!is_new) {
            return;
        }

        {
            std::lock_guard lock(symbol2new_obt_mutex[obt.symbol]);
            is_new = false;
            if ((*placed_obt)->update_id.has_value() && obt.update_id.has_value()) {
                is_new = *obt.update_id > *(*placed_obt)->update_id;
            }
            if (!is_new) {
                return;
            }
            *placed_obt = obt;
        }

        if (is_new) {
            std::lock_guard lock(symbols_set_for_callback_mutex);
            symbols_set_for_callback.insert(obt.symbol);
        }

        request_callback_execution();
    }

    void ObtManager::request_callback_execution() {

    }
}