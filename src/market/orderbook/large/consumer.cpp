#include "./consumer.hpp"

namespace viperfish::market::orderbook::large {

    Event::Event(const std::string& trigger_symbol)
        : trigger_symbol(trigger_symbol)
    {}

    void Consumer::init() {
        // make symbols batches 
        // run consumers (binance)
        // download snapshots
        // download ob diffs tail
        // apply snapshots+tail
    }

    void Consumer::on_ob_diff(const market::orderbook::OrderBookDiff& ob_diff) {

        // snapshots+tail are ready?

        // call callbacks
    }
}