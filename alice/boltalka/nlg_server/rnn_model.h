#pragma once
#include "nlg_model.h"
#include "context_transform.h"

#include <cv/imgclassifiers/danet/common/common.h>
#include <cv/imgclassifiers/danet/common/typedefs.h>
#include <cv/imgclassifiers/danet/config/config.h>
#include <cv/imgclassifiers/danet/data_provider/ext/text_utils/text_utils.h>
#include <cv/imgclassifiers/danet/nnlib/tester/rnn_tester.h>

#include <util/folder/path.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NNlgServer {

using TRnnTesterSPtr = TAtomicSharedPtr<NNeuralNet::TRnnTester>;
using TTokenDictSPtr = TAtomicSharedPtr<const NNeuralNet::TTokenDictionaryContainer>;

class TRnnModel : public INlgModel {
public:
    TRnnModel(const TFsPath &modelDir, IContextTransformPtr contextTransform, ui64 samplingSubNetId, ui64 numWorkers);
    TVector<TUtf16String> GetReplies(const TUtf16String &context, ui64 maxLen, double temperature, ui64 numSamples) const override;

private:
    NNeuralNet::TSampleSPtr GetSamplingSeed(const TUtf16String &context) const;

private:
    NNeuralNet::TConfigSPtr Config;
    IContextTransformPtr ContextTransform;
    TRnnTesterSPtr RnnTester;
    ui64 SamplingSubNetId;
    TTokenDictSPtr TokenDict;
};

}
