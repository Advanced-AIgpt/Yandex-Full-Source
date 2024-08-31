#include "sample_features.h"

#include <library/cpp/testing/unittest/registar.h>

namespace NVins {

namespace {

Y_UNIT_TEST_SUITE(SampleFeatures) {

Y_UNIT_TEST(Simple) {
    UNIT_ASSERT_VALUES_EQUAL(
        TVector<TString>({"O", "O", "O"}),
        GetTags(
            "find in yandex",
            {"find", "in", "yandex"},
            LANG_RUS
        )
    );

    UNIT_ASSERT_VALUES_EQUAL(
        TVector<TString>({"B-a", "O", "B-bb", "I-bb", "O", "O", "B-ccc", "I-ccc", "I-ccc", "O", "O", "O"}),
        GetTags(
            "'one'(a) x 'two three'(bb) x x 'four five six'(ccc) x x x",
            {"one", "x", "two", "three", "x", "x", "four", "five", "six", "x", "x", "x"},
            LANG_RUS
        )
    );
}

Y_UNIT_TEST(Continuation) {
    UNIT_ASSERT_VALUES_EQUAL(
        TVector<TString>({"B-where", "B-what", "B-open", "I-open", "O", "I-where"}),
        GetTags(
            "'ближайшие'(where) 'аптеки'(what) 'двадцать четыре часа'(open) в 'анге'(+where)",
            {"ближайшие", "аптеки", "24", "часа", "в", "анге"},
            LANG_RUS
        )
    );

    UNIT_ASSERT_VALUES_EQUAL(
        TVector<TString>({"B-where", "B-what", "B-open", "I-open", "O", "I-where"}),
        GetTags(
            "'ближайшие'(where) 'аптеки'(what) 'двадцать четыре часа'(open) в 'анге'( where)",
            {"ближайшие", "аптеки", "24", "часа", "в", "анге"},
            LANG_RUS
        )
    );

    UNIT_ASSERT_VALUES_EQUAL(
        TVector<TString>({"O", "O", "O", "B-time", "B-day_part"}),
        GetTags(
            "ايقظني في الساعة"
            "'"
            "تسعة"
            "'(time)"
            "'"
            "ليلاً"
            "'(day_part)",
            {
                "ايقظني",
                "في",
                "الساعة",
                "تسعة",
                "ليلاً"
            },
            LANG_ARA
        )
    );
}

}

} // namespace

} // namespace NVins
