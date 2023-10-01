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

    void OrderBookBase::put_order(OrderSide order_side, const Order& order) {
        auto& side = order_side == OrderSide::BUY ? bids : asks;
        side.put(order);
    }

    void OrderBook::apply_diff(const OrderBookDiff& diff) {
        std::lock_guard lock(common_mutex);
        apply_diff_body(diff);
        last_diff_id = diff.last_id;
    }

    void OrderBook::apply_diff_body(const OrderBookDiff& diff) {
        for (const auto& bid: diff.get_bids()) {
            put_order(OrderSide::BUY, bid);
        }
        for (const auto& ask: diff.get_asks()) {
            put_order(OrderSide::SELL, ask);
        }
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

        last_snapshot_id = snapshot.last_id;
    }

    amount_t OrderBook::get_top_amount(OrderSide order_side, fprice_t price) const {
        std::lock_guard lock(common_mutex);
        auto& side = order_side == OrderSide::BUY ? bids : asks;
        return side.get_top_amount(price);
    }

    std::vector<Order> OrderBook::get_bids(const std::optional<std::size_t>& max_count) const {
        std::lock_guard lock(common_mutex);
        return OrderBookBase::get_bids(max_count);
    }

    std::vector<Order> OrderBook::get_asks(const std::optional<std::size_t>& max_count) const {
        std::lock_guard lock(common_mutex);
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
