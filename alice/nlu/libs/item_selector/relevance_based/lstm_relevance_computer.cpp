#include "lstm_relevance_computer.h"

#include <alice/nlu/libs/tf_nn_model/tensor_helpers.h>
#include <alice/nlu/libs/request_normalizer/request_normalizer.h>

#include <library/cpp/json/json_reader.h>

#include <util/string/split.h>


namespace NAlice {
namespace NItemSelector {
namespace {

constexpr TStringBuf CLF_TOKEN = "[CLF]";
constexpr TStringBuf SEP_TOKEN = "[SEP]";

TVector<TString> Tokenize(const TString& text) {
    // TODO(volobuev): предусмотреть более хитрые варианты токенизации
    const TString normalized = NNlu::TRequestNormalizer::Normalize(ELanguage::LANG_RUS, text);
    return StringSplitter(normalized).Split(' ').ToList<TString>();
}

TVector<TString> GetModelInputTokens(const TString& request, const TVector<TString>& synonyms, const size_t maxTokens) {
    TVector<TString> inputTokens = Tokenize(request);
    inputTokens.emplace_back(CLF_TOKEN);
    for (const TString& synonym : synonyms) {
        const TVector<TString> synomymTokens = Tokenize(synonym);
        inputTokens.insert(inputTokens.end(), synomymTokens.begin(), synomymTokens.end());
        inputTokens.emplace_back(SEP_TOKEN);
    }
    inputTokens.crop(maxTokens);
    return inputTokens;
}

} // anonymous namespace

THashMap<TString, TVector<float>> ReadSpecialEmbeddings(IInputStream* input) {
    const NJson::TJsonValue trainableEmbeddingsJson = NJson::ReadJsonTree(input, true);
    THashMap<TString, TVector<float>> specialTokens;

    for (const auto& [token, embeddingArray] : trainableEmbeddingsJson.GetMap()) {
        TVector<float> embedding;
        for (const auto& value : embeddingArray.GetArray()) {
            embedding.push_back(value.GetDouble());
        }
        specialTokens[token] = embedding;
    }

    return specialTokens;
}

TNnComputer::TNnComputer(IInputStream* protobufModel, IInputStream* modelDescription) : NVins::TTfNnModel(protobufModel) {
    EstablishSessionIfNotYet();

    const NJson::TJsonValue descriptionJson = NJson::ReadJsonTree(modelDescription, true);

    SetInputNodes({descriptionJson["inputs"]["lengths"].GetString(), descriptionJson["inputs"]["token_ids"].GetString()});
    SetOutputNodes({descriptionJson["outputs"]["class_probs"].GetString()});
}

float TNnComputer::Predict(const TVector<int>& lengths, const TVector<TVector<TVector<float>>>& embeddings) const {
    Y_ENSURE(IsSessionEstablished());
    const auto tensor = NVins::Convert3DimTableToTensor<float>(embeddings);
    const auto lengthTensor = NVins::Convert1DimTableToTensor<int>(lengths);
    NNeuralNet::TTensorList inputs{lengthTensor, tensor};
    const auto result = CreateWorker()->Process(inputs);
    return NVins::ConvertTensorTo2DimTable<float>(result[0])[0][0];
}

TLSTMRelevanceComputer::TLSTMRelevanceComputer(THolder<const INnComputer> computer, const NAlice::ITokenEmbedder* embedder,
                                               const THashMap<TString, TVector<float>>& specialEmbeddings, const size_t maxTokens)
    : Computer(std::move(computer))
    , Embedder(embedder)
    , SpecialEmbeddings(specialEmbeddings)
    , MaxTokens(maxTokens)
{
    Y_ENSURE(Computer);
    Y_ENSURE(Embedder);
    Y_ENSURE(SpecialEmbeddings.contains(CLF_TOKEN));
    Y_ENSURE(SpecialEmbeddings.contains(SEP_TOKEN));
    Y_ENSURE(MaxTokens > 0);
}

TVector<TVector<float>> TLSTMRelevanceComputer::Embed(const TString& request, const TVector<TString>& synonyms) const {
    const TVector<TString> inputTokens = GetModelInputTokens(request, synonyms, MaxTokens);
    TVector<TVector<float>> result;
    result.reserve(inputTokens.size());

    for (const TString& token : inputTokens) {
        if (token == CLF_TOKEN || token == SEP_TOKEN) {
            result.push_back(SpecialEmbeddings.at(token));
        } else {
            result.push_back(Embedder->EmbedToken(token));
        }
    }

    return result;
}

float TLSTMRelevanceComputer::ComputeRelevance(const TString& request, const TVector<TString>& synonyms) const {
    const TVector<TVector<TVector<float>>> embeddings {Embed(request, synonyms)};
    const TVector<int> lengths {static_cast<int>(embeddings[0].size())};
    return Computer->Predict(lengths, embeddings);
}

} // namespace NItemSelector
} // namespace NAlice
