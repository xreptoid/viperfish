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

    ObDiffQueue::ObDiffQueue(
        const std::string& symbol, int max_size
    )
        : symbol(symbol)
        , max_size(max_size)
    {}

    void ObDiffQueue::track(const OrderBookDiff& diff) {
        if (q.empty()) {
            return;
        }
        last_update_id = diff.final_update_id;
    }

    void ObDiffQueue::put(const OrderBookDiff& diff) {
        auto it = q.begin();
        while (it != q.end() && it->first_update_id < diff.first_update_id) {
            ++it;
        } 
        if (it != q.end() && it->first_update_id == diff.first_update_id) {
            return;
        }
        if (q.size() >= max_size) {
            std::cout << "ObDiffQueue(" << diff.symbol << "): queue limit(" << max_size << ") exceeded" << std::endl;
            return;
        }
        q.insert(it, diff);
    }

    OrderBookDiff* ObDiffQueue::next_diff() {
        if (q.empty()) {
            return NULL;
        }
        if (q.begin()->first_update_id <= *last_update_id + 1 || q.size() >= max_size) {
            // assume final_update_id > last_update_id
            return &(*q.begin());
        }
        return NULL;
    }

    void ObDiffQueue::release(OrderBookDiff* diff_p) {
        q.pop_front(); // assume diff_p is the first element
        last_update_id = diff_p->final_update_id;
    }

    void OrderBook::apply_diff(const OrderBookDiff& diff) {
        if (symbol != diff.symbol) {
            throw std::runtime_error(
                "OrderBook(" + symbol + "): "
                + "Applying diff with incorrect symbol: " + diff.symbol
            );
        }
        std::lock_guard lock(common_mutex);
        if (!last_update_id.has_value()) {
            throw std::runtime_error("OrderBook(" + symbol + "): Applying diff to not initialized orderbook");
        }
        if (diff.final_update_id <= *last_update_id) {
            return;
        }

        if (diff.first_update_id <= *last_update_id + 1) {
            apply_diff_body(diff);
            last_update_id = diff.final_update_id;
            diff_queue.track(diff);
        } else {
            diff_queue.put(diff);
        }
        OrderBookDiff* diff_p;
        while ((diff_p = diff_queue.next_diff())) {
            // inconsistance may be in the case of diff queue limit exceeded
            if (diff_p->first_update_id > *last_update_id + 1) {
                std::cout << (
                    "OrderBook(" + symbol + "): "
                    + "Applying diff with incorrect update id: "
                    + "diff.first_update_id(=" + std::to_string(diff_p->first_update_id) + ")"
                    + " > ob.last_update_id(=" + std::to_string(*last_update_id) + ") + 1"
                ) << std::endl;
            }

            apply_diff_body(*diff_p);
            last_update_id = diff_p->final_update_id;
            diff_queue.release(diff_p);
        }
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
        if (symbol != snapshot.symbol) {
            throw std::runtime_error(
                "OrderBook(" + symbol + "): "
                + "Applying snapshot with incorrect symbol: " + snapshot.symbol
            );
        }
        if (!snapshot.last_update_id.has_value()) {
            throw std::runtime_error("OrderBook(" + symbol + "): Applying snapshot without last_update_id");
        }
        std::lock_guard lock(common_mutex);
        for (const auto& bid: snapshot.get_bids()) {
            put_order(OrderSide::BUY, bid);
        }
        for (const auto& ask: snapshot.get_asks()) {
            put_order(OrderSide::SELL, ask);
        }
        last_update_id = snapshot.last_update_id;
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

    ObsContainer::ObsContainer(const std::vector<std::string>& symbols)
            : symbols(symbols)
    {
        for (const auto& symbol: symbols) {
            symbol2ob.insert(std::make_pair(symbol, OrderBook(symbol)));
        }
    }

    bool ObsContainer::put(const OrderBookDiff& ob_diff) {
        auto* ob = get(ob_diff.symbol);
        ob->apply_diff(ob_diff);
        return true;
    }

    bool ObsContainer::put_snapshot(const OrderBook& ob_snapshot) {
        auto* ob = get(ob_snapshot.symbol);
        ob->apply_snapshot(ob_snapshot);
        return true;
    }

    OrderBook* ObsContainer::get(const std::string& symbol) {
        auto it = symbol2ob.find(symbol);
        if (it == symbol2ob.end()) {
            throw std::runtime_error("ObsContainer: no symbol " + symbol);
        }
        return &it->second;
    }
}
