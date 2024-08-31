#include <alice/nlu/libs/token_interval_inflector/token_interval_inflector.h>
#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TokenIntervalInflectorTestSuite) {

Y_UNIT_TEST(TokenIntervalInflector) {
    NAlice::TTokenIntervalInflector inflector;

    // single words
    UNIT_ASSERT_EQUAL(inflector.Inflect({"расскажи", "про", "гагарина"}, {"", "", "acc"}, {2, 3}), "гагарин");
    UNIT_ASSERT_EQUAL(inflector.Inflect({"расскажи", "про", "гагарину"}, {"", "", "acc"}, {2, 3}), "гагарина");
    UNIT_ASSERT_EQUAL(inflector.Inflect({"расскажи", "о", "гагарине"}, {"", "", "loc"}, {2, 3}), "гагарин");
    UNIT_ASSERT_EQUAL(inflector.Inflect({"расскажи", "о", "гагариной"}, {"", "", "loc"}, {2, 3}), "гагарина");
    UNIT_ASSERT_EQUAL(inflector.Inflect({"расскажи", "про", "королева"}, {"", "", "acc"}, {2, 3}), "королев");

    // phrases
    UNIT_ASSERT_EQUAL(inflector.Inflect({"расскажи", "про", "юрия", "гагарина"}, {"", "", "acc", "acc"}, {2, 4}), "юрий гагарин");
    UNIT_ASSERT_EQUAL(inflector.Inflect({"расскажи", "про", "улицу", "гагарина"}, {"", "", "acc", "gen"}, {2, 4}), "улица гагарина");
}

}
