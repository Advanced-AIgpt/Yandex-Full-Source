#include "dsv_test_reader.h"
#include "measure_quality.h"
#include <alice/nlu/libs/anaphora_resolver/matchers/lstm/lstm.h>
#include <util/folder/path.h>
#include <util/generic/algorithm.h>
#include <util/memory/blob.h>

namespace NAlice {
    namespace {
        NAlice::TTokenEmbedder LoadEmbedder(const TFsPath& dirPath) {
            return NAlice::TTokenEmbedder(TBlob::PrechargedFromFile(dirPath / "embeddings"),
                                          TBlob::PrechargedFromFile(dirPath / "embeddings_dictionary.trie"));
        }

        double CalcPrecision(const size_t truePositive, const size_t falsePositive) {
             return (truePositive + falsePositive) ? double(truePositive) / (truePositive + falsePositive) : 0.0;
        }

        double CalcRecall(const size_t truePositive, const size_t falseNegative) {
            return (truePositive + falseNegative) ? double(truePositive) / (truePositive + falseNegative) : 0.0;
        }

        double CalcF1(const double precision, const double recall) {
            return (precision + recall > 0.0) ? (2.0 * precision * recall) / (precision + recall) : 0.0;
        }
    } // namespace anonymous

    void MeasureAnaphoraMatcherQuality(const TMeasureAnaphoraMatcherQualityOptions& options, IOutputStream* log) {
        Y_ASSERT(log);

        TFileOutput resultsFile(options.ResultsPath);
        TDsvAnaphoraTestReader testReader(MakeHolder<TFileInput>(options.TestDataPath));

        if (options.MatcherModel == EAnaphoraMatcherModel::LSTM) {
            THPTimer timer;

            const auto embedder = LoadEmbedder(options.EmbedderDirPath);
            *log << "Embedder loaded in " << timer.PassedReset() << " sec." << Endl;

            TLstmAnaphoraMatcherModel matcher(options.ModelDirPath, embedder);
            matcher.EstablishSessionIfNotYet();
            *log << "Matcher loaded in " << timer.PassedReset() << " sec." << Endl;

            return MeasureQuality(matcher, &testReader, &resultsFile, log);
        }
        Y_VERIFY(false, "Unknown matcher model type.");
    }

    void OutputMatchStatistics(const TMatchStatistics& statistics, IOutputStream* resultsOutput) {
        Y_ASSERT(resultsOutput);

        *resultsOutput << "---" << Endl;
        *resultsOutput << "true positive: " << statistics.TruePositiveCount << Endl;
        *resultsOutput << "false positive: " << statistics.FalsePositiveCount << Endl;
        *resultsOutput << "false negative: " << statistics.FalseNegativeCount << Endl;
        *resultsOutput << "true negative: " << statistics.TrueNegativeCount << Endl;

        const double precision = CalcPrecision(statistics.TruePositiveCount, statistics.FalsePositiveCount);
        *resultsOutput << "precision = " << precision << Endl;

        const double recall = CalcRecall(statistics.TruePositiveCount, statistics.FalseNegativeCount);
        *resultsOutput << "recall = " << recall << Endl;

        const double f1 = CalcF1(precision, recall);
        *resultsOutput << "f1 = " << f1 << Endl;
    }
} // namespace NAlice
