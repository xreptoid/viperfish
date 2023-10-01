#include "./consumer.hpp"

namespace viperfish::market::orderbook::large {

    Snapshots::Snapshots(const ob_map_t& ob_map)
        : ob_map(ob_map)
    {}

    ObDiffsTail::ObDiffsTail(const std::vector<OrderBookDiff>& ob_diffs)
        : ob_diffs(ob_diffs)
    {}

    Event::Event(const std::string& trigger_symbol)
        : trigger_symbol(trigger_symbol)
    {}

    Consumer::Consumer(const std::vector<std::string>& symbols)
        : symbols(symbols)
        , obs_container(symbols)
    {
        symbols_set = std::unordered_set(symbols.begin(), symbols.end());
    }

    void Consumer::set_ob_snapshots(const Snapshots& snapshots) {
        this->ob_snapshots.emplace(snapshots);
        this->try_init_data();
    }

    void Consumer::set_ob_diffs_tail(const ObDiffsTail& ob_diffs_tail) {
        this->ob_diffs_tail = ob_diffs_tail;
        this->try_init_data();
    }

    void Consumer::try_init_data() {
        std::lock_guard lock(init_data_mutex);
        if (!ob_snapshots.has_value()) {
            return;
        }
        if (!ob_diffs_tail.has_value()) {
            return;
        }
        std::unordered_map<std::string, std::uint64_t> symbol2last_update_id;
        for (const auto& [symbol, snapshot]: ob_snapshots->ob_map) {
            if (!symbols_set.count(symbol)) {
                continue;
            }
            this->obs_container.put_snapshot(snapshot);
            symbol2last_update_id[symbol] = *snapshot.last_update_id;
        }
        for (const auto& ob_diff: ob_diffs_tail->ob_diffs) {
            if (!symbols_set.count(ob_diff.symbol) || ob_diff.final_update_id <= symbol2last_update_id[ob_diff.symbol]) {
                continue;
            } 
            this->obs_container.put(ob_diff);
            symbol2last_update_id[ob_diff.symbol] = ob_diff.final_update_id;
        }
        for (const auto& ob_diff: ob_diffs_init_buffer) {
            if (!symbols_set.count(ob_diff.symbol) || ob_diff.final_update_id <= symbol2last_update_id[ob_diff.symbol]) {
                continue;
            } 
            this->obs_container.put(ob_diff);
            symbol2last_update_id[ob_diff.symbol] = ob_diff.final_update_id;
        }
        is_ready = true;
    }

    void Consumer::push_ob_diff(const OrderBookDiff& ob_diff) {
        if (!is_ready) {
            std::lock_guard lock(init_data_mutex);
            if (!is_ready) {
                this->ob_diffs_init_buffer.push_back(ob_diff);
                return;
            }
        }

        this->obs_container.put(ob_diff);
    }
}