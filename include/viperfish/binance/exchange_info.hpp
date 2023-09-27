#ifndef INCLUDE_VIPERFISH_BINANCE_EXCHANGE_INFO_HPP
#define INCLUDE_VIPERFISH_BINANCE_EXCHANGE_INFO_HPP
#include <mutex>
#include <string>
#include <optional>
#include <unordered_map>
#include "viperfish/json.hpp"
#include "viperfish/numbers.hpp"
#include "viperfish/market/symbol.hpp"
#include "viperfish/timestamp.hpp"
#include "./mode.hpp"

namespace viperfish::binance {

    class BinanceExchangeInfo {
    public:
        BinanceMode mode;
        json data;

        class SymbolMeta {
        public:

            market::Symbol xsymbol;
            std::string symbol;
            double price_step;
            int price_step_precision;
            double amount_step;
            int amount_step_precision;
            int quote_asset_precision;

            double min_amount;

            SymbolMeta(
                    const market::Symbol& xsymbol,
                    const std::string& symbol,
                    double price_step,
                    int price_step_precision,
                    double amount_step,
                    int amount_step_precision,
                    int quote_asset_precision,
                    double min_amount
            )
                    : xsymbol(xsymbol)
                    , symbol(symbol)
                    , price_step(price_step)
                    , price_step_precision(price_step_precision)
                    , amount_step(amount_step)
                    , amount_step_precision(amount_step_precision)
                    , quote_asset_precision(quote_asset_precision)
                    , min_amount(min_amount)
            {}
        };

        std::unordered_map<std::string, std::optional<SymbolMeta>> symbol2meta;
        std::vector<market::Symbol> spot_symbols;
        std::vector<market::Symbol> margin_symbols;

        BinanceExchangeInfo(BinanceMode mode, const json& data);
    };

    class BinanceExchangeInfoCache {
    public:

        class Cache {
        public:
            std::mutex mutex;
            std::uint64_t last_ts = 0;
            std::optional<BinanceExchangeInfo> ei;

            Cache() = default;

            BinanceExchangeInfo get_or_update(const std::function<BinanceExchangeInfo()>& fetcher) {
                std::lock_guard lock(mutex);
                if (last_ts < get_current_ts() - 5 * 60 * 1000) {
                    ei = fetcher();
                    last_ts = get_current_ts();
                }
                return *ei;
            }
        };

        BinanceExchangeInfoCache() = default;

        BinanceExchangeInfo get_or_update(BinanceMode mode, const std::function<BinanceExchangeInfo()>& fetcher) {
            std::lock_guard lock(mutex);
            return mode2cache[mode].get_or_update(fetcher);
        }

    protected:
        std::unordered_map<BinanceMode, Cache> mode2cache;
        std::mutex mutex;
    };
}
#endif 
