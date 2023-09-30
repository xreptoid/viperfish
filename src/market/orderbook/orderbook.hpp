#ifndef VIPERFISH_ORDERBOOK_ORDERBOOK_HPP
#define VIPERFISH_ORDERBOOK_ORDERBOOK_HPP
#include <map>
#include <thread>
#include <mutex>
#include <map>
#include <unordered_map>
#include <list>
#include <optional>
#include <vector>
#include "market/order.hpp"
#include "./order.hpp"

namespace viperfish::market::orderbook {

    class OrderBookSide {
    public:

        friend class OrderBook;

        OrderSide side;
        bool auto_remove_zero_amount;
       
        OrderBookSide(
            OrderSide side,
            const std::unordered_map<std::string, Order>& sprice2order,
            bool auto_remove_zero_amount = true
        )
                : side(side)
                , sprice2order(sprice2order)
                , auto_remove_zero_amount(auto_remove_zero_amount)
        {}

        explicit OrderBookSide(OrderSide side, bool auto_remove_zero_amount = true)
                : side(side)
                , auto_remove_zero_amount(auto_remove_zero_amount)
        {}
        void put(const Order&);

        std::size_t size() const { return sprice2order.size(); }
        std::vector<Order> get_orders(const std::optional<std::size_t>& max_count = {}) const;
        amount_t get_top_amount(fprice_t price) const;

    protected:
        std::unordered_map<std::string, Order> sprice2order;
        std::map<fprice_t, std::string> fprice2sprice;
    };

    class OrderBookDiff;

    class OrderBookBase {
    public:

        std::string symbol;
        std::optional<std::int64_t> last_id;
        bool auto_remove_zero_amount;

        OrderBookSide bids;
        OrderBookSide asks;

        OrderBookBase(
            const std::string& symbol,
            const std::optional<std::int64_t>& last_id = {},
            const OrderBookSide& bids = OrderBookSide(BUY),
            const OrderBookSide& asks = OrderBookSide(SELL),
            bool auto_remove_zero_amount = true
        )
                : symbol(symbol)
                , last_id(last_id)
                , bids(bids)
                , asks(asks)
                , auto_remove_zero_amount(auto_remove_zero_amount)
        
        {
            this->bids.auto_remove_zero_amount = auto_remove_zero_amount;
            this->asks.auto_remove_zero_amount = auto_remove_zero_amount;
        }

        OrderBookBase(
            const std::string& symbol,
            const std::optional<std::int64_t>& last_id = {},
            bool auto_remove_zero_amount = true
        )
                : symbol(symbol)
                , last_id(last_id)
                , bids(OrderBookSide(BUY, auto_remove_zero_amount))
                , asks(OrderBookSide(SELL, auto_remove_zero_amount))
                , auto_remove_zero_amount(auto_remove_zero_amount)
        
        {}

        OrderBookBase()
                : bids(OrderBookSide(BUY, true))
                , asks(OrderBookSide(SELL, true))
                , auto_remove_zero_amount(true)
        {}

        void put_order(OrderSide, const Order&);
        virtual std::vector<Order> get_bids(const std::optional<std::size_t>& max_count = {}) const {
            return bids.get_orders(max_count); };
        virtual std::vector<Order> get_asks(const std::optional<std::size_t>& max_count = {}) const {
            return asks.get_orders(max_count); };
    };

    class OrderBook : public OrderBookBase {
    public:
        std::optional<std::int64_t> last_diff_id;
        std::optional<std::int64_t> last_snapshot_id;
        std::optional<std::int64_t> timestamp;

        OrderBook(
            const std::string& symbol,
            const std::optional<std::int64_t>& last_id = {},
            const std::optional<std::int64_t>& timestamp = {}
        )
                : OrderBookBase(symbol, last_id, true)
                , timestamp(timestamp)
        {}

        OrderBook(): OrderBookBase() {}
        OrderBook(const OrderBook& other)
                : OrderBookBase(other.symbol, other.last_id, other.bids, other.asks) {}

        void apply_diff(const OrderBookDiff&);
        void apply_snapshot(const OrderBook&);
        amount_t get_top_amount(OrderSide, fprice_t) const;
        std::vector<Order> get_bids(const std::optional<std::size_t>& max_count = {}) const override;
        std::vector<Order> get_asks(const std::optional<std::size_t>& max_count = {}) const override;

    protected:
        mutable std::mutex common_mutex;

        void apply_diff_body(const OrderBookDiff&);
    };

    class OrderBookDiff : public OrderBookBase {
    public:
        OrderBookDiff(const std::string& symbol, const std::optional<std::int64_t>& last_id = {})
                : OrderBookBase(symbol, last_id, false) {}
    };

    class ObsContainer {
    public:

        ObsContainer() = default;
        virtual ~ObsContainer();

        bool put(const OrderBookDiff&);
        bool put_snapshot(const OrderBook&);
        OrderBook* get(const std::string&);

    protected:
        std::unordered_map<std::string, OrderBook> symbol2ob;
        std::unordered_map<std::string, std::mutex*> symbol2mutex;
        std::mutex meta_mutex;

        std::mutex* get_symbol_mutex(const std::string&);
    };
}

#endif 
