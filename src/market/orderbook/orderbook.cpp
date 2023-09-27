#include "./orderbook.hpp"
#include <cmath>
#include <iostream>

namespace viperfish::market::orderbook {

    void OrderBookSide::put(const Order& order) {
        auto old_it = sprice2order.find(order.sprice);

        if (old_it == sprice2order.end()) {
            if (auto_remove_zero_amount && std::abs(order.amount) < 1e-12) { // ??
                return;
            }
            sprice2order[order.sprice] = order;
            fprice2sprice[order.fprice] = order.sprice;
            return;
        }

        if (auto_remove_zero_amount && std::abs(order.amount) < 1e-12) {
            fprice2sprice.erase(old_it->second.fprice);
            sprice2order.erase(old_it);
            return;
        }

        old_it->second.amount = order.amount;
    }

    std::vector<Order> OrderBookSide::get_orders(const std::optional<std::size_t>& max_count) const {
        std::vector<Order> orders;
        auto it = fprice2sprice.begin();
        auto rit = fprice2sprice.rbegin();
        int i = 0;
        while ((side == BUY && rit != fprice2sprice.rend()) || (side == SELL && it != fprice2sprice.end())) {
            if (max_count.has_value() && i >= *max_count) {
                break;
            }

            std::string sprice = side == BUY
                    ? (rit++)->second
                    : (it++)->second;

            auto o_it = sprice2order.find(sprice);

            if (o_it != sprice2order.end()) {
                orders.push_back(o_it->second);
            } else {
                // ???
            }

            ++i;
        }
        return orders;
    }

    amount_t OrderBookSide::get_top_amount(fprice_t price) const {
        amount_t top_amount = 0;
        if (side == BUY) {
            auto it = fprice2sprice.rbegin();
            while (it != fprice2sprice.rend()) {
                if (it->first < price - 1e-12) {
                    break;
                }
                auto o_it = sprice2order.find(it->second);
                if (o_it != sprice2order.end()) {
                    top_amount += o_it->second.amount;
                } else {
                    // ???
                }
                ++it;
            }
        } else {
            auto it = fprice2sprice.begin();
            while (it != fprice2sprice.end()) {
                if (it->first > price + 1e-12) {
                    break;
                }
                auto o_it = sprice2order.find(it->second);
                if (o_it != sprice2order.end()) {
                    top_amount += o_it->second.amount;
                } else {
                    // ???
                }
                ++it;
            }
        }
        return top_amount;
    }

    int OrderBookSide::trunc_top(double price) {
        int count = 0;
        if (side == BUY) {
            while (!fprice2sprice.empty() && fprice2sprice.rbegin()->first > price + 1e-12) {
                sprice2order.erase(fprice2sprice.rbegin()->second);
                fprice2sprice.erase(fprice2sprice.rbegin()->first);
                ++count;
            }
        } else {
            while (!fprice2sprice.empty() && fprice2sprice.begin()->first < price - 1e-12) {
                sprice2order.erase(fprice2sprice.begin()->second);
                fprice2sprice.erase(fprice2sprice.begin()->first);
                ++count;
            }
        }
        return count;
    }

    void OrderBookBase::put_order(OrderSide order_side, const Order& order) {
        auto& side = order_side == OrderSide::BUY ? bids : asks;
        side.put(order);
    }

    void OrderBook::apply_diff(const OrderBookDiff& diff) {
        if (!diff.last_id.has_value()) {
            return;
        }
        std::lock_guard lock(common_mutex);
        if (last_diff_id.has_value() && *last_diff_id >= *diff.last_id) {
            return;
        }
        apply_diff_body(diff);

        /*
        while (!last_obts.empty() && last_obts.front().update_id <= diff.last_id) {
            last_obts.pop_front();
        }
        for (const auto& obt: last_obts) {
            apply_obt_body(obt);
        }
        last_diffs.push_back(diff);
        while (last_diffs.size() > 1000) {
            std::cout << "!!DIFFS LARGE" << std::endl;
            last_diffs.pop_front();
        }
        */
        last_diff_id = diff.last_id;
        relax_obts_prefix();
    }

    void OrderBook::apply_diff_body(const OrderBookDiff& diff) {
        for (const auto& bid: diff.get_bids()) {
            put_order(OrderSide::BUY, bid);
        }
        for (const auto& ask: diff.get_asks()) {
            put_order(OrderSide::SELL, ask);
        }
    }

    void OrderBook::apply_obt(const OrderBookTop& obt) {
        if (!obt.update_id.has_value()) {
            return;
        }
        std::lock_guard lock(obt_mutex);
        if (last_id.has_value() && *last_id >= *obt.update_id) {
            return;
        }
        //apply_obt_body(obt);

        last_id = obt.update_id;
        last_obts.push_back(obt);
        while (last_obts.size() > 10000) {
            std::cout << "order-book(" << symbol << ") OBTS LARGE " << last_obts.size() << std::endl;
            last_obts.pop_front();
        }
    }


    void OrderBook::apply_obt_body(const OrderBookTop& obt) {
        bids.trunc_top(obt.bid_o.fprice);
        bids.put(obt.bid_o);
        asks.trunc_top(obt.ask_o.fprice);
        asks.put(obt.ask_o);
    }


    void OrderBook::apply_snapshot(const OrderBook& snapshot) {
        if (!snapshot.last_id.has_value()) {
            return;
        }
        std::lock_guard lock(common_mutex);
        if (last_snapshot_id.has_value() && *last_snapshot_id >= *snapshot.last_id) {
            return;
        }

        for (const auto& bid: snapshot.get_bids()) {
            put_order(OrderSide::BUY, bid);
        }
        for (const auto& ask: snapshot.get_asks()) {
            put_order(OrderSide::SELL, ask);
        }

        /*
        while (!last_diffs.empty() && last_diffs.front().last_id < snapshot.last_id) {
            last_diffs.pop_front();
        }

        auto last_id_for_obt = snapshot.last_id;
        if (!last_diffs.empty() && last_diffs.back().last_id > last_id_for_obt) {
            last_id_for_obt = last_diffs.back().last_id;
        }

        while (!last_obts.empty() && last_obts.front().update_id <= last_id_for_obt) {
            last_obts.pop_front();
        }

        for (const auto& ob_diff: last_diffs) {
            apply_diff_body(ob_diff);
        }
        for (const auto& obt: last_obts) {
            apply_obt_body(obt);
        }
        */

        last_snapshot_id = snapshot.last_id;
        relax_obts_prefix();
    }

    amount_t OrderBook::get_top_amount(OrderSide order_side, fprice_t price) const {
        std::lock_guard lock(common_mutex);
        relax_obts();
        auto& side = order_side == OrderSide::BUY ? bids : asks;
        return side.get_top_amount(price);
    }

    void OrderBook::relax_obts_prefix() {
        std::lock_guard lock(obt_mutex);
        std::optional<std::int64_t> last_id_for_obt = last_diff_id;
        if (!last_id_for_obt.has_value() || (last_snapshot_id.has_value() && *last_snapshot_id > last_id_for_obt)) {
            last_id_for_obt = last_snapshot_id;
        }
        if (!last_id_for_obt.has_value()) {
            return;
        }
        while (!last_obts.empty() && last_obts.begin()->update_id <= *last_id_for_obt) {
            last_obts.pop_front();
        }
    }

    void OrderBook::relax_obts() {
        std::list<OrderBookTop> local_last_obts;
        {
            std::lock_guard lock(obt_mutex);
            if (last_obts.empty()) {
                return;
            }
            local_last_obts = last_obts;
            last_obts.clear();
        }
        std::optional<std::int64_t> last_id_for_obt = last_diff_id;
        if (!last_id_for_obt.has_value() || (last_snapshot_id.has_value() && *last_snapshot_id > last_id_for_obt)) {
            last_id_for_obt = last_snapshot_id;
        }
        for (const auto& obt: local_last_obts) {
            if (!last_id_for_obt.has_value() || obt.update_id > *last_id_for_obt) {
                apply_obt_body(obt);
            }
        }
    }

    std::vector<Order> OrderBook::get_bids(const std::optional<std::size_t>& max_count) const {
        relax_obts();
        return OrderBookBase::get_bids(max_count);
    }

    std::vector<Order> OrderBook::get_asks(const std::optional<std::size_t>& max_count) const {
        relax_obts();
        return OrderBookBase::get_asks(max_count);
    }

    ObsContainer::~ObsContainer() {
        for (const auto& [_, mutex]: symbol2mutex) {
            delete mutex;
        }
    }

    bool ObsContainer::put(const OrderBookDiff& ob_diff) {
        auto& ob = *get(ob_diff.symbol);
        ob.apply_diff(ob_diff);
        return true;
    }

    bool ObsContainer::put(const OrderBookTop& obt) {
        auto& ob = *get(obt.symbol);
        ob.apply_obt(obt);
        return true;
    }

    bool ObsContainer::put_snapshot(const OrderBook& ob_snapshot) {
        auto& ob = *get(ob_snapshot.symbol);
        ob.apply_snapshot(ob_snapshot);
        return true;
    }

    OrderBook* ObsContainer::get(const std::string& symbol) {
        std::lock_guard lock(*get_symbol_mutex(symbol));
        return &symbol2ob[symbol];
    }

    std::mutex* ObsContainer::get_symbol_mutex(const std::string& symbol) {
        std::lock_guard lock(meta_mutex);
        if (!symbol2mutex.count(symbol)) {
            symbol2mutex[symbol] = new std::mutex();
        }
        return symbol2mutex[symbol];
    }
}
