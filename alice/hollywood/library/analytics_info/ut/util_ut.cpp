#include <alice/hollywood/library/analytics_info/util.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywood {

Y_UNIT_TEST_SUITE(AnalyticsInfoUtil) {

Y_UNIT_TEST(TimePointWhenToStrTest) {
    // e.g. "Перемотай на {time point when}"
    UNIT_ASSERT_STRINGS_EQUAL(TimePointWhenToStr(0), "самое начало");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointWhenToStr(1), "1 секунду");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointWhenToStr(2), "2 секунду");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointWhenToStr(3), "3 секунду");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointWhenToStr(4), "4 секунду");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointWhenToStr(5), "5 секунду");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointWhenToStr(6), "6 секунду");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointWhenToStr(7), "7 секунду");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointWhenToStr(8), "8 секунду");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointWhenToStr(9), "9 секунду");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointWhenToStr(10), "10 секунду");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointWhenToStr(11), "11 секунду");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointWhenToStr(12), "12 секунду");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointWhenToStr(13), "13 секунду");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointWhenToStr(14), "14 секунду");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointWhenToStr(15), "15 секунду");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointWhenToStr(16), "16 секунду");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointWhenToStr(17), "17 секунду");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointWhenToStr(18), "18 секунду");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointWhenToStr(19), "19 секунду");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointWhenToStr(20), "20 секунду");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointWhenToStr(21), "21 секунду");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointWhenToStr(59), "59 секунду");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointWhenToStr(60), "1 минуту");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointWhenToStr(61), "1 минуту 1 секунду");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointWhenToStr(62), "1 минуту 2 секунду");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointWhenToStr(120), "2 минуту");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointWhenToStr(121), "2 минуту 1 секунду");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointWhenToStr(122), "2 минуту 2 секунду");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointWhenToStr(123), "2 минуту 3 секунду");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointWhenToStr(124), "2 минуту 4 секунду");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointWhenToStr(125), "2 минуту 5 секунду");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointWhenToStr(180), "3 минуту");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointWhenToStr(181), "3 минуту 1 секунду");
}

Y_UNIT_TEST(TimeAmountToStrTest) {
    // e.g. "До таймера осталось {time amount}"
    UNIT_ASSERT_STRINGS_EQUAL(TimeAmountToStr(0), "0 секунд");
    UNIT_ASSERT_STRINGS_EQUAL(TimeAmountToStr(1), "1 секунда");
    UNIT_ASSERT_STRINGS_EQUAL(TimeAmountToStr(2), "2 секунды");
    UNIT_ASSERT_STRINGS_EQUAL(TimeAmountToStr(3), "3 секунды");
    UNIT_ASSERT_STRINGS_EQUAL(TimeAmountToStr(4), "4 секунды");
    UNIT_ASSERT_STRINGS_EQUAL(TimeAmountToStr(5), "5 секунд");
    UNIT_ASSERT_STRINGS_EQUAL(TimeAmountToStr(6), "6 секунд");
    UNIT_ASSERT_STRINGS_EQUAL(TimeAmountToStr(7), "7 секунд");
    UNIT_ASSERT_STRINGS_EQUAL(TimeAmountToStr(8), "8 секунд");
    UNIT_ASSERT_STRINGS_EQUAL(TimeAmountToStr(9), "9 секунд");
    UNIT_ASSERT_STRINGS_EQUAL(TimeAmountToStr(10), "10 секунд");
    UNIT_ASSERT_STRINGS_EQUAL(TimeAmountToStr(11), "11 секунд");
    UNIT_ASSERT_STRINGS_EQUAL(TimeAmountToStr(12), "12 секунд");
    UNIT_ASSERT_STRINGS_EQUAL(TimeAmountToStr(13), "13 секунд");
    UNIT_ASSERT_STRINGS_EQUAL(TimeAmountToStr(14), "14 секунд");
    UNIT_ASSERT_STRINGS_EQUAL(TimeAmountToStr(15), "15 секунд");
    UNIT_ASSERT_STRINGS_EQUAL(TimeAmountToStr(16), "16 секунд");
    UNIT_ASSERT_STRINGS_EQUAL(TimeAmountToStr(17), "17 секунд");
    UNIT_ASSERT_STRINGS_EQUAL(TimeAmountToStr(18), "18 секунд");
    UNIT_ASSERT_STRINGS_EQUAL(TimeAmountToStr(19), "19 секунд");
    UNIT_ASSERT_STRINGS_EQUAL(TimeAmountToStr(20), "20 секунд");
    UNIT_ASSERT_STRINGS_EQUAL(TimeAmountToStr(21), "21 секунда");
    UNIT_ASSERT_STRINGS_EQUAL(TimeAmountToStr(59), "59 секунд");
    UNIT_ASSERT_STRINGS_EQUAL(TimeAmountToStr(60), "1 минута");
    UNIT_ASSERT_STRINGS_EQUAL(TimeAmountToStr(61), "1 минута 1 секунда");
    UNIT_ASSERT_STRINGS_EQUAL(TimeAmountToStr(62), "1 минута 2 секунды");
    UNIT_ASSERT_STRINGS_EQUAL(TimeAmountToStr(120), "2 минуты");
    UNIT_ASSERT_STRINGS_EQUAL(TimeAmountToStr(121), "2 минуты 1 секунда");
    UNIT_ASSERT_STRINGS_EQUAL(TimeAmountToStr(122), "2 минуты 2 секунды");
    UNIT_ASSERT_STRINGS_EQUAL(TimeAmountToStr(123), "2 минуты 3 секунды");
    UNIT_ASSERT_STRINGS_EQUAL(TimeAmountToStr(124), "2 минуты 4 секунды");
    UNIT_ASSERT_STRINGS_EQUAL(TimeAmountToStr(125), "2 минуты 5 секунд");
    UNIT_ASSERT_STRINGS_EQUAL(TimeAmountToStr(180), "3 минуты");
    UNIT_ASSERT_STRINGS_EQUAL(TimeAmountToStr(181), "3 минуты 1 секунда");
}

Y_UNIT_TEST(TimePointAtToStrTest) {
    // e.g. "Игра окончилась на {time point at}"
    UNIT_ASSERT_STRINGS_EQUAL(TimePointAtToStr(0), "самом начале");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointAtToStr(1), "1 секунде");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointAtToStr(2), "2 секунде");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointAtToStr(3), "3 секунде");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointAtToStr(4), "4 секунде");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointAtToStr(5), "5 секунде");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointAtToStr(6), "6 секунде");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointAtToStr(7), "7 секунде");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointAtToStr(8), "8 секунде");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointAtToStr(9), "9 секунде");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointAtToStr(10), "10 секунде");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointAtToStr(11), "11 секунде");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointAtToStr(12), "12 секунде");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointAtToStr(13), "13 секунде");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointAtToStr(14), "14 секунде");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointAtToStr(15), "15 секунде");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointAtToStr(16), "16 секунде");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointAtToStr(17), "17 секунде");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointAtToStr(18), "18 секунде");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointAtToStr(19), "19 секунде");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointAtToStr(20), "20 секунде");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointAtToStr(21), "21 секунде");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointAtToStr(59), "59 секунде");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointAtToStr(60), "1 минуте");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointAtToStr(61), "1 минуте 1 секунде");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointAtToStr(62), "1 минуте 2 секунде");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointAtToStr(120), "2 минуте");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointAtToStr(121), "2 минуте 1 секунде");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointAtToStr(122), "2 минуте 2 секунде");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointAtToStr(123), "2 минуте 3 секунде");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointAtToStr(124), "2 минуте 4 секунде");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointAtToStr(125), "2 минуте 5 секунде");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointAtToStr(180), "3 минуте");
    UNIT_ASSERT_STRINGS_EQUAL(TimePointAtToStr(181), "3 минуте 1 секунде");
}

}

} // namespace NAlice::NHollywood
