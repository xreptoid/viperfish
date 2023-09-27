#include "./api.hpp"

namespace viperfish::reptoid::orderbook {

    Snapshots::Snapshots(const ob_map_t& ob_map)
        : ob_map(ob_map)
    {}

    Diffs::Diffs() {}
}
