#pragma once

#include <alice/nlu/libs/embedder/embedder.h>
#include <alice/nlu/libs/encoder/encoder.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/input.h>

namespace NAlice {

class TLstmBasedBinaryClassifier {
public:
    TLstmBasedBinaryClassifier(const NAlice::TTokenEmbedder& embedder, IInputStream* protobufModelStream,
                               IInputStream* jsonConfigStream);

    float PredictProbability(const TString& request) const;

private:
    NVins::TEncoderInput GetModelFeed(const TString& request) const;

private:
    const NAlice::TTokenEmbedder& Embedder;
    NVins::TEncoder Encoder;
};

using TLstmBasedBinaryClassifierPtr = THolder<TLstmBasedBinaryClassifier>;

} // namespace NAlice
