#include "binance/consumer.hpp"
#include "timestamp.hpp"
#include "json/getter.hpp"

namespace viperfish::binance {

    BinanceFill BinanceFill::parse(const json& data) {
        auto& idata = data.find("o") != data.end() && data["o"].is_object()
                ? data["o"]
                : data;
        return BinanceFill(
            json_field_get<std::string>(idata, "s"),
            json_field_get<std::string>(idata, "o"),
            json_field_get<std::string>(idata, "X"),
            std::stold(json_field_get<std::string>(idata, "l")), // amount
            std::stold(json_field_get<std::string>(idata, "L")), // price
            json_field_get<long long>(idata, "t"), // trade_id
            json_field_get<long long>(idata, "i"), // order_id
            json_field_get<long long>(idata, "T"), // ts
            get_current_ts()
        );
    }
}
