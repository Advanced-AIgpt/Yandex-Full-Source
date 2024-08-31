#include "user_bookmarks.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/algorithm.h>
#include <util/generic/vector.h>

#include <utility>

using namespace NBASS;

namespace {

Y_UNIT_TEST_SUITE(TBookmarksMatcherUnitTest) {
    Y_UNIT_TEST(Smoke) {
        const TVector<TStringBuf> bookmarks = {
            {"улица Лейтезена", "улица Лейтизена", "улица Спортивная", "Ростов Бабушка", "Москва Бабушка",
             "улица ленина 17", "Роза Хутор", "улица кожевническая, 14с5", "Дом", "Работа", "баба света", "враг мой",
             "художественная школа", "школа", "музыкальная школа", "⚽Светик😜"}};

        TUserBookmarksHelper bookmarksHelper(bookmarks);
        {
            const auto result = bookmarksHelper.GetUserBookmark("на улицу лейтизена");
            UNIT_ASSERT(result);
            UNIT_ASSERT_STRINGS_EQUAL(result->Name(), "улица Лейтизена");
        }

        {
            const auto result = bookmarksHelper.GetUserBookmark("улица ленина 17");
            UNIT_ASSERT(result);
            UNIT_ASSERT_STRINGS_EQUAL(result->Name(), "улица ленина 17");
        }

        for (const auto query : {"улица ленина 16", "ленинский 17"}) {
            const auto result = bookmarksHelper.GetUserBookmark(query);
            UNIT_ASSERT(!result);
        }

        for (const auto query : {"к бабушке", "в ростов к бабушке", "к бабушке в ростове"}) {
            const auto result = bookmarksHelper.GetUserBookmark(query);
            UNIT_ASSERT(result);
            UNIT_ASSERT_STRINGS_EQUAL(result->Name(), "Ростов Бабушка");
        }

        for (const auto query : {"в москву к бабушке", "к бабушке в москве"}) {
            const auto result = bookmarksHelper.GetUserBookmark(query);
            UNIT_ASSERT(result);
            UNIT_ASSERT_STRINGS_EQUAL(result->Name(), "Москва Бабушка");
        }

        {
            const auto result = bookmarksHelper.GetUserBookmark("в розу хутор");
            UNIT_ASSERT(result);
            UNIT_ASSERT_STRINGS_EQUAL(result->Name(), "Роза Хутор");
        }

        // "хутор" is a geo stop word, so it's better not to match it with "Роза Хутор".
        {
            const auto result = bookmarksHelper.GetUserBookmark("хутор");
            UNIT_ASSERT(!result);
        }

        {
            const auto result = bookmarksHelper.GetUserBookmark("кожевническая 14");
            UNIT_ASSERT(result);
            UNIT_ASSERT_STRINGS_EQUAL(result->Name(), "улица кожевническая, 14с5");
        }

        {
            const auto result = bookmarksHelper.GetSavedAddressBookmark(TSpecialLocation(TSpecialLocation::EType::HOME), "дом");
            UNIT_ASSERT(result);
            UNIT_ASSERT_STRINGS_EQUAL(result->Name(), "Дом");
        }

        {
            const auto result = bookmarksHelper.GetSavedAddressBookmark(TSpecialLocation(TSpecialLocation::EType::WORK), "работа");
            UNIT_ASSERT(result);
            UNIT_ASSERT_STRINGS_EQUAL(result->Name(), "Работа");
        }

        for (const auto& query : {"бабы света", "где бабы света", "баба света", "где находится баба света"}) {
            const auto result = bookmarksHelper.GetUserBookmark(query);
            UNIT_ASSERT(result);
            UNIT_ASSERT_STRINGS_EQUAL(result->Name(), "баба света");
        }

        for (const auto& query : {"мой враг", "где мой враг"}) {
            const auto result = bookmarksHelper.GetUserBookmark(query);
            UNIT_ASSERT(result);
            UNIT_ASSERT_STRINGS_EQUAL(result->Name(), "враг мой");
        }

        {
            const auto result = bookmarksHelper.GetUserBookmark("школа");
            UNIT_ASSERT(result);
            UNIT_ASSERT_STRINGS_EQUAL(result->Name(), "школа");
        }

        {
            const auto result = bookmarksHelper.GetUserBookmark("спортивная школа");
            UNIT_ASSERT(!result);
        }

        {
            const auto result = bookmarksHelper.GetUserBookmark("к светику");
            UNIT_ASSERT(result);
            UNIT_ASSERT_STRINGS_EQUAL(result->Name(), "⚽Светик😜");
        }
    }
}
} // namespace
