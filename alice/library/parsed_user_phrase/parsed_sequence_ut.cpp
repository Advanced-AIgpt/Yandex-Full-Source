#include "parsed_sequence.h"
#include "stopwords.h"

#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>

using namespace NParsedUserPhrase;


Y_UNIT_TEST_SUITE(MatchTestSuite) {
    Y_UNIT_TEST(ComputeIntersectionScore) {
        NParsedUserPhrase::TStopWordsHolder patch;
        patch.Add(u"не", 10);
        TParsedSequence first("хочу");
        TParsedSequence second("не хочу");
        UNIT_ASSERT_DOUBLES_EQUAL(0.75, ComputeIntersectionScore(first, second, patch), 1e-3);
    }
}
