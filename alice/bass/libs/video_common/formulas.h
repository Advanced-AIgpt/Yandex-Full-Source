#pragma once

#include <alice/bass/libs/logging_v2/logger.h>

#include <catboost/libs/model/model.h>

#include <util/generic/array_ref.h>
#include <util/generic/fwd.h>
#include <util/generic/maybe.h>
#include <util/generic/ptr.h>
#include <util/generic/singleton.h>
#include <util/generic/vector.h>

#include <cmath>

namespace NVideoCommon {

namespace NHasGoodResult {
struct TFactors;
}

namespace NShowOrGallery {
struct TFactors;
}

class TFormulas {
public:
    static TFormulas* Instance() {
        return Singleton<TFormulas>();
    }

    void Init();

    TMaybe<double> GetProb(const NHasGoodResult::TFactors& factors) const;
    TMaybe<double> GetProb(const NShowOrGallery::TFactors& factors) const;

private:
    template <typename TFactors>
    static TMaybe<double> GetProbImpl(const std::unique_ptr<TFullModel>& formula, const TFactors& factors) {
        try {
            if (!formula)
                return {};

            TVector<float> values;
            factors.GetValues(values);

            double relevs[1] = {};
            formula->CalcFlat(TConstArrayRef<float>(values.begin(), values.end()), TArrayRef<double>(relevs));
            const double relev = relevs[0];

            return 1.0 / (1.0 + exp(-relev));
        } catch (...) {
            LOG(ERR) << "Failed to apply formula" << Endl;
            return {};
        }
    }

    std::unique_ptr<TFullModel> ShowOrGallery;
    std::unique_ptr<TFullModel> HasGoodResult;
};

} // namespace NVideoCommon
