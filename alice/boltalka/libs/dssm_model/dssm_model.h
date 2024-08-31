#pragma once
#include <alice/boltalka/libs/text_utils/utterance_transform.h>
#include <alice/boltalka/libs/text_utils/context_transform.h>

#include <ml/dssm/dssm/lib/dssm.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NNlg {

class TDssmModel : public TThrRefBase {
public:
    TDssmModel(const TString& modelFilename);
    TDssmModel(NDssm3::TModelFactoryPtr<float> modelFactory);

    // not thread-safe
    TVector<TVector<float>> Fprop(const TVector<TString>& context, const TString& reply, const TVector<TString>& output) const;
    // not thread-safe
    TVector<TVector<float>> FpropBatch(const TVector<TVector<TString>>& contexts, const TVector<TString>& replies, const TVector<TString>& output) const;

private:
    NNlgTextUtils::TReplacePunct4Insight ReplacePunct;
    THolder<NNlgTextUtils::TSetContextNumTurns> SetNumTurns;
    bool IsAggregatingModel;

    NDssm3::TSampleProcessor SampleReader;
    NDssm3::IModelPtr<float> Model;
};

using TDssmModelPtr = TIntrusivePtr<TDssmModel>;

}
