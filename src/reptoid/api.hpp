#ifndef VIPERFISH_REPTOID_HPP
#define VIPERFISH_REPTOID_HPP
#include <unordered_map>
#include "market/orderbook/large/consumer.hpp"

namespace viperfish::reptoid {

    class Api {
    public:
        Api();
        virtual ~Api();

        virtual market::orderbook::large::Snapshots get_snapshots();
        virtual market::orderbook::large::ObDiffsTail get_ob_diffs_tail(std::int64_t ts_from, std::int64_t ts_to);

    protected:
        virtual void update_endpoints();
    };
}

#endif