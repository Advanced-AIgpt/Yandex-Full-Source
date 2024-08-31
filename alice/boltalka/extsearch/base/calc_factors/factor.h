#pragma once
#include "factor_calcer_ctx.h"

#include <util/generic/ptr.h>

namespace NNlg {

class IFactor : public TThrRefBase {
public:
    virtual void AppendFactors(const TFactorCalcerCtx& ctx, TVector<TVector<float>>* factors) const = 0;
    virtual TString GetName() const = 0;
};

using IFactorPtr = TIntrusivePtr<IFactor>;

}
