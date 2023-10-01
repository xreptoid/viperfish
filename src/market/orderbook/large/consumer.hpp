#ifndef VIPERFISH_MARKET_DATA_ORDER_BOOK_LARGE_CONSUMER_HPP
#define VIPERFISH_MARKET_DATA_ORDER_BOOK_LARGE_CONSUMER_HPP
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <functional>
#include "market/orderbook/orderbook.hpp"

namespace viperfish::market::orderbook::large {

    class Snapshots {
    public:
        typedef std::unordered_map<std::string, OrderBook> ob_map_t;
        ob_map_t ob_map;

        Snapshots(const ob_map_t&);
    };

    class ObDiffsTail {
    public:
        std::vector<OrderBookDiff> ob_diffs;

        ObDiffsTail(const std::vector<OrderBookDiff>&);
    };

    class Event {
    public:
        std::string trigger_symbol;

        Event(const std::string&);
    };

    class Consumer {
    public:
        market::orderbook::ObsContainer obs_container;

        Consumer(const std::vector<std::string>& symbols);

        virtual void set_ob_snapshots(const Snapshots&);
        virtual void set_ob_diffs_tail(const ObDiffsTail&);
        virtual void try_init_data();

        virtual void push_ob_diff(const OrderBookDiff&);

    protected:
        std::vector<std::string> symbols;
        std::unordered_set<std::string> symbols_set;

        volatile bool is_ready = false;
        std::mutex init_data_mutex;
        std::optional<Snapshots> ob_snapshots;
        std::optional<ObDiffsTail> ob_diffs_tail;
        std::vector<OrderBookDiff> ob_diffs_init_buffer;
    };
}

#endif