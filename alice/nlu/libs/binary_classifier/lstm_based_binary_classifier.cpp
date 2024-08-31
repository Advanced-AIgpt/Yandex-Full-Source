#include "lstm_based_binary_classifier.h"

#include <library/cpp/json/json_reader.h>

#include <util/string/split.h>

namespace NAlice {

namespace {

NVins::TEncoderDescription LoadEncoderDescription(IInputStream* jsonConfigStream) {
    Y_ENSURE(jsonConfigStream);

    NJson::TJsonValue jsonConfig;
    const bool readCorrectly = NJson::ReadJsonTree(jsonConfigStream, &jsonConfig);
    Y_ENSURE(readCorrectly);

    NVins::TEncoderDescription description;
    for (const auto& [key, value] : jsonConfig["inputs"].GetMapSafe()) {
        description.InputsMapping[key] = {value.GetStringSafe()};
    }
    description.Output = jsonConfig["outputs"]["class_probs"].GetStringSafe();

    return description;
}

} // namespace

TLstmBasedBinaryClassifier::TLstmBasedBinaryClassifier(const NAlice::TTokenEmbedder& embedder,
                                                       IInputStream* protobufModelStream,
                                                       IInputStream* jsonConfigStream)
    : Embedder(embedder)
    , Encoder(protobufModelStream, LoadEncoderDescription(jsonConfigStream))
{
    Encoder.EstablishSessionIfNotYet();
}

float TLstmBasedBinaryClassifier::PredictProbability(const TString& request) const {
    const NVins::TEncoderInput input = GetModelFeed(request);

    const TVector<TVector<float>> predictions = Encoder.Encode(input);
    Y_ENSURE(predictions.size() == 1);
    Y_ENSURE(predictions[0].size() == 1);

    return predictions[0][0];
}

NVins::TEncoderInput TLstmBasedBinaryClassifier::GetModelFeed(const TString& request) const {
    NVins::TEncoderInput input;

    const TVector<TString> tokens = StringSplitter(request).Split(' ');

    input.Lengths.push_back(tokens.ysize());
    input.DenseSeq.emplace_back(Embedder.EmbedSequence(tokens));

    return input;
}

} // namespace NAlice
