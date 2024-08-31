#pragma once

#include "test_sample.h"
#include <alice/nlu/libs/anaphora_resolver/common/match.h>
#include <alice/nlu/libs/anaphora_resolver/common/mention.h>
#include <alice/nlu/libs/embedder/embedder.h>
#include <alice/nlu/libs/sample_features/sample_features.h>
#include <util/folder/path.h>
#include <util/stream/file.h>
#include <util/stream/str.h>
#include <util/system/hp_timer.h>

namespace NAlice {
    enum EAnaphoraMatcherModel {
        LSTM
    };

    struct TMeasureAnaphoraMatcherQualityOptions {
        TFsPath ModelDirPath;
        TFsPath EmbedderDirPath;
        TFsPath TestDataPath;
        TFsPath ResultsPath;
        bool Verbose = false;
        EAnaphoraMatcherModel MatcherModel = EAnaphoraMatcherModel::LSTM;
    };

    void MeasureAnaphoraMatcherQuality(const TMeasureAnaphoraMatcherQualityOptions& options, IOutputStream* log);

    struct TMatchStatistics {
        size_t TruePositiveCount = 0;
        size_t FalsePositiveCount = 0;
        size_t FalseNegativeCount = 0;
        size_t TrueNegativeCount = 0;
    };

    template <class TMatcher, class TTestReader>
    TMatchStatistics CollectMatchStatistics(const TMatcher& matcher,
                                            TTestReader* testReader,
                                            IOutputStream* resultsOutput,
                                            IOutputStream* log) {
        Y_ASSERT(resultsOutput);
        Y_ASSERT(log);

        THPTimer timer;

        TMatchStatistics statistics;
        size_t lineIdx = 0;
        while (testReader->HasLine()) {
            if (timer.Passed() > 1.0) {
                *log << "\r" << lineIdx << " samples processed";
                log->Flush();
                timer.Reset();
            }

            ++lineIdx;

            TAnaphoraMatcherTestSample testSample;
            TString errorLogString;
            TStringOutput errorLog(errorLogString);

            const bool parsed = testReader->ParseLine(&testSample, &errorLog);

            if (!parsed) {
                *resultsOutput << "PARSE ERROR: " << errorLogString << " (line " << lineIdx << ")" << Endl;
                continue;
            }

            TVector<NAlice::TMentionInDialogue> correctEntities;
            for (size_t entityIdx = 0; entityIdx < testSample.EntityPositions.size(); ++entityIdx) {
                if (testSample.ValidEntitiesMarkup[entityIdx]) {
                    correctEntities.push_back(testSample.EntityPositions[entityIdx]);
                }
            }

            const TMaybe<TAnaphoraMatch> match = matcher.Predict(testSample.DialogHistory,
                                                                 testSample.EntityPositions,
                                                                 testSample.PronounPosition);
            if (!match.Empty() && match->Anaphora != testSample.PronounPosition) {
                *resultsOutput << "MATCH ERROR: Pronoun is not as stated in request." << Endl;
            }

            if (match.Empty()) {
                if (correctEntities.empty()) {
                    ++statistics.TrueNegativeCount;
                } else {
                    ++statistics.FalseNegativeCount;
                }
            } else {
                bool isTrue= false;
                for (const auto& entity : correctEntities) {
                    if (entity == match->Antecedent) {
                        isTrue = true;
                        break;
                    }
                }
                if (isTrue) {
                    ++statistics.TruePositiveCount;
                } else {
                    ++statistics.FalsePositiveCount;
                }
            }
        }
        *log << "\r" << lineIdx << " samples processed" << Endl;

        return statistics;
    }

    void OutputMatchStatistics(const TMatchStatistics& statistics, IOutputStream* resultsOutput);

    template <class TMatcher, class TTestReader>
    void MeasureQuality(const TMatcher& matcher,
                        TTestReader* testReader,
                        IOutputStream* resultsOutput,
                        IOutputStream* log) {
        Y_ASSERT(resultsOutput);
        Y_ASSERT(log);

        *log << "Processing test samples..." << Endl;
        const auto statistics = CollectMatchStatistics(matcher, testReader, resultsOutput, log);
        OutputMatchStatistics(statistics, resultsOutput);
    }

} // namespace NAlice
