#include "viperfish/binance/exchange_info.hpp"
#include <iostream>

namespace viperfish::binance {

    BinanceExchangeInfo::BinanceExchangeInfo(BinanceMode mode, const json& data)
            : mode(mode)
            , data(data)
    {
        for (auto& symbol_data: data["symbols"]) {
            /*
                COIN-M:
                {"baseAsset":"BTC","baseAssetPrecision":8,"contractSize":100,"contractStatus":"TRADING","contractType":"PERPETUAL",
                "deliveryDate":4133404800000,"equalQtyPrecision":4,
                "filters":[
                    {"filterType":"PRICE_FILTER","maxPrice":"4520958","minPrice":"1000","tickSize":"0.1"},
                    {"filterType":"LOT_SIZE","maxQty":"1000000","minQty":"1","stepSize":"1"},
                    {"filterType":"MARKET_LOT_SIZE","maxQty":"60000","minQty":"1","stepSize":"1"},
                    {"filterType":"MAX_NUM_ORDERS","limit":200},
                    {"filterType":"MAX_NUM_ALGO_ORDERS","limit":100},
                    {"filterType":"PERCENT_PRICE","multiplierDecimal":"4","multiplierDown":"0.9500","multiplierUp":"1.0500"}],
                "liquidationFee":"0.015000","maintMarginPercent":"2.5000","marginAsset":"BTC","marketTakeBound":"0.05","maxMoveOrderLimit":10000,
                "onboardDate":1597042800000,
                orderTypes":["LIMIT","MARKET","STOP","STOP_MARKET","TAKE_PROFIT","TAKE_PROFIT_MARKET","TRAILING_STOP_MARKET"],
                "pair":"BTCUSD","pricePrecision":1,"quantityPrecision":0,"quoteAsset":"USD","quotePrecision":8,"requiredMarginPercent":"5.0000",
                "symbol":"BTCUSD_PERP","timeInForce":["GTC","IOC","FOK","GTX"],"triggerProtect":"0.0500","underlyingSubType":["PoW"],"underlyingType":"COIN"}
                
                USDS-M:
                "{baseAsset":"BTC","baseAssetPrecision":8,
                "contractType":"PERPETUAL","deliveryDate":4133404800000,
                "filters":[
                    {"filterType":"PRICE_FILTER","maxPrice":"4529764","minPrice":"556.80","tickSize":"0.10"},
                    {"filterType":"LOT_SIZE","maxQty":"1000","minQty":"0.001","stepSize":"0.001"},
                    {"filterType":"MARKET_LOT_SIZE","maxQty":"120","minQty":"0.001","stepSize":"0.001"},
                    {"filterType":"MAX_NUM_ORDERS","limit":200},
                    {"filterType":"MAX_NUM_ALGO_ORDERS","limit":10},
                    {"filterType":"MIN_NOTIONAL","notional":"5"},
                    {"filterType":"PERCENT_PRICE","multiplierDecimal":"4","multiplierDown":"0.9500","multiplierUp":"1.0500"}],
                "liquidationFee":"0.017500","maintMarginPercent":"2.5000","marginAsset":"USDT",
                "marketTakeBound":"0.05","onboardDate":1569398400000,
                "orderTypes":["LIMIT","MARKET","STOP","STOP_MARKET","TAKE_PROFIT","TAKE_PROFIT_MARKET","TRAILING_STOP_MARKET"],
                "pair":"BTCUSDT","pricePrecision":2,"quantityPrecision":3,"quoteAsset":"USDT","quotePrecision":8,
                "requiredMarginPercent":"5.0000","settlePlan":0,
                "status":"TRADING","symbol":"BTCUSDT",
                "timeInForce":["GTC","IOC","FOK","GTX"],
                "triggerProtect":"0.0500","underlyingSubType":["PoW"],"underlyingType":"COIN"}
            */
            std::string symbol = json_field_get<std::string>(symbol_data, "symbol");
            std::string base_asset = json_field_get<std::string>(symbol_data, "baseAsset");
            std::string quote_asset = json_field_get<std::string>(symbol_data, "quoteAsset");
            std::string symbol_orig = base_asset + "/" + quote_asset;
            bool is_enabled = json_field_get<std::string>(symbol_data, mode == FUTURES_DELIVERY ? "contractStatus" : "status") == "TRADING";
            bool is_margin_enabled = false;
            if (is_enabled && (mode == SPOT || mode == CROSS_MARGIN || mode == ISOLATED_MARGIN)) {
                for (const auto& p: json_field_get<std::vector<std::string>>(symbol_data, "permissions")) {
                    if (p == "MARGIN") {
                        is_margin_enabled = true;
                        break;
                    }
                }
            }

            std::string price_step_str;
            std::string amount_step_str;
            std::string min_amount_str;
            for (auto& filter: symbol_data["filters"]) {
                std::string filter_type = filter["filterType"];
                if (filter_type == "PRICE_FILTER") {
                    price_step_str = filter["tickSize"];
                } else if (filter_type == "LOT_SIZE") {
                    amount_step_str = filter["stepSize"];
                    if (mode == FUTURES_DELIVERY) {
                        min_amount_str = json_field_get<std::string>(filter, "minQty");
                    }
                } else if (filter_type == "MIN_NOTIONAL" || filter_type == "NOTIONAL") {
                    min_amount_str = mode == FUTURES
                            ? filter["notional"]
                            : filter["minNotional"];
                }
            }
            assert(!price_step_str.empty());
            assert(!amount_step_str.empty());
            assert(!min_amount_str.empty());
            try {
                int quote_asset_precision = symbol_data.find("quoteAssetPrecision") != symbol_data.end()
                        ? symbol_data["quoteAssetPrecision"]
                        : symbol_data["quotePrecision"];
                symbol2meta[symbol] = SymbolMeta(
                        market::Symbol(base_asset, quote_asset),
                        symbol,
                        std::stod(price_step_str),
                        get_number_precision(price_step_str),
                        std::stod(amount_step_str),
                        get_number_precision(amount_step_str),
                        quote_asset_precision,
                        std::stod(min_amount_str)
                );
            } catch (const std::exception& e) {
                std::cout << "BinanceExchangeInfo: Error on SymbolMeta for " << symbol << ": " << e.what() << std::endl;
                std::cout << price_step_str << " " << amount_step_str << " " << min_amount_str << std::endl;
                std::cout << symbol_data["filters"].dump() << std::endl;
                std::cout << "ignoring " << symbol << std::endl;
                continue;
            }

            if (is_enabled) {
                bool is_spot_enabled = true;
                if (mode == FUTURES) {
                    is_spot_enabled = symbol_data["contractType"] == "PERPETUAL";
                }
                if (is_spot_enabled) {
                    spot_symbols.emplace_back(base_asset, quote_asset);
                }
                if (is_margin_enabled) {
                    margin_symbols.emplace_back(base_asset, quote_asset);
                }
            }
        }
    }
}