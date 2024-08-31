#include "date.h"
#include <alice/library/proto/proto.h>
#include <library/cpp/scheme/scheme.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NBg;
using namespace NBg::NAliceEntityCollector;

Y_UNIT_TEST_SUITE(Date) {

    NSc::TValue CollectValues(const TVector<NGranet::TEntity>& entities) {
        NSc::TValue values;
        values.SetArray();
        for (const NGranet::TEntity& entity : entities) {
            values.Push(NSc::TValue::FromJson(entity.Value));
        }
        return values;
    }

    void TestDateValues(TStringBuf markupStr, TStringBuf expectedStr) {
        const NSc::TValue expected = NSc::TValue::FromJson(expectedStr);
        const auto markup = NAlice::ParseProtoText<NProto::TExternalMarkupProto>(markupStr);
        const NSc::TValue actual = CollectValues(CollectPASkillsDate(markup));
        UNIT_ASSERT_VALUES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(Entity) {
        const auto markup = NAlice::ParseProtoText<NProto::TExternalMarkupProto>(R"(
            Tokens {Text: "24"}
            Tokens {Text: "января"}
            Date {
                Tokens {Begin: 0, End: 2}
                Day: 24
                Month: 1
            }
        )");
        const TVector<NGranet::TEntity> expected = {
            {
                .Interval = {0, 2},
                .Type = "YANDEX.DATETIME",
                .Value = R"({"day":24,"month":1})",
                .LogProbability = -3,
            }
        };
        UNIT_ASSERT_VALUES_EQUAL(expected, CollectPASkillsDate(markup));
    }

    Y_UNIT_TEST(Full) {
        const TStringBuf markup = R"(
            Tokens {Text: "24"}
            Tokens {Text: "01"}
            Tokens {Text: "1980"}
            Date {
                Tokens {Begin: 0, End: 3}
                Day: 24
                Month: 1
                Year: 1980
            }
        )";
        const TStringBuf expected = R"([
            {
                "day": 24,
                "month": 1,
                "year": 1980
            }
        ])";
        TestDateValues(markup, expected);
    }

    Y_UNIT_TEST(Hour) {
        const TStringBuf markup = R"(
            Tokens {Text: "в"}
            Tokens {Text: "2"}
            Tokens {Text: "часа"}
            Date {
                Tokens {Begin: 0, End: 3}
                Hour: 2
            }
        )";
        const TStringBuf expected = R"([
            {
                "hour": 2
            }
        ])";
        TestDateValues(markup, expected);
    }

    Y_UNIT_TEST(HourPM) {
        const TStringBuf markup = R"(
            Tokens {Text: "в"}
            Tokens {Text: "2"}
            Tokens {Text: "часа"}
            Tokens {Text: "дня"}
            Date {
                Tokens {Begin: 0, End: 4}
                Hour: 14
            }
        )";
        const TStringBuf expected = R"([
            {
                "hour": 14
            }
        ])";
        TestDateValues(markup, expected);
    }

    Y_UNIT_TEST(Midnight) {
        const TStringBuf markup = R"(
            Tokens {Text: "в"}
            Tokens {Text: "полночь"}
            Date {
                Tokens {Begin: 0, End: 2}
                Hour: 0
            }
        )";
        const TStringBuf expected = R"([
            {
                "hour": 0
            }
        ])";
        TestDateValues(markup, expected);
    }

    Y_UNIT_TEST(Interval) {
        const TStringBuf markup = R"(

            Tokens {Text: "с"}
            Tokens {Text: "20"}
            Tokens {Text: "10"}
            Tokens {Text: "до"}
            Tokens {Text: "21"}
            Tokens {Text: "25"}
            Date {
                Tokens {Begin: 0, End: 3}
                Hour: 20
                Min: 10
                IntervalBegin: true
            }
            Date {
                Tokens {Begin: 3, End: 6}
                Hour: 21
                Min: 25
                IntervalEnd: true
            }
        )";
        const TStringBuf expected = R"([
            {
                "hour": 20,
                "minute": 10
            },
            {
                "hour": 21,
                "minute": 25
            }
        ])";
        TestDateValues(markup, expected);
    }

    Y_UNIT_TEST(DayRelative) {
        const TStringBuf markup = R"(
            Tokens {Text: "вчера"}
            Date {
                Tokens {Begin: 0, End: 1}
                Day: -1
                RelativeDay: true
            }
        )";
        const TStringBuf expected = R"([
            {
                "day": -1,
                "day_is_relative": true
            }
        ])";
        TestDateValues(markup, expected);
    }

    Y_UNIT_TEST(MinuteForward) {
        const TStringBuf markup = R"(
            Tokens {Text: "через"}
            Tokens {Text: "15"}
            Tokens {Text: "минут"}
            Date {
                Tokens {Begin: 0, End: 3}
                Duration {
                    Type: "FORWARD"
                    Min: 15
                }
            }
        )";
        const TStringBuf expected = R"([
            {
                "minute": 15,
                "minute_is_relative": true
            }
        ])";
        TestDateValues(markup, expected);
    }

    Y_UNIT_TEST(WeekForward) {
        const TStringBuf markup = R"(
            Tokens {Text: "через"}
            Tokens {Text: "2"}
            Tokens {Text: "недели"}
            Date {
                Tokens {Begin: 0, End: 3}
                Duration {
                    Type: "FORWARD"
                    Week: 2
                }
            }
        )";
        const TStringBuf expected = R"([
            {
                "day": 14,
                "day_is_relative": true
            }
        ])";
        TestDateValues(markup, expected);
    }

    Y_UNIT_TEST(YearBack) {
        const TStringBuf markup = R"(
            Tokens {Text: "3"}
            Tokens {Text: "года"}
            Tokens {Text: "тому"}
            Tokens {Text: "назад"}
            Date {
                Tokens {Begin: 0, End: 4}
                Duration {
                    Type: "BACK"
                    Year: 3
                }
            }
        )";
        const TStringBuf expected = R"([
            {
                "year": -3,
                "year_is_relative": true
            }
        ])";
        TestDateValues(markup, expected);
    }

    Y_UNIT_TEST(RelativeMix) {
        const TStringBuf markup = R"(
            Tokens {Text: "поставь"}
            Tokens {Text: "будильник"}
            Tokens {Text: "на"}
            Tokens {Text: "завтра"}
            Tokens {Text: "в"}
            Tokens {Text: "7"}
            Tokens {Text: "утра"}
            Date {
                Tokens {Begin: 2, End: 7}
                Day: 1
                RelativeDay: true
                Hour: 7
                Prep: "на"
            }
        )";
        const TStringBuf expected = R"([
            {
                "day": 1,
                "day_is_relative": true,
                "hour": 7
            }
        ])";
        TestDateValues(markup, expected);
    }

    Y_UNIT_TEST(Periodical) {
        // Not supported by converter
        const TStringBuf markup = R"(
            Tokens {Text: "каждый"}
            Tokens {Text: "день"}
            Date {
                Tokens {Begin: 0, End: 2}
                Duration {
                    Type: "PERIODICAL"
                    Day: 1
                }
            }
        )";
        const TStringBuf expected = R"([
        ])";
        TestDateValues(markup, expected);
    }

     Y_UNIT_TEST(PeriodicalWithTimes) {
        // Not supported by converter
        const TStringBuf markup = R"(
            Tokens {Text: "3"}
            Tokens {Text: "раза"}
            Tokens {Text: "в"}
            Tokens {Text: "час"}
            Date {
                Tokens {Begin: 0, End: 4}
                Duration {
                    Type: "PERIODICAL"
                    Hour: 1
                    Times: 3
                }
            }
        )";
        const TStringBuf expected = R"([
        ])";
        TestDateValues(markup, expected);
    }

    Y_UNIT_TEST(Empty) {
        const TStringBuf markup = R"(
            Tokens {Text: "в"}
            Tokens {Text: "следующий"}
            Tokens {Text: "вторник"}
            Date {
                Tokens {Begin: 2, End: 3}
            }
        )";
        const TStringBuf expected = R"([
        ])";
        TestDateValues(markup, expected);
    }
}
