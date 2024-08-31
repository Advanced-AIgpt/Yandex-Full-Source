#include "easy_tagger.h"

#include <alice/nlu/libs/request_normalizer/request_normalizer.h>
#include <alice/nlu/libs/request_normalizer/request_tokenizer.h>

namespace NAlice {
namespace NItemSelector{
namespace {

NVins::TSampleFeatures GetSampleFeatures(const TTokenEmbedder& embedder, const TVector<TString>& tokens) {
    NVins::TSampleFeatures sampleFeatures;
    sampleFeatures.Sample.Tokens = tokens;
    sampleFeatures.Sample.Tags = TVector<TString>(sampleFeatures.Sample.Tokens.size(), "O");
    sampleFeatures.DenseSeq["alice_requests_emb"] = embedder.EmbedSequence(sampleFeatures.Sample.Tokens);
    return sampleFeatures;
}

} // anonymous namespace

TTaggerResult TEasyTagger::Tag(const TString& text, ELanguage language /* = LANG_RUS */) const {
    const auto normalized = NNlu::TRequestNormalizer::Normalize(language, text);
    const TVector<TStringBuf> tokens = NNlu::TRequestTokenizer::Tokenize(normalized);
    const auto sampleFeatures = GetSampleFeatures(TokenEmbedder, {tokens.begin(), tokens.end()});
    const auto nativePredictions = RnnTagger.PredictTop(sampleFeatures, TopSize, BeamWidth);

    TTaggerResult result;
    for (const auto& prediction : nativePredictions) {
        TTaggingAlternative alternative;
        alternative.Probability = prediction.FullProbability;
        for (size_t i = 0; i < tokens.size(); ++i) {
            alternative.Tokens.push_back({TString{tokens[i]}, prediction.Tags[i]});
        }
        result.push_back(alternative);
    }

    return result;
}

} // NItemSelector
} // NAlice
