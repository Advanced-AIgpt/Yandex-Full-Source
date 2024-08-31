#include "default.h"

#include <alice/nlu/libs/item_selector/common/common.h>

#include <alice/library/parsed_user_phrase/parsed_sequence.h>
#include <alice/library/parsed_user_phrase/stopwords.h>
#include <alice/library/parsed_user_phrase/utils.h>

#include <library/cpp/dot_product/dot_product.h>
#include <library/cpp/resource/resource.h>

#include <util/generic/is_in.h>
#include <util/generic/utility.h>
#include <util/stream/str.h>
#include <util/string/join.h>

namespace NAlice {
namespace NItemSelector {
namespace {

constexpr double INTERSECTION_THRESHOLD = 0.52;
constexpr double DSSM_THRESHOLD = 0.9;
constexpr double NONSENSE_THRESHOLD = 0.9;
constexpr double EPSILON = 1e-6;
constexpr double NEGATIVE_MATCH = -1;
constexpr double POSITIVE_EXACT = 1;

TString MakeRequestWithoutNonsense(const TNonsenseTagging& nonsenseTagging, const double threshold) {
    TVector<TStringBuf> meaningfulTokens;
    for (const auto& token : nonsenseTagging) {
        if (token.NonsenseProbability < threshold) {
            meaningfulTokens.push_back(token.Text);
        }
    }
    return JoinSeq(" ", meaningfulTokens);
}

float EasyDotProduct(const TVector<float>& first, const TVector<float>& second) {
    Y_ENSURE(first.size() == second.size());
    return DotProduct(first.data(), second.data(), first.size());
}

float ComputeWordWeightPatch(const TUtf16String& word) {
    return 1. / (NParsedUserPhrase::ComputeWordWeight(word) + EPSILON);
}

NParsedUserPhrase::TStopWordsHolder MakeWeightPatch(const TVector<TUtf16String>& words) {
    NParsedUserPhrase::TStopWordsHolder patch;
    for (const auto& word : words) {
        patch.Add(word, ComputeWordWeightPatch(word));
    }
    return patch;
}

double ComputeIntersectionScoreWithPatch(const NParsedUserPhrase::TParsedSequence& request,
                                         const NParsedUserPhrase::TParsedSequence& phrase,
                                         const ELanguage language) {
    if (language == ELanguage::LANG_RUS) {
        static const NParsedUserPhrase::TStopWordsHolder patch = MakeWeightPatch({u"не"});
        return NParsedUserPhrase::ComputeIntersectionScore(request, phrase, patch);
    }
    return NParsedUserPhrase::ComputeIntersectionScore(request, phrase);
}

class TEvaluator {
public:
    TEvaluator(const TSelectionRequest& request, const NAlice::TBoltalkaDssmEmbedder* embedder,
               const ELanguage language, const TMaybe<NParsedUserPhrase::TStopWordsHolder>& idfs)
        : Request(Lowercase(request))
        , Embedder(embedder)
        , Language(language)
        , ParsedRequest(Request.Phrase.Text)
        , IDFs(idfs)
    {
        if (Embedder) {
            RequestEmbedding = Embedder->Embed(Request.Phrase.Text);
            RequestWithoutNonsenseEmbedding = Embedder->Embed(
                MakeRequestWithoutNonsense(request.NonsenseTagging, NONSENSE_THRESHOLD)
            );
        }
    }

    double Evaluate(const TSelectionItem& item) const {
        const TSelectionItem itemLowercased = Lowercase(item);
        const TVector<TString> positiveTexts = GetTexts(item.Synonyms);
        const double positiveScore = EvaluatePhrases(positiveTexts);
        if (IsIn(positiveTexts, Request.Phrase.Text)) {
            return POSITIVE_EXACT;
        }
        const TVector<TString> negativeTexts = GetTexts(itemLowercased.Negatives);
        const double negativeScore = EvaluatePhrases(negativeTexts);
        if (IsIn(negativeTexts, Request.Phrase.Text) || negativeScore > positiveScore) {
            return NEGATIVE_MATCH;
        }
        return positiveScore;
    }

private:
    double EvaluatePhrase(const TString& phrase) const {
        const NParsedUserPhrase::TParsedSequence parsedPhrase(phrase);
        double intersection;
        if (IDFs.Defined()) {
            intersection = NParsedUserPhrase::ComputeIntersectionScore(ParsedRequest, parsedPhrase, *IDFs);
        } else {
            intersection = ComputeIntersectionScoreWithPatch(ParsedRequest, parsedPhrase, Language);
        }
        if (intersection > INTERSECTION_THRESHOLD) {
            return intersection;
        }

        if (Embedder) {
            const TVector<float> phraseEmbedding(Embedder->Embed(phrase));
            if (const double v = EasyDotProduct(RequestEmbedding, phraseEmbedding); v > DSSM_THRESHOLD) {
                return v;
            }
            if (const double v = EasyDotProduct(RequestWithoutNonsenseEmbedding, phraseEmbedding); !v > DSSM_THRESHOLD) {
                return v;
            }
        }
        return -1;
    }

    double EvaluatePhrases(const TVector<TString>& phrases) const {
        double score = -1;
        for (const TString& phrase : phrases) {
            score = Max(score, EvaluatePhrase(phrase));
        }
        return score;
    }

private:
    const TSelectionRequest Request;
    const NAlice::TBoltalkaDssmEmbedder* Embedder;
    const ELanguage Language;
    const NParsedUserPhrase::TParsedSequence ParsedRequest;
    const TMaybe<NParsedUserPhrase::TStopWordsHolder>& IDFs;
    TVector<float> RequestEmbedding;
    TVector<float> RequestWithoutNonsenseEmbedding;
};

TVector<double> ComputeScores(const TVector<TSelectionItem>& items, const TEvaluator& evaluator) {
    TVector<double> scores;
    for (const TSelectionItem& item : items) {
        scores.push_back(evaluator.Evaluate(item));
    }
    return scores;
}

} // anonymous namespace

NParsedUserPhrase::TStopWordsHolder LoadIDFs(IInputStream& stream) {
    NParsedUserPhrase::TStopWordsHolder idfs;
    TString line;
    while(stream.ReadLine(line)) {
        TVector<TString> tokens;
        Split(line, "\t", tokens);
        idfs.Add(tokens[0], FromString<float>(tokens[1]), /*addLemma = */ false);
    }
    return idfs;
}

NParsedUserPhrase::TStopWordsHolder LoadIDFs() {
    TStringStream stream(NResource::Find("/idfs.tsv"));
    return LoadIDFs(stream);
}

TVector<TSelectionResult> TDefaultItemSelector::Select(const TSelectionRequest& request, const TVector<TSelectionItem>& items) const {
    const TEvaluator evaluator(request, Embedder, Language, IDFs);

    const TVector<double> scores = ComputeScores(items, evaluator);
    TVector<TSelectionResult> result;

    const size_t maxScoreIndex = std::max_element(scores.begin(), scores.end()) - scores.begin();

    for (size_t i = 0; i < scores.size(); ++i) {
        result.push_back({scores[i], i == maxScoreIndex && scores[i] > Min(INTERSECTION_THRESHOLD, DSSM_THRESHOLD)});
    }
    return result;
}

} // namespace NItemSelector
} // namespace NAlice
