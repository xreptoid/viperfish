#ifndef INCLUDE_VIPERFISH_MARKET_SYMBOL_HPP
#define INCLUDE_VIPERFISH_MARKET_SYMBOL_HPP

#include <string>
#include <utility>
#include "strings.hpp"

namespace viperfish::market {

    class Symbol {
    public:
        std::string base_asset;
        std::string quote_asset;
        std::string base_currency; // FIXME remove it
        std::string quote_currency; // FIXME remove it

        Symbol(
                const std::string& base_asset,
                const std::string& quote_asset
        )
                : base_asset(base_asset)
                , quote_asset(quote_asset)
                , base_currency(base_asset)
                , quote_currency(quote_asset)
        {}

        std::pair<std::string, std::string> as_pair() const {
            return {base_asset, quote_asset};
        }

        std::vector<std::string> get_currencies_vector() const {
            return {base_asset, quote_asset};
        }

        bool has_currency(const std::string& asset) const { return has_asset(asset); } // FIXME remove it

        bool has_asset(const std::string& asset) const {
            return base_asset == asset || quote_asset == asset;
        }

        std::string binance() const {
            return to_upper(base_asset + quote_asset);
        }
    };
}

#endif 
