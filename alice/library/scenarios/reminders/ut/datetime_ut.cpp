#include "ut.h"

#include <alice/library/scenarios/reminders/datetime.h>

namespace {

using namespace NAlice::NRemindersApi;
using namespace NAlice::NRemindersApi::NTesting;
using namespace NDatetime;

TEST(DateTime, TimeBoundsValidFromTill) {
    TCivilSecond from{2022, 01, 02, 01, 02, 03};
    TCivilSecond till{2022, 02, 03, 02, 03, 04};

    TDateBounds db{from, till, GetTimeZone("Europe/Kiev")};

    ASSERT_EQ(db.IsIn(TCivilSecond{2022, 01, 02, 02, 02, 03}), true);
    ASSERT_EQ(db.IsIn(TCivilSecond{2022, 02, 04, 02, 02, 03}), false);
    ASSERT_EQ(db.IsIn(TCivilSecond{2022, 01, 01, 02, 02, 03}), false);

    ASSERT_EQ(db.FormatFrom("%s"), "1641078123");
    ASSERT_EQ(db.FormatTill("%s"), "1643846584");
}

TEST(DateTime, TimeBoundsInvalidFromTillLessThanDay) {
    // Means today is 01.02.2022 15:00:00 and try to get everyting from this to 14:00
    // after adjust till bound must be chnaged to one day further.
    TCivilSecond from{2022, 02, 01, 15, 0, 0};
    TCivilSecond till{2022, 02, 01, 14, 0, 0};

    TDateBounds db{from, till, GetTimeZone("Europe/Kiev")};

    ASSERT_EQ(db.IsIn(TCivilSecond{2022, 02, 01, 14, 59, 0}), false);
    ASSERT_EQ(db.IsIn(TCivilSecond{2022, 02, 01, 15, 1, 0}), false);

    ASSERT_EQ(db.IsIn(TCivilSecond{2022, 02, 01, 13, 59, 0}), false);
    ASSERT_EQ(db.IsIn(TCivilSecond{2022, 02, 01, 14, 1, 0}), false);

    // Added 1 day to `till`.
    db.AdjustRightBound();

    ASSERT_EQ(db.IsIn(TCivilSecond{2022, 02, 01, 15, 1, 0}), true);
    ASSERT_EQ(db.IsIn(TCivilSecond{2022, 02, 01, 14, 59, 0}), false);

    ASSERT_EQ(db.IsIn(TCivilSecond{2022, 02, 01, 14, 1, 0}), false);
    ASSERT_EQ(db.IsIn(TCivilSecond{2022, 02, 01, 13, 59, 0}), false);
    ASSERT_EQ(db.IsIn(TCivilSecond{2022, 02, 02, 13, 59, 0}), true);
    ASSERT_EQ(db.IsIn(TCivilSecond{2022, 02, 02, 14, 1, 0}), false);

}

} // ns
