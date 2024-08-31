#include <library/cpp/testing/unittest/registar.h>

#include <alice/megamind/library/session/dialog_history.h>

using namespace NAlice;

Y_UNIT_TEST_SUITE(DialogHistory) {
    Y_UNIT_TEST(MakeDialogHistory) {
        static_assert(TDialogHistory::MAX_TURNS_COUNT == 4);

        TDialogHistory dialogHistory({{"0", "0", "0", "0", 0, 0}, {"1", "1", "1", "1", 1, 1},
                {"2", "2", "2", "2", 2, 2}, {"3", "3", "3", "3", 3, 3}, {"4", "4", "4", "4", 4, 4}, {"5", "5", "5", "5", 5, 5}});

        const TDeque<TDialogHistory::TDialogTurn> expected = {{"2", "2", "2", "2", 2, 2}, {"3", "3", "3", "3", 3, 3}, {"4", "4", "4", "4", 4, 4}, {"5", "5", "5", "5", 5, 5}};
        UNIT_ASSERT_VALUES_EQUAL(dialogHistory.GetDialogTurns(), expected);
    }

    Y_UNIT_TEST(PushToDialogHistory) {
        static_assert(TDialogHistory::MAX_TURNS_COUNT == 4);

        TDialogHistory dialogHistory;

        {
            dialogHistory.PushDialogTurn({"0", "0", "0", "0", 0, 0});
            const TDeque<TDialogHistory::TDialogTurn> expected = {{"0", "0", "0", "0", 0, 0}};
            UNIT_ASSERT_VALUES_EQUAL(dialogHistory.GetDialogTurns(), expected);
        }
        {
            dialogHistory.PushDialogTurn({"1", "1", "1", "1", 1, 1});
            const TDeque<TDialogHistory::TDialogTurn> expected = {{"0", "0", "0", "0", 0, 0}, {"1", "1", "1", "1", 1, 1}};
            UNIT_ASSERT_VALUES_EQUAL(dialogHistory.GetDialogTurns(), expected);
        }
        {
            dialogHistory.PushDialogTurn({"2", "2", "2", "2", 2, 2});
            const TDeque<TDialogHistory::TDialogTurn> expected = {{"0", "0", "0", "0", 0, 0}, {"1", "1", "1", "1", 1, 1}, {"2", "2", "2", "2", 2, 2}};
            UNIT_ASSERT_VALUES_EQUAL(dialogHistory.GetDialogTurns(), expected);
        }
        {
            dialogHistory.PushDialogTurn({"3", "3", "3", "3", 3, 3});
            const TDeque<TDialogHistory::TDialogTurn> expected = {{"0", "0", "0", "0", 0, 0}, {"1", "1", "1", "1", 1, 1}, {"2", "2", "2", "2", 2, 2}, {"3", "3", "3", "3", 3, 3}};
            UNIT_ASSERT_VALUES_EQUAL(dialogHistory.GetDialogTurns(), expected);
        }
        {
            dialogHistory.PushDialogTurn({"4", "4", "4", "4", 4, 4});
            const TDeque<TDialogHistory::TDialogTurn> expected = {{"1", "1", "1", "1", 1, 1}, {"2", "2", "2", "2", 2, 2}, {"3", "3", "3", "3", 3, 3}, {"4", "4", "4", "4", 4, 4}};
            UNIT_ASSERT_VALUES_EQUAL(dialogHistory.GetDialogTurns(), expected);
        }
        {
            dialogHistory.PushDialogTurn({"5", "5", "5", "5", 5, 5});
            const TDeque<TDialogHistory::TDialogTurn> expected = {{"2", "2", "2", "2", 2, 2}, {"3", "3", "3", "3", 3, 3}, {"4", "4", "4", "4", 4, 4}, {"5", "5", "5", "5", 5, 5}};
            UNIT_ASSERT_VALUES_EQUAL(dialogHistory.GetDialogTurns(), expected);
        }

    }
}
