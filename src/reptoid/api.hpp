#ifndef VIPERFISH_REPTOID_HPP
#define VIPERFISH_REPTOID_HPP
#include <unordered_map>
#include "market/orderbook.hpp"

namespace viperfish::reptoid {

    namespace orderbook {
        class Snapshots {
        public:
            typedef std::unordered_map<std::string, market::orderbook::OrderBook> ob_map_t;

            ob_map_t ob_map;

            Snapshots(const ob_map_t&);
        };

        class Diffs {
        public:
            Diffs();
        };
    }

    class Api {
    public:
        Api();
        virtual ~Api();

        virtual orderbook::Snapshots get_snapshots();
        virtual orderbook::Diffs get_ob_diffs();

    protected:
        virtual void update_endpoints();
    };
}

#endif