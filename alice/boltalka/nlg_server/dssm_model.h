#pragma once
#include "nlg_model.h"
#include "context_transform.h"

#include <cv/imgclassifiers/danet/common/common.h>
#include <cv/imgclassifiers/danet/common/typedefs.h>
#include <cv/imgclassifiers/danet/config/config.h>
#include <cv/imgclassifiers/danet/data_provider/core/generalized_data_provider.h>
#include <cv/imgclassifiers/danet/data_provider/ext/text_utils/text_utils.h>
#include <cv/imgclassifiers/danet/nnlib/tester/ff_tester.h>

#include <dict/word2vec/model/model.h>
#include <dict/word2vec/util/analogy/invmi/searcher.h>

#include <util/folder/path.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NNlgServer {

using TFFTesterSPtr = TAtomicSharedPtr<NNeuralNet::TFeedFwdTester>;
using TRepliesWithEmbeddingsSPtr = TAtomicSharedPtr<NWord2Vec::TModel>;
using TAnnSearcherSPtr = TAtomicSharedPtr<TInvMiSearcher>;

class TDssmModel : public INlgModel {
public:
    TDssmModel(const TFsPath &modelDir, IContextTransformPtr contextTransform, const TString &contextFeaturesLayerName, ui64 numWorkers);
    TVector<TUtf16String> GetReplies(const TUtf16String &context, ui64 /*maxLen*/, double temperature, ui64 numSamples) const override;

private:
    NNeuralNet::TSampleSPtr GetSamplingSeed(const TUtf16String &context) const;

private:
    NNeuralNet::TConfigSPtr Config;
    IContextTransformPtr ContextTransform;
    TFFTesterSPtr FFTester;
    TString ContextFeaturesLayerName;
    NNeuralNet::TMasterEncoderSPtr DataPartsEncoder;

    TRepliesWithEmbeddingsSPtr RepliesWithEmbeddings;
    TAnnSearcherSPtr AnnSearcher;

    ui64 ContextLength;
};

}
