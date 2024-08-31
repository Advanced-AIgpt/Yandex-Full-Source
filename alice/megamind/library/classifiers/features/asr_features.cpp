#include "asr_features.h"

#include <alice/megamind/library/scenarios/defs/names.h>

#include <alice/library/experiments/experiments.h>

#include <kernel/alice/asr_factors_info/factors_gen.h>
#include <kernel/factor_storage/factor_storage.h>

#include <library/cpp/statistics/statistics.h>
#include <library/cpp/string_utils/levenshtein_diff/levenshtein_diff.h>

#include <util/charset/utf8.h>
#include <util/string/cast.h>

#include <algorithm>
#include <cmath>

using namespace NAliceAsrFactors;

namespace NAlice {

namespace {

size_t GetHypothesisLength(const TAsrResult& hypothesis) {
    size_t hypothesisLength = 0;
    for (const auto& word : hypothesis.GetWords()) {
        hypothesisLength += GetNumberOfUTF8Chars(word.GetValue());
    }
    return hypothesisLength;
}

double GetNormalizedLevenshteinDistance(const TAsrResult& first, const TAsrResult& second) {
    double distance = 0, maxDistance = 0;
    for (uint i = 0; i < std::min(first.WordsSize(), second.WordsSize()); i++) {
        distance += NLevenshtein::Distance(
            ToLowerUTF8(first.GetWords(i).GetValue()),
            ToLowerUTF8(second.GetWords(i).GetValue())
        );
        maxDistance += std::max(
            first.GetWords(i).GetValue().length(),
            second.GetWords(i).GetValue().length()
        );
    }
    if (maxDistance > 0) {
        distance /= static_cast<double>(maxDistance);
    }
    return distance;
}

bool IsNewSpeaker(const TVector<const TAsrResult>& speakerSamples, const TAsrResult& utterance, double newSpeakerThreshold) {
    if (!speakerSamples) {
        return true;
    }
    for (const auto& speakerSample : speakerSamples) {
        if (GetNormalizedLevenshteinDistance(speakerSample, utterance) > newSpeakerThreshold) {
            return true;
        }
    }
    return false;
}

size_t GetSpeakersNumber(const google::protobuf::RepeatedPtrField<TAsrResult>& asrResult, double newSpeakerThreshold) {
    TVector<const TAsrResult> speakerSamples;

    for (const auto& hypothesis : asrResult) {
        if (IsNewSpeaker(speakerSamples, hypothesis, newSpeakerThreshold)) {
            speakerSamples.push_back(hypothesis);
        }
    }
    return speakerSamples.size();
}

template<typename T>
T Min(const TVector<T>& values) {
    const auto minIt = std::min_element(values.begin(), values.end());
    if (minIt == values.end()) {
        return 0;
    }
    return *minIt;
}

template<typename T>
T Max(const TVector<T>& values) {
    const auto maxIt = std::max_element(values.begin(), values.end());
    if (maxIt == values.end()) {
        return 0;
    }
    return *maxIt;
}

double Median(TVector<double> values) {
    if (values.empty()) {
        return 0;
    }
    size_t pos = values.size() / 2;
    NthElement(values.begin(), values.begin() + pos, values.end());
    return values[pos];
}

enum EAcousticModelType {
    AMT_UNDEFINED,
    AMT_SEQ2SEQ,
    AMT_GRAPHEME,
};

struct TAcousticScores {
    TVector<double> Scores;
    EAcousticModelType ModelType;
};

TAcousticScores ParseAcousticScoresFromPartialRequest(const NJson::TJsonValue& hypothesesInfo) {
    TVector<double> acousticScores;
    auto modelType = AMT_UNDEFINED;
    for (const auto& hypObj : hypothesesInfo.GetArray()) {
        acousticScores.push_back(hypObj["acoustic_score"].GetDouble());
        const auto& modelName = hypObj["parent_model"].GetString();
        if (modelName == "seq2seq") {
            modelType = AMT_SEQ2SEQ;
        } else if (modelName == "grapheme") {
            modelType = AMT_GRAPHEME;
        }
    }
    return {
        .Scores = acousticScores,
        .ModelType = modelType,
    };

}

TAcousticScores ParseAcousticScoresFromFinalRequest(const NJson::TJsonValue& hypoFeats) {
    TVector<double> acousticScores;
    for (const auto& hypObj : hypoFeats.GetArray()) {
        const auto& seq2seq = hypObj["seq2seq_info"];
        acousticScores.push_back(seq2seq["acoustic_score"].GetDouble());
    }
    return {
        .Scores = acousticScores,
        .ModelType = AMT_SEQ2SEQ,
    };
}

} // namespace

void FillAsrDebugFactors(const TEvent& event, const TFactorView& view) {
    if (event.GetAsrCoreDebug().empty()) {
        return;
    }

    NJson::TJsonValue asrCoreDebug;
    NJson::ReadJsonFastTree(event.GetAsrCoreDebug(), &asrCoreDebug);

    TAcousticScores acousticScores;
    if (const auto& finalHypothesInfo = asrCoreDebug["HypoFeats"]; finalHypothesInfo.IsDefined()) {
        acousticScores = ParseAcousticScoresFromFinalRequest(finalHypothesInfo);
    } else if (const auto& partialHypothesesInfo = asrCoreDebug["HypothesesInfo"]; partialHypothesesInfo.IsDefined()) {
        acousticScores = ParseAcousticScoresFromPartialRequest(partialHypothesesInfo);
    }

    if (!acousticScores.Scores) {
        return;
    }

    const auto acousticMeanStd = NStatistics::MeanAndStandardDeviation(acousticScores.Scores.begin(), acousticScores.Scores.end());
    view[FI_MAX_ASR_ACOUSTIC_SCORE] = Max(acousticScores.Scores);
    view[FI_MIN_ASR_ACOUSTIC_SCORE] = Min(acousticScores.Scores);
    view[FI_AVG_ASR_ACOUSTIC_SCORE] = acousticMeanStd.Mean;
    view[FI_STDDEV_ASR_ACOUSTIC_SCORE] = acousticMeanStd.Std;
    view[FI_MEDIAN_ASR_ACOUSTIC_SCORE] = Median(acousticScores.Scores);
    view[FI_WINNER_ASR_ACOUSTIC_SCORE] = acousticScores.Scores.front();

    view[FI_ACOUSTIC_MODEL_TYPE] = acousticScores.ModelType;

    view[FI_DURATION_PROCESSED_AUDIO] = event.GetAsrDurationProcessedAudio();
}

void FillAsrFactors(const TEvent& event, const TRequest& request, const IContext::TExpFlags& expFlags, TFactorStorage& storage) {
    const TFactorView view = storage.CreateViewFor(NFactorSlices::EFactorSlice::ALICE_ASR_FACTORS);

    view[FI_ASR_HYPOTHESIS_COUNT] = event.AsrResultSize();

    TVector<double> hypothesisWordCounts(event.AsrResultSize());
    TVector<double> hypothesisLengths(event.AsrResultSize());
    for (const auto& hypothesis : event.GetAsrResult()) {
        const double hypothesisLength = GetHypothesisLength(hypothesis);
        hypothesisWordCounts.push_back(hypothesis.WordsSize());
        hypothesisLengths.push_back(hypothesisLength);
    }

    const auto wordsCountMeanStd = NStatistics::MeanAndStandardDeviation(hypothesisWordCounts.begin(), hypothesisWordCounts.end());
    view[FI_MAX_ASR_HYPOTHESIS_WORDS_COUNT] = Max(hypothesisWordCounts);
    view[FI_MIN_ASR_HYPOTHESIS_WORDS_COUNT] = Min(hypothesisWordCounts);
    view[FI_AVG_ASR_HYPOTHESIS_WORDS_COUNT] = wordsCountMeanStd.Mean;
    view[FI_STDDEV_ASR_HYPOTHESIS_WORDS_COUNT] = wordsCountMeanStd.Std;

    const auto hypothesisLengthMeanStd = NStatistics::MeanAndStandardDeviation(hypothesisLengths.begin(), hypothesisLengths.end());
    view[FI_MAX_ASR_HYPOTHESIS_UTTERANCE_LENGTH] = Max(hypothesisLengths);
    view[FI_MIN_ASR_HYPOTHESIS_UTTERANCE_LENGTH] = Min(hypothesisLengths);
    view[FI_AVG_ASR_HYPOTHESIS_UTTERANCE_LENGTH] = hypothesisLengthMeanStd.Mean;
    view[FI_STDDEV_ASR_HYPOTHESIS_UTTERANCE_LENGTH] = hypothesisLengthMeanStd.Std;

    if (event.AsrResultSize() > 0) {
        const auto& winnerHypothesis = event.GetAsrResult(0);

        view[FI_WINNER_ASR_HYPOTHESIS_WORDS_COUNT] = winnerHypothesis.WordsSize();
        view[FI_WINNER_ASR_HYPOTHESIS_UTTERANCE_LENGTH] = GetHypothesisLength(winnerHypothesis);
    }

    view[FI_ASR_SPEAKERS_NUMBER] = GetSpeakersNumber(
        event.GetAsrResult(),
        FromStringWithDefault<double>(
            GetExperimentValueWithPrefix(expFlags, "mm_new_speaker_threshold=").GetOrElse("0.5"),
            0.5
        )
    );

    if (event.AsrResultSize() > 1) {
        TVector<double> distancesFromWinner;
        const auto& target = event.GetAsrResult(0);
        for (size_t i = 1; i < event.AsrResultSize(); i++) {
            const auto& phrase = event.GetAsrResult(i);
            distancesFromWinner.push_back(
                GetNormalizedLevenshteinDistance(target, phrase)
            );
        }

        const auto distancesFromWinnerMeanStd = NStatistics::MeanAndStandardDeviation(distancesFromWinner.begin(), distancesFromWinner.end());
        view[FI_ASR_HYPOTHESIS_DISTANCE_MEAN] = distancesFromWinnerMeanStd.Mean;
        view[FI_ASR_HYPOTHESIS_DISTANCE_STDDEV] = distancesFromWinnerMeanStd.Std;
    }

    for (const auto& classificationResult : event.GetBiometryClassification().GetScores()) {
        if (classificationResult.GetClassName() == "child") {
            view[FI_CHILD_CONFIDENCE] = classificationResult.GetConfidence();
            break;
        }
    }
    view[FI_IS_CHILD] = request.GetIsClassifiedAsChildRequest();

    FillAsrDebugFactors(event, view);
}

} // namespace NAlice
