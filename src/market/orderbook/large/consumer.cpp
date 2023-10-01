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

    void Consumer::set_ob_snapshots(const Snapshots& snapshots) {
        std::lock_guard lock(init_data_mutex);
        this->ob_snapshots.emplace(snapshots);
        this->try_init_data();
    }

    void Consumer::set_ob_diffs_tail(const ObDiffsTail& ob_diffs_tail) {
        std::lock_guard lock(init_data_mutex);
        this->ob_diffs_tail = ob_diffs_tail;
        this->try_init_data();
    }

    void Consumer::try_init_data() {
        if (!ob_snapshots.has_value()) {
            return;
        }
        if (!ob_diffs_tail.has_value()) {
            return;
        }
        for (const auto& [symbol, snapshot]: ob_snapshots->ob_map) {
            this->obs_container.put_snapshot(snapshot);
        }
        for (const auto& ob_diff: ob_diffs_tail->ob_diffs) {
            this->obs_container.put(ob_diff);
        }
        for (const auto& ob_diff: ob_diffs_init_buffer) {
            this->obs_container.put(ob_diff);
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