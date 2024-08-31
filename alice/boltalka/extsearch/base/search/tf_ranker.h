#pragma once

#include <alice/nlu/libs/tf_nn_model/tf_nn_model.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>

namespace NNlg {

class TTfRanker : public NVins::TTfNnModel, public TThrRefBase {
public:
    TTfRanker(const TString& modelDirName);

    void Predict(const float* queryEmbedding, const TVector<const float*>& replyEmbeddings, size_t dimension, const TVector<double>& relevs, TVector<double>* score, bool boostTop, bool boostRelev);

private:
    void SaveModelParameters(const TString&) const override {}
};

using TTfRankerPtr = TIntrusivePtr<TTfRanker>;

}
