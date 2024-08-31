#include "fst_map_mixin.h"

#include <library/cpp/json/json_reader.h>

namespace NAlice {

    TFstMapMixin::TFstMapMixin(const IDataLoader& loader)
    {
        auto map = loader.GetInputStream("maps.json");
        auto weights = loader.Has("weights.json") ? loader.GetInputStream("weights.json") : THolder<IInputStream>{};
        LoadMapAndWeights(*map, weights.Get());
    }

    TMaybe<TValueAndWeight> TFstMapMixin::FindCanonicalValueAndWeight(const TString& value) const
    {
        auto f = CanonicalValuesAndWeights.find(value);
        if (f == end(CanonicalValuesAndWeights)) {
            return {};
        }
        return f->second;
    }

    void TFstMapMixin::LoadMapAndWeights(IInputStream& map, IInputStream* weights) {
        using namespace NJson;
        TJsonValue canonicalMap;
        if (!ReadJsonTree(&map, &canonicalMap)) {
            ythrow yexception() << "Failed to load canonical map";
        }
        TJsonValue weightMap{JSON_MAP};
        if (weights) {
            if (!ReadJsonTree(weights, &weightMap)) {
                ythrow yexception() << "Failed to load weight map";
            }
        }
        auto& w = weightMap.GetMapSafe();
        for (const auto& pair : canonicalMap.GetMapSafe()) {
            TValueAndWeight valueAndWeight;
            if (pair.second.IsMap()) {
                for (const auto& subPair : pair.second.GetMap()) {
                    valueAndWeight = TValueAndWeight{};
                    const auto& type = pair.first;
                    const auto& canonical = subPair.second.GetStringSafe();
                    const auto& weightKey = type + '_' + canonical;
                    const auto& mainKey = type + '_' + subPair.first;
                    valueAndWeight.Value = canonical;
                    auto weightSubmap = w.find(weightKey);
                    if (weightSubmap != end(w)) {
                        const auto& v = weightSubmap->second.GetMapSafe();
                        auto f = v.find(subPair.first);
                        if (f != end(v)) {
                            valueAndWeight.Weight = f->second.GetDoubleSafe();
                        }
                    }
                    CanonicalValuesAndWeights.emplace(mainKey, std::move(valueAndWeight));
                }
                continue;
            }

            const auto& canonicalValue = pair.second.GetStringSafe();
            valueAndWeight.Value = canonicalValue;
            auto weightSubmap = w.find(canonicalValue);
            if (weightSubmap != end(w)) {
                const auto& v = weightSubmap->second.GetMapSafe();
                auto f = v.find(pair.first);
                if (f != end(v)) {
                    valueAndWeight.Weight = f->second.GetDoubleSafe();
                }
            }
            CanonicalValuesAndWeights.emplace(pair.first, std::move(valueAndWeight));
        }
    }

} // namespace NAlice
