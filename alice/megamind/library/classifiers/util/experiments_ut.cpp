#include "experiments.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/maybe.h>

namespace NAlice::NMegamind {

Y_UNIT_TEST_SUITE(GetFormulaExperimentForSpecificClient) {
    Y_UNIT_TEST(WithAbsentFormula) {
        const THashMap<TStringBuf, TStringBuf> flags = {{"something=is_going_on", "1"}};
        const EClientType clientType = ECT_SMART_SPEAKER;
        const auto actual = GetMMFormulaExperimentForSpecificClient(flags, clientType);

        TMaybe<TStringBuf> expected = Nothing();
        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    }
    Y_UNIT_TEST(WithAnotherClient) {
        const THashMap<TStringBuf, TStringBuf> flags = {{"mm_formula__ECT_STRANGE_DEVICE=my_experiment", "1"}};
        const EClientType clientType = ECT_SMART_SPEAKER;
        const auto actual = GetMMFormulaExperimentForSpecificClient(flags, clientType);

        TMaybe<TStringBuf> expected = Nothing();
        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    }
    Y_UNIT_TEST(WithAnotherClient2) {
        const THashMap<TStringBuf, TStringBuf> flags = {{"mm_formula__ECT_SMART_SPEAKER=my_experiment", "1"}};
        const EClientType clientType = ECT_UNKNOWN;
        const auto actual = GetMMFormulaExperimentForSpecificClient(flags, clientType);

        TMaybe<TStringBuf> expected = Nothing();
        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    }
    Y_UNIT_TEST(WithoutEqual) {
        const THashMap<TStringBuf, TStringBuf> flags = {{"mm_formula__ECT_TOUCHmy_experiment", "1"}};
        const EClientType clientType = ECT_TOUCH;
        const auto actual = GetMMFormulaExperimentForSpecificClient(flags, clientType);

        TMaybe<TStringBuf> expected = Nothing();
        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    }
    Y_UNIT_TEST(WithoutEqual2) {
        const THashMap<TStringBuf, TStringBuf> flags = {{"mm_formula__ECT_TOUCH", "1"}};
        const EClientType clientType = ECT_TOUCH;
        const auto actual = GetMMFormulaExperimentForSpecificClient(flags, clientType);

        TMaybe<TStringBuf> expected = Nothing();
        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    }
    Y_UNIT_TEST(WithEmptyExperiment) {
        const THashMap<TStringBuf, TStringBuf> flags = {{"mm_formula__ECT_TOUCH=", "1"}};
        const EClientType clientType = ECT_TOUCH;
        const auto actual = GetMMFormulaExperimentForSpecificClient(flags, clientType);

        TMaybe<TStringBuf> expected = TStringBuf("");
        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    }
    Y_UNIT_TEST(MatchingFlag) {
        const THashMap<TStringBuf, TStringBuf> flags = {{"mm_formula__ECT_TOUCH=my_experiment", "1"}};
        const EClientType clientType = ECT_TOUCH;
        const auto actual = GetMMFormulaExperimentForSpecificClient(flags, clientType);

        TMaybe<TStringBuf> expected = TStringBuf("my_experiment");
        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    }
    Y_UNIT_TEST(MatchingFlagWithEqual) {
        const THashMap<TStringBuf, TStringBuf> flags = {{"mm_formula__ECT_TOUCH=my_experiment=something_else", "1"}};
        const EClientType clientType = ECT_TOUCH;
        const auto actual = GetMMFormulaExperimentForSpecificClient(flags, clientType);

        TMaybe<TStringBuf> expected = TStringBuf("my_experiment=something_else");
        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    }
    Y_UNIT_TEST(FirstWithNothingAfterClientType) {
        const THashMap<TStringBuf, TStringBuf> flags = {{"mm_formula__ECT_TOUCH", "1"}, {"mm_formula__ECT_TOUCH=my_experiment", "1"}};
        const EClientType clientType = ECT_TOUCH;
        const auto actual = GetMMFormulaExperimentForSpecificClient(flags, clientType);

        TMaybe<TStringBuf> expected = TStringBuf("my_experiment");
        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    }
    Y_UNIT_TEST(FirstExpIsOld) {
        const THashMap<TStringBuf, TStringBuf> flags = {{"mm_formula=old_experiment", "1"}, {"mm_formula__ECT_TOUCH=my_experiment", "1"}};
        const EClientType clientType = ECT_TOUCH;
        const auto actual = GetMMFormulaExperimentForSpecificClient(flags, clientType);

        TMaybe<TStringBuf> expected = TStringBuf("my_experiment");
        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    }
    Y_UNIT_TEST(AllClientsNoEqual) {
        const THashMap<TStringBuf, TStringBuf> flags = {{"mm_formulamy_experiment", "1"}};
        const EClientType clientType = ECT_TOUCH;
        const auto actual = GetMMFormulaExperimentForSpecificClient(flags, clientType);

        TMaybe<TStringBuf> expected = Nothing();
        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    }
    Y_UNIT_TEST(AllClientsEmptyExperiment) {
        const THashMap<TStringBuf, TStringBuf> flags = {{"mm_formula=", "1"}};
        const EClientType clientType = ECT_TOUCH;
        const auto actual = GetMMFormulaExperimentForSpecificClient(flags, clientType);

        TMaybe<TStringBuf> expected = TStringBuf("");
        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    }
    Y_UNIT_TEST(AllClientsMatchingFlag) {
        const THashMap<TStringBuf, TStringBuf> flags = {{"mm_formula=my_experiment", "1"}};
        const EClientType clientType = ECT_TOUCH;
        const auto actual = GetMMFormulaExperimentForSpecificClient(flags, clientType);

        TMaybe<TStringBuf> expected = TStringBuf("my_experiment");
        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    }
    Y_UNIT_TEST(AllClientsMatchingFlagWithEqual) {
        const THashMap<TStringBuf, TStringBuf> flags = {{"mm_formula=my_experiment=something_else", "1"}};
        const EClientType clientType = ECT_TOUCH;
        const auto actual = GetMMFormulaExperimentForSpecificClient(flags, clientType);

        TMaybe<TStringBuf> expected = TStringBuf("my_experiment=something_else");
        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    }
}
Y_UNIT_TEST_SUITE(GetPreclassificationConfidentFramesExperiment) {
    Y_UNIT_TEST(CorrectFlag) {
        const THashMap<TStringBuf, TStringBuf> flags = {{"mm_add_preclassifier_confident_frame_Vins=personal_assistant.scenarios.get_weather", "1"}};
        const TStringBuf scenario = "Vins";
        const auto actual = GetPreclassificationConfidentFramesExperiment(flags, scenario);

        TMaybe<TStringBuf> expected = "personal_assistant.scenarios.get_weather";
        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    }
    Y_UNIT_TEST(WrongScenario) {
        const THashMap<TStringBuf, TStringBuf> flags = {{"mm_add_preclassifier_confident_frame_Vins=personal_assistant.scenarios.get_weather", "1"}};
        const TStringBuf scenario = "Search";
        const auto actual = GetPreclassificationConfidentFramesExperiment(flags, scenario);

        TMaybe<TStringBuf> expected = Nothing();
        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    }
    Y_UNIT_TEST(NoFlag) {
        const THashMap<TStringBuf, TStringBuf> flags = {{"mm_add_preclassifier_confident_frameVins=personal_assistant.scenarios.get_weather", "1"}};
        const TStringBuf scenario = "Vins";
        const auto actual = GetPreclassificationConfidentFramesExperiment(flags, scenario);

        TMaybe<TStringBuf> expected = Nothing();
        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    }
}

} // namespace NAlice::NMegamind
