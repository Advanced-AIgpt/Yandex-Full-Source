#pragma once

#include <alice/boltalka/libs/text_utils/utterance_transform.h>

#include <kernel/dssm_applier/nn_applier/lib/layers.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/memory/blob.h>
#include <util/stream/input.h>

namespace NAlice {

class TBoltalkaDssmEmbedder {
public:
    struct TConfig {
        TVector<TString> Inputs;
        TVector<TString> Outputs;
        ELanguage Language = LANG_RUS;
        TVector<size_t> OutputVectorSizes;
    };

    TBoltalkaDssmEmbedder(const TBlob& modelBlob, IInputStream* jsonConfigStream);
    TBoltalkaDssmEmbedder(const TBlob& modelBlob, const TConfig& config);

    TVector<float> Embed(const TStringBuf query) const;

    TVector<TVector<float>> Embed(const TVector<TString>& inputs) const;

    static TConfig LoadConfig(IInputStream* jsonConfigStream);

private:
    NNeuralNetApplier::TModel Model;
    NNlgTextUtils::IUtteranceTransformPtr Transform;
    TConfig Config;
};

} // namespace NAlice
