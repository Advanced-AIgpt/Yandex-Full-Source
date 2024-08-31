#include "aligner.h"
#include "joined_tokens.h"
#include <library/cpp/testing/unittest/registar.h>
#include <util/string/join.h>
#include <util/string/split.h>
#include <util/string/strip.h>

using namespace NNlu;

Y_UNIT_TEST_SUITE(TTokenAligner) {

    TVector<TStringBuf> ReadTokens(TStringBuf data) {
        return StringSplitter(data).SplitBySet("| ").SkipEmpty().ToList<TStringBuf>();
    }

    TVector<size_t> ReadSegments(TStringBuf data) {
        TVector<size_t> segments;
        for (TStringBuf group : StringSplitter(data).Split('|')) {
            segments.push_back(StringSplitter(group).Split(' ').SkipEmpty().Count());
        }
        return segments;
    }

    TVector<bool> ReadEquality(TStringBuf data) {
        TVector<bool> result;
        for (const char c : data) {
            if (c == '=') {
                result.push_back(true);
            } else if (c == '~') {
                result.push_back(false);
            }
        }
        return result;
    }

    void CheckResult(TStringBuf data1, TStringBuf data2, TStringBuf equality, const TAlignment& actualAlignment) {
        const TAlignment expectedAlignment({ReadSegments(data1), ReadSegments(data2), ReadEquality(equality)});
        const TString expected = expectedAlignment.WriteToString();
        const TString actual = actualAlignment.WriteToString();
        if (expected == actual) {
            return;
        }
        TStringBuilder message;
        message << Endl;
        message << "TTokenAligner error:" << Endl;
        message << "  data1:    " << data1 << Endl;
        message << "  data2:    " << data2 << Endl;
        message << "  expected: " << expected << Endl;
        message << "  actual:   " << actual << Endl;
        UNIT_FAIL(message);
    }

    static const std::tuple<TStringBuf, TStringBuf, TStringBuf> TestData[] = {
        {
            "две тысячи двадцатый|двадцать одна тысяча пятьсот|двадцать тысяч пятьсот",
            "2020                |21500                       |20500",
            "~                   |~                           |~"
        },
        {
            "тысяча пятьсот|одна тысяча пятьсот",
            "1500          |1500",
            "=             |~"
        },
        {
            "Сколько|будет|дважды|два",
            "сколько|будет|дважды|2",
            "=       =     =      ="
        },
        {
            "Сколько|будет|минус|сто двадцать три|плюс|пятьдесят семь",
            "сколько|будет|-    |123             |+   |57",
            "=       =     =     =                =    ="
        },
        {
            "АИ95 |на|тысячу двести",
            "АИ 95|на|1200",
            "~     =  ="
        },
        {
            "две с половиной|тысячи|рублей",
            "2,5            |1000  |рублей",
            "~               =      ="
        },
        {
            "громкость|на|три с половиной|пожалуйста",
            "громкость|на|3,5            |пожалуйста",
            "=         =  ~               ="
        },
        {
            "дом|тридцать четыре|дробь|два",
            "дом|34             |/    |2",
            "=   =               =     ="
        },
        {
            "дом|восемь|дробь|а",
            "дом|8     |дробь|а",
            "=   =      =     ="
        },
        {
            "сорок|-|имя|числительное",
            "40   |-|имя|числительное",
            "=     = =   ="
        },
        {
            "Четыреста семьдесят восемь миллиардов пятьсот одиннадцать тысяч семь",
            "478000511007",
            "~"
        },
        {
            "Грюнвальдская|битва|произошла|в|тысяча четыреста десятом|году",
            "грюнвальдская|битва|произошла|в|1410                    |году",
            "=             =     =         = ~                        ="
        },
        {
            "Николай|II|правил|с|тысяча восемьсот девяносто четвёртого|года|по|тысяча девятьсот семнадцатый|год",
            "николай|ii|правил|с|1894                                 |года|по|1917                        |год",
            "=       =  =      = =                                     =    =  =                            ="
        },
        {
            "x x x x x x x x|A|x|x|x|x|x|x|x|x|x|x|x|2|x x x x x x x x x x x x    |B|x x x x",
            "2 2 2 2 2 2 2  |A|2|2|2|2|2|2|2|2|2|2|2|2|2 2 2 2 2 2 2 2 2 2 2 2 2 2|B|2 2 2 2 2",
            "~               = ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ = ~                           = ~"
        },
        {
            "громкость|на|три с половиной|пожалуйста",
            "громкость|на|3,5            |пожалуйста",
            "=         =  ~               ="
        },
        {
            "останови ка|четырнадцатый",
            "останови-ка|14",
            "=           ="
        },
        {
            "а|ну ка|давай ка|двадцать третий",
            "а|ну-ка|давай-ка|23",
            "= =     =        ="
        },
        {
            "тысяча девятьсот семнадцатый|две тысячи двадцатый",
            "1917                        |2020",
            "=                            ~"
        },
    };

    Y_UNIT_TEST(Align) {
        for (const auto& [data1, data2, equality] : TestData) {
            const TVector<TStringBuf> tokens1 = ReadTokens(data1);
            const TVector<TStringBuf> tokens2 = ReadTokens(data2);
            const TAlignment actual = TTokenAligner::Align(tokens1, tokens2);
            CheckResult(data1, data2, equality, actual);
        }
    }

    Y_UNIT_TEST(JoinedTokens) {
        for (const auto& [data1, data2, equality] : TestData) {
            const TString tokens1 = NJoinedTokens::JoinTokens(ReadTokens(data1));
            const TString tokens2 = NJoinedTokens::JoinTokens(ReadTokens(data2));
            const TAlignment actual = TTokenAligner::Align(tokens1, tokens2);
            CheckResult(data1, data2, equality, actual);
        }
    }

    Y_UNIT_TEST(CachedAligner) {
        TTokenCachedAligner aligner;
        for (const auto& [data1, data2, equality] : TestData) {
            const TVector<TStringBuf> tokens1 = ReadTokens(data1);
            const TVector<TStringBuf> tokens2 = ReadTokens(data2);
            const TAlignment actual = aligner.Align(tokens1, tokens2);
            CheckResult(data1, data2, equality, actual);
        }
    }

    void TestInterval(TStringBuf text1, TStringBuf text2, const TInterval& src, const TInterval& interval,
        bool hasSure, bool hasStrict)
    {
        const TAlignment alignment = TTokenAligner::Align(text1, text2);
        const TAlignmentMap& map = alignment.GetMap1To2();
        if (map.ConvertInterval(src) == interval
            && map.HasSureMatch(src) == hasSure
            && map.HasStrictMatch(src) == hasStrict)
        {
            return;
        }
        TStringBuilder message;
        message << Endl;
        message << "TTokenAligner error:" << Endl;
        message << "  text1: " << text1 << Endl;
        message << "  text2: " << text2 << Endl;
        message << "  interval: expected " << interval << ", actual " << map.ConvertInterval(src) << Endl;
        message << "  hasSure: expected " << hasSure << ", actual " << map.HasSureMatch(src) << Endl;
        message << "  hasStrict: expected " << hasStrict << ", actual " << map.HasStrictMatch(src) << Endl;
        UNIT_FAIL(message);
    }

    Y_UNIT_TEST(EllipsisRewriter) {
        TestInterval("расписание электричек в саратов сегодня", "нет лучше сегодня", {4, 5}, {2, 3}, true, true);
        TestInterval("расписание электричек в саратов сегодня", "нет лучше сегодня", {3, 4}, {0, 2}, false, false);
        TestInterval("расписание электричек в саратов сегодня", "нет лучше сегодня", {2, 5}, {0, 3}, true, false);
    }

    Y_UNIT_TEST(InvalidInterval) {
        TestInterval("0 1 2", "0 1 2", {4, 5}, {3, 3}, false, false);
    }

    Y_UNIT_TEST(InvalidAll) {
        const TString texts[] = {
            "",
            " ",
            "       ",
            "1 ",
            "  123 45 -   _a",
            "сто двадцать   четыре + АААЁЁЁ-. ",
            "  тысяча--сто-24'четыре''' '' -',-",
        };
        const TInterval intervals[] = {
            {0, 0},
            {0, 1},
            {2, 1000},
            {1000, 1000},
            {1000, 2000},
        };
        TTokenCachedAligner aligner;
        for (const TString& text1 : texts) {
            for (const TString& text2 : texts) {
                const TAlignment alignment = aligner.Align(text1, text2);
                for (const TInterval& interval : intervals) {
                    alignment.GetMap1To2().ConvertInterval(interval);
                    alignment.GetMap1To2().ConvertInterval(interval);
                }
            }
        }
    }

    void TestRewriterUseCase(const std::tuple<TStringBuf, TStringBuf, TStringBuf>& texts,
        const TInterval& src, bool expectedHasMatch, const TInterval& expectedInterval = {})
    {
        const auto& [text1, text2, text3] = texts;
        NNlu::TAlignment alignment1;
        NNlu::TAlignment alignment2;
        if (!text2.empty()) {
            alignment1 = TTokenAligner::Align(text1, text2);
            alignment2 = TTokenAligner::Align(text2, text3);
        } else {
            // Skip text2
            alignment1 = NNlu::TAlignment::CreateTrivial(NJoinedTokens::CountTokens(text1));
            alignment2 = TTokenAligner::Align(text1, text3);
        }
        const NNlu::TAlignmentMap& map1 = alignment1.GetMap1To2();
        const NNlu::TAlignmentMap& map2 = alignment2.GetMap1To2();

        const bool actualHasMatch = map1.HasSureMatch(src);
        if (!expectedHasMatch && !actualHasMatch) {
            return;
        }
        const TInterval actualInterval = map2.ConvertInterval(map1.ConvertInterval(src));
        if (expectedHasMatch == actualHasMatch
            && expectedInterval == actualInterval)
        {
            return;
        }
        TStringBuilder message;
        message << Endl;
        message << "TestRewriterUseCase error:" << Endl;
        message << "  text1: " << text1 << Endl;
        message << "  text2: " << text2 << Endl;
        message << "  text3: " << text3 << Endl;
        message << "  alignment1: " << alignment1.WriteToString() << Endl;
        message << "  alignment2: " << alignment2.WriteToString() << Endl;
        message << "  interval: " << src << " -> " << map1.ConvertInterval(src) << " -> " << actualInterval << Endl;
        message << "  expectedInterval: " << expectedInterval << Endl;
        message << "  hasMatch: expected " << expectedHasMatch << ", actual " << actualHasMatch << Endl;
        UNIT_FAIL(message);
    }

    Y_UNIT_TEST(EllipsisRewriter1) {
        const auto& texts = std::make_tuple(
            "расписание электричек в саратов 23 сентября", // rewriter text
            "нет лучше 23 сентября", // normalized text
            "нет лучше двадцать третье сентября" // original text
        );
        TestRewriterUseCase(texts, {4, 6}, true, {2, 5}); // 23 сентября -> двадцать третье сентября
        TestRewriterUseCase(texts, {4, 5}, true, {2, 4}); // 23 -> двадцать третье
        TestRewriterUseCase(texts, {3, 4}, false); // саратов -> no
    }

    Y_UNIT_TEST(EllipsisRewriter2) {
        const auto& texts = std::make_tuple(
            "нет лучше 23 сентября",
            "", // not defined, should skip
            "нет лучше двадцать третье сентября"
        );
        TestRewriterUseCase(texts, {2, 4}, true, {2, 5}); // 23 сентября -> двадцать третье сентября
        TestRewriterUseCase(texts, {2, 3}, true, {2, 4}); // 23 -> двадцать третье
    }
}
