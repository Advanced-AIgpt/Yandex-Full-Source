#include "fst_geo.h"

#include <util/string/cast.h>
#include <util/string/strip.h>

namespace NAlice {

    TParsedToken::TValueType TFstGeo::ProcessValue(const TParsedToken::TValueType& value) const {
        if (!value.IsArray()) {
            return value;
        }
        const auto& array = value.GetArray();
        Y_ENSURE(array.size() % 2 == 0);
        NSc::TValue out;
        for (auto i = 0u; i < array.size(); i += 2u) {
            auto type = StripStringRight(array[i].GetString(), EqualsStripAdapter('='));
            auto value = array[i + 1];
            const char* const regionTypes[] = {"continent", "country", "city", "metro_station"};
            using std::begin, std::end;
            if (std::find(begin(regionTypes), end(regionTypes), type) != end(regionTypes)) {
                out[type]["id"] = value;
                auto valueAndWeight = FindCanonicalValueAndWeight(ToString(value));
                if (valueAndWeight) {
                    out[type]["name"] = valueAndWeight->Value;
                }
            } else {
                if (value.IsString()) {
                    value = Collapse(Strip(ToString(value.GetString())));
                } else if (value.IsIntNumber() && type == "street") {
                    value = ToString(value.GetIntNumber());
                }
                out[type] = value;
            }
        }

        return out;
    }

} // namespace NAlice
