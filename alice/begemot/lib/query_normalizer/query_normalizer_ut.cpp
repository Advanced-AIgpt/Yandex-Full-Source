#include <alice/begemot/lib/query_normalizer/query_normalizer.h>
#include <alice/library/json/json.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/string/join.h>
#include <util/string/vector.h>

namespace {

const TVector<NBg::TPrefixNormalizationRule> NORMALIZATION_RULES = {
    {std::make_shared<re2::RE2>("((.* |^)про|((найди|поищи)(| мне)))"), "acc"},
    {std::make_shared<re2::RE2>("(.* |^)о(б|)"), "loc"},
};

struct TNormalizationByPrefixTestCase {
    TString Query;
    size_t IntervalBegin;
    size_t IntervalEnd;
    TString Expected;
};

void TestNormalizationByPrefix(const TNormalizationByPrefixTestCase& test) {
    TVector<TString> tokens{SplitString(test.Query, " ")};
    NNlu::TInterval queryInterval{test.IntervalBegin, test.IntervalEnd};
    NAlice::TTokenIntervalInflector inflector;
    const auto testResult = NBg::NormalizeQueryByPrefix(NORMALIZATION_RULES, tokens, queryInterval, inflector);
    UNIT_ASSERT_VALUES_EQUAL(test.Expected, testResult);
}

} // namespace

Y_UNIT_TEST_SUITE(QueryNormalizer) {
    Y_UNIT_TEST(QueryNormalizerAcc) {
        const TVector<TNormalizationByPrefixTestCase> testCases = {
            {"расскажи про гагарина", 2, 3, "гагарин"},
            {"расскажи про гагарину", 2, 3, "гагарина"},
            {"расскажи про королёва", 2, 3, "королев"},
            {"расскажи про королёва ага", 2, 3, "королев"},
            {"найди королева", 1, 2, "королев"},
            {"поищи королева", 1, 2, "королев"},
            {"а про королева", 2, 3, "королев"},
        };

        for (const auto& testCase : testCases) {
            TestNormalizationByPrefix(testCase);
        }
    }
    Y_UNIT_TEST(QueryNormalizerLoc) {
        const TVector<TNormalizationByPrefixTestCase> testCases = {
            {"расскажи о гагарине", 2, 3, "гагарин"},
            {"расскажи о кошке гагарина", 2, 4, "кошка гагарина"},
            {"расскажи о месте приземления гагарина", 2, 5, "место приземления гагарина"},
            {"расскажи о гагариной", 2, 3, "гагарина"},
        };

        for (const auto& testCase : testCases) {
            TestNormalizationByPrefix(testCase);
        }
    }
    Y_UNIT_TEST(QueryNormalizerNotMatch) {
        const TVector<TNormalizationByPrefixTestCase> testCases = {
            {"расскажи гагарина", 1, 2, "гагарина"},
            {"расскажипро гагарина", 1, 2, "гагарина"},
        };

        for (const auto& testCase : testCases) {
            TestNormalizationByPrefix(testCase);
        }
    }
}
