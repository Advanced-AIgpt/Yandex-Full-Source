#include <alice/nlu/libs/anaphora_resolver/matchers/lstm/lstm.h>
#include <alice/nlu/libs/anaphora_resolver/measure_quality/lib/dsv_test_reader.h>
#include <alice/nlu/libs/anaphora_resolver/measure_quality/lib/measure_quality.h>
#include <alice/nlu/libs/anaphora_resolver/measure_quality/lib/test_sample.h>
#include <library/cpp/resource/resource.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/folder/path.h>
#include <util/generic/algorithm.h>
#include <util/stream/str.h>
#include <util/string/join.h>

namespace {
    const NAlice::TAnaphoraMatcherTestSample INVALID_EXAMPLE = NAlice::TAnaphoraMatcherTestSample{
        {NVins::TSample{"", {"invalid", "entity", "anaphora"}, {}}},
        {NAlice::TMentionInDialogue(/*sample*/0, /*start*/0, /*end*/1)},
        NAlice::TMentionInDialogue(/*sample*/0, /*start*/1, /*end*/2),
        {true},
        Nothing()
    };

    const NAlice::TAnaphoraMatcherTestSample TRUE_POSITIVE_EXAMPLE = NAlice::TAnaphoraMatcherTestSample{
        {NVins::TSample{"", {"tp", "correct_match", "anaphora"}, {}}},
        {NAlice::TMentionInDialogue(/*sample*/0, /*start*/1, /*end*/2)},
        NAlice::TMentionInDialogue(/*sample*/0, /*start*/2, /*end*/3),
        {true},
        NAlice::TMentionInDialogue(/*sample*/0, /*start*/1, /*end*/2)
    };

    const NAlice::TAnaphoraMatcherTestSample FALSE_POSITIVE_EXAMPLE = NAlice::TAnaphoraMatcherTestSample{
        {NVins::TSample{"", {"fp", "correct_match", "incorrect_match", "anaphora"}, {}}},
        {NAlice::TMentionInDialogue(/*sample*/0, /*start*/1, /*end*/2), NAlice::TMentionInDialogue(/*sample*/0, /*start*/2, /*end*/3)},
        NAlice::TMentionInDialogue(/*sample*/0, /*start*/3, /*end*/4),
        {true, false},
        NAlice::TMentionInDialogue(/*sample*/0, /*start*/2, /*end*/3)
    };

    const NAlice::TAnaphoraMatcherTestSample FALSE_NEGATIVE_EXAMPLE = NAlice::TAnaphoraMatcherTestSample{
        {NVins::TSample{"", {"fn", "correct_match", "anaphora"}, {}}},
        {NAlice::TMentionInDialogue(/*sample*/0, /*start*/1, /*end*/2)},
        NAlice::TMentionInDialogue(/*sample*/0, /*start*/2, /*end*/3),
        {true},
        Nothing()
    };

    const NAlice::TAnaphoraMatcherTestSample TRUE_NEGATIVE_EXAMPLE = NAlice::TAnaphoraMatcherTestSample{
        {NVins::TSample{"", {"tn", "incorrect_match", "anaphora"}, {}}},
        {NAlice::TMentionInDialogue(/*sample*/0, /*start*/1, /*end*/2)},
        NAlice::TMentionInDialogue(/*sample*/0, /*start*/2, /*end*/3),
        {false},
        Nothing()
    };

    class TMockMatcherModel {
    public:
        TMaybe<NAlice::TAnaphoraMatch> Predict(const TVector<NVins::TSample>& dialogHistory,
                                               const TVector<NAlice::TMentionInDialogue>&/*entityPositions*/,
                                               const NAlice::TMentionInDialogue&/*pronounPosition*/) const {
            Y_ENSURE(dialogHistory.size() > 0);
            Y_ENSURE(dialogHistory[0].Tokens.size() > 0);
            const auto& exampleType = dialogHistory[0].Tokens[0];
            UNIT_ASSERT(exampleType != "invalid");

            NAlice::TAnaphoraMatcherTestSample receivedExample;
            if (exampleType == "tp") {
                receivedExample = TRUE_POSITIVE_EXAMPLE;
            } else if (exampleType == "fp") {
                receivedExample = FALSE_POSITIVE_EXAMPLE;
            } else if (exampleType == "fn") {
                receivedExample = FALSE_NEGATIVE_EXAMPLE;
            } else if (exampleType == "tn") {
                receivedExample = TRUE_NEGATIVE_EXAMPLE;
            } else {
                UNIT_ASSERT(false); // unknown example
            }

            return receivedExample.AnswerEntity.Empty() ? Nothing() :
                   TMaybe<NAlice::TAnaphoraMatch>(NAlice::TAnaphoraMatch{/*Anaphora*/receivedExample.PronounPosition,
                                                                         /*Antecedent*/receivedExample.AnswerEntity.GetRef(),
                                                                         /*Score*/1.0,
                                                                         /*AnaphoraGrammemes*/""});
        }
    };

    class TMockTestReader {
    public:
        TMockTestReader(const TVector<NAlice::TAnaphoraMatcherTestSample>& tests)
            : Tests(tests)
        {
            Reverse(Tests.begin(), Tests.end());
        }

        bool HasLine() const {
            return Tests.size() > 0;
        }

        bool ParseLine(NAlice::TAnaphoraMatcherTestSample* testSample, IOutputStream* errorLog) {
            UNIT_ASSERT(HasLine());
            const NAlice::TAnaphoraMatcherTestSample readTest = Tests.back();
            const TString& exampleType = readTest.DialogHistory[0].Tokens[0];
            Tests.pop_back();

            if (exampleType == "invalid") {
                (*errorLog << "ERROR").Flush();
                return false;
            }

            *testSample = readTest;
            return true;
        }

    private:
        TVector<NAlice::TAnaphoraMatcherTestSample> Tests;
    };

    NAlice::TMatchStatistics CollectStatistics(const size_t numErrorsExpected,
                                               const TVector<NAlice::TAnaphoraMatcherTestSample>& examples) {
        TMockTestReader testData(examples);

        TString errorLogData;
        TStringOutput errorLog(errorLogData);

        TString logData;
        TStringOutput log(logData);

        const NAlice::TMatchStatistics statistics = NAlice::CollectMatchStatistics(TMockMatcherModel(), &testData, &errorLog, &log);
        UNIT_ASSERT_EQUAL(/*number of lines*/static_cast<size_t>(Count(errorLogData, '\n')), numErrorsExpected);

        return statistics;
    }
} // namespace anonymous

Y_UNIT_TEST_SUITE(MeasureQualitySuite) {

Y_UNIT_TEST(InvalidTests) {
    const auto statistics = CollectStatistics(/*numErrors*/5,
                                              TVector<NAlice::TAnaphoraMatcherTestSample>(5, INVALID_EXAMPLE));
    UNIT_ASSERT_EQUAL(statistics.TruePositiveCount, 0);
    UNIT_ASSERT_EQUAL(statistics.FalsePositiveCount, 0);
    UNIT_ASSERT_EQUAL(statistics.FalseNegativeCount, 0);
    UNIT_ASSERT_EQUAL(statistics.TrueNegativeCount, 0);
}

Y_UNIT_TEST(StatisticsTest) {
    TVector<NAlice::TAnaphoraMatcherTestSample> tests;
    tests.insert(tests.end(), 8, TRUE_POSITIVE_EXAMPLE);
    tests.insert(tests.end(), 1, FALSE_POSITIVE_EXAMPLE);
    tests.insert(tests.end(), 13, FALSE_NEGATIVE_EXAMPLE);
    tests.insert(tests.end(), 32, TRUE_NEGATIVE_EXAMPLE);
    tests.insert(tests.end(), 3, INVALID_EXAMPLE);

    const auto statistics = CollectStatistics(/*numErrors*/3, tests);

    UNIT_ASSERT_EQUAL(statistics.TruePositiveCount, 8);
    UNIT_ASSERT_EQUAL(statistics.FalsePositiveCount, 1);
    UNIT_ASSERT_EQUAL(statistics.FalseNegativeCount, 13);
    UNIT_ASSERT_EQUAL(statistics.TrueNegativeCount, 32);
}

Y_UNIT_TEST(NoTests) {
    const auto statistics = CollectStatistics(/*numErrors*/0, {});
    UNIT_ASSERT_EQUAL(statistics.TruePositiveCount, 0);
    UNIT_ASSERT_EQUAL(statistics.FalsePositiveCount, 0);
    UNIT_ASSERT_EQUAL(statistics.FalseNegativeCount, 0);
    UNIT_ASSERT_EQUAL(statistics.TrueNegativeCount, 0);
}

}
