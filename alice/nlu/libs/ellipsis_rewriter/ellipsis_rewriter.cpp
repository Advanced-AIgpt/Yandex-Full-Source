#include "ellipsis_rewriter.h"
#include <library/cpp/json/json_reader.h>
#include <util/generic/algorithm.h>

namespace NAlice {

namespace {

constexpr TStringBuf MAX_HISTORY_KEY = "max_history_size";
constexpr int DEFAULT_MAX_HISTORY_SIZE = 7;
constexpr TStringBuf NONSENSE_THRESHOLD_KEY = "nonsense_probability_threshold";
constexpr float DEFAULT_NONSENSE_THRESHOLD = 0.5f;

constexpr size_t BEAM_WIDTH = 10;
constexpr TStringBuf SENSE_TAG = "sense";
constexpr TStringBuf SEPARATOR_TOKEN = "[SEP]";

void FixEmptyRewritings(TVector<TRewriterToken>& tokens) {
    const bool isRewritingEmpty = AllOf(tokens, [](const auto& token) { return !token.IsRewrittenTextPart; });
    if (isRewritingEmpty) {
        for (auto& token : tokens) {
            token.IsRewrittenTextPart = token.IsMainRequestPart;
        }
    }
}

float GetTokenNonsenseProbability(const NVins::TPrediction& taggerPrediction, const size_t tokenIndex) {
    if (taggerPrediction.Tags[tokenIndex] == SENSE_TAG) {
        return 1. - taggerPrediction.Probabilities[tokenIndex];
    } else {
        return taggerPrediction.Probabilities[tokenIndex];
    }
}

} // anonymous namespace

TEllipsisRewriterConfig::TEllipsisRewriterConfig(IInputStream* jsonInputStream) {
    Y_ASSERT(jsonInputStream);

    NJson::TJsonValue jsonConfig;
    const bool readCorrectly = NJson::ReadJsonTree(jsonInputStream, &jsonConfig);
    Y_ENSURE(readCorrectly);

    MaxHistorySize = jsonConfig[MAX_HISTORY_KEY].GetIntegerSafe(DEFAULT_MAX_HISTORY_SIZE);
    NonsenseProbabilityThreshold = jsonConfig[NONSENSE_THRESHOLD_KEY].GetDoubleSafe(DEFAULT_NONSENSE_THRESHOLD);
}

TEllipsisRewriter::TEllipsisRewriter(const TVector<float>& separatorEmbedding,
                                     const TTokenEmbedder& tokenEmbedder,
                                     const TEllipsisRewriterConfig& config,
                                     IInputStream* protobufModel,
                                     IInputStream* modelDescription)
    : Tagger(protobufModel, modelDescription)
    , SeparatorEmbedding(separatorEmbedding)
    , TokenEmbedder(tokenEmbedder)
    , Config(config)
{
    Tagger.EstablishSession();
}

TVector<TRewriterToken> TEllipsisRewriter::Predict(const TVector<NVins::TSample>& dialogHistory) const {
    NVins::TSampleFeatures sampleFeatures = PrepareSampleFeatures(dialogHistory);
    const size_t tokenCount = sampleFeatures.Sample.Tokens.size();

    Y_ENSURE(tokenCount >= dialogHistory.back().Tokens.size());
    const size_t mainRequestBeginIndex = tokenCount - dialogHistory.back().Tokens.size();

    const auto prediction = Tagger.PredictTop(sampleFeatures, /*topSize*/ 1, /*beamWidth*/ BEAM_WIDTH)[0];
    Y_ENSURE(prediction.Tags.size() == tokenCount);

    return ConvertTaggerPrediction(prediction, sampleFeatures.Sample.Tokens, mainRequestBeginIndex);
}

NVins::TSampleFeatures TEllipsisRewriter::PrepareSampleFeatures(const TVector<NVins::TSample>& dialogHistory) const {
    NVins::TSampleFeatures sampleFeatures;
    auto& sampleTokens = sampleFeatures.Sample.Tokens;
    auto& embeddingsFeature = sampleFeatures.DenseSeq["alice_requests_emb"];

    const size_t beginPhraseIndex = Max(0, dialogHistory.ysize() - Config.MaxHistorySize);
    for (size_t phraseIndex = beginPhraseIndex; phraseIndex < dialogHistory.size(); ++phraseIndex) {
        const auto& phraseTokens = dialogHistory[phraseIndex].Tokens;

        sampleTokens.insert(sampleTokens.end(), phraseTokens.begin(), phraseTokens.end());

        const auto phraseEmbeddings = TokenEmbedder.EmbedSequence(phraseTokens);
        embeddingsFeature.insert(embeddingsFeature.end(), phraseEmbeddings.begin(), phraseEmbeddings.end());

        if (phraseIndex + 1 < dialogHistory.size()) {
            sampleTokens.emplace_back(SEPARATOR_TOKEN);
            embeddingsFeature.push_back(SeparatorEmbedding);
        }
    }

    return sampleFeatures;
}

TVector<TRewriterToken> TEllipsisRewriter::ConvertTaggerPrediction(const NVins::TPrediction& taggerPrediction,
                                                                   const TVector<TString>& inputTokens,
                                                                   const size_t mainRequestBeginIndex) const {
    TVector<TRewriterToken> resultTokens;
    for (size_t tokenIndex = 0; tokenIndex < inputTokens.size(); ++tokenIndex) {
        const float nonsenseProbability = GetTokenNonsenseProbability(taggerPrediction, tokenIndex);

        TRewriterToken token;
        token.Text = inputTokens[tokenIndex];
        token.NonsenseProbability = nonsenseProbability;
        token.IsMainRequestPart = tokenIndex >= mainRequestBeginIndex;
        token.IsRewrittenTextPart = nonsenseProbability < Config.NonsenseProbabilityThreshold;

        resultTokens.emplace_back(std::move(token));
    }

    FixEmptyRewritings(resultTokens);

    return resultTokens;
}

} // namespace NAlice
