#include "nlg_model.h"

#include <cv/imgclassifiers/danet/data_provider/ext/text_data_provider.h>
#include <cv/imgclassifiers/danet/data_provider/ext/encoders/config_factory.h>

#include <cv/imgclassifiers/danet/data_provider/core/readers/memory.h>
#include <cv/imgclassifiers/danet/data_provider/core/generalized_data_provider.h>

#include <util/charset/wide.h>

namespace NNlgServer {

using namespace NNeuralNet;

namespace {

static ui64 CountEos(TWtringBuf context) {
    ui64 count = 0;
    for (TWtringBuf tok; context.NextTok(u"_EOS_", tok); ) {
        ++count;
    }
    return count;
}

}

TEveryContextLengthModel::TEveryContextLengthModel(TVector<INlgModelSPtr> nlgModels)
    : NlgModels(std::move(nlgModels))
{
}

TVector<TUtf16String> TEveryContextLengthModel::GetReplies(const TUtf16String &context, ui64 maxLen, double temperature, ui64 numSamples) const {
    ui64 contextLen = CountEos(context);
    contextLen = Min(contextLen, NlgModels.size());
    return NlgModels[contextLen - 1]->GetReplies(context, maxLen, temperature, numSamples);
}

}

