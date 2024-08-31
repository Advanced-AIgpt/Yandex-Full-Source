#include "mixed_input.h"

#include <util/string/split.h>

namespace NAlice {

using namespace std::literals;

namespace {
    template<class T>
    inline float ToFloat(T x) {
        return static_cast<float>(x);
    }

    inline void UpdateMax(float& dst, float src) {
        dst = Max(dst, src);
    }
}

THashMap<TString, float> CalculateSentenceFeatures(TStringBuf text,
    const TVector<NGranet::TEntity>& entities)
{
    THashMap<TString, float> features;
    const float tokenCount = ToFloat(Max<int>(1, StringSplitter(text).Split(' ').Count()));
    for (const NGranet::TEntity& entity : entities) {
        UpdateMax(features[entity.Type + ".relative_length"sv], ToFloat(entity.Interval.Length()) / tokenCount);
        float quality = ToFloat(entity.Quality);
        if (quality == 0 && (entity.Type.StartsWith("custom."sv) || entity.Type.StartsWith("sys."sv))) {
            quality = 1.f;
        }
        UpdateMax(features[entity.Type + ".quality"sv], quality);
    }
    return features;
}

} // namespace NAlice
