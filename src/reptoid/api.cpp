#include "./api.hpp"

namespace viperfish::reptoid {

    Api::Api() {
        update_endpoints();
    }

    Api::~Api() {}

    orderbook::Snapshots Api::get_snapshots() {
        return orderbook::Snapshots({});
    }

    orderbook::Diffs Api::get_ob_diffs() {
        return orderbook::Diffs();
    }

    void Api::update_endpoints() {
    }
}
