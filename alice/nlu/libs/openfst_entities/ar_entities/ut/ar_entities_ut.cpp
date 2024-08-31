#include <alice/nlu/libs/openfst_entities/ar_entities/ar_entities.h>
#include <alice/nlu/libs/openfst_entities/ar_entities/json_keys.h>

#include <alice/nlu/libs/normalization/normalize.h>

#include <alice/library/json/json.h>

#include <library/cpp/testing/common/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/string/split.h>


namespace NAlice {

    TString MODELS_BASE_PATH = "alice/nlu/data/ar/models/openfst_entities";

    template <typename TEntityType>
    struct TCase {
        TString Input;
        TEntityType ExpectedOutput;
    };

    struct TJsonEntityTestCase {
        TString Input;
        NJson::TJsonValue ExpectedEntity;
    };

    struct TSpanTestCase {
        TSpan ByteSpan;
        TSpan TokenSpan;
    };

    template <typename TEntityType>
    TVector<TEntityType> ConvertToEntityList(const TVector<NJson::TJsonValue>& jsonEntities)
    {
        TVector<TEntityType> entities;
        for (const NJson::TJsonValue& jsonEntity : jsonEntities) {
            entities.push_back(TEntityType(jsonEntity));
        }
        return entities;
    }

    void AssertEqualTEntityBlockBase(const TEntityBlockBase& lhsEntity, const TEntityBlockBase& rhsEntity,
                                     size_t caseNumber)
    {
        UNIT_ASSERT_EQUAL_C(lhsEntity.GetSpan().Begin, rhsEntity.GetSpan().Begin, caseNumber);
        UNIT_ASSERT_EQUAL_C(lhsEntity.GetSpan().End, rhsEntity.GetSpan().End, caseNumber);
        UNIT_ASSERT_EQUAL_C(lhsEntity.GetText(), rhsEntity.GetText(), caseNumber);
    }

    void TestJsonEntities(const TVector<TJsonEntityTestCase>& testCases, TEntityParser& entityParser) {
        for (int i = 0; i < testCases.size(); i++) {
            const TString input = NNlu::NormalizeText(testCases[i].Input, LANG_ARA);
            const TVector<NJson::TJsonValue> entities = entityParser.GetJsonEntities(input);
            UNIT_ASSERT_C(!entities.empty(), i);
            for (const auto& [key, value] : testCases[i].ExpectedEntity.GetMap()) {
                UNIT_ASSERT_EQUAL_C(entities[0][key], value, "test case: " << i << ", key:" << key << "\n" << entities[0][key] << "\n" << value);
            }
        }
    }

    Y_UNIT_TEST_SUITE(testHelperFunctions)
    {
        Y_UNIT_TEST(TestJsonFromFstResult)
        {
            const TVector<TCase<TString>> testCases {
                {"{\"a\": 1, \"b\": \"مرحبا\", \"arr\": [4, 23, 12, 546, 2,],   }",
                 "{\"a\":1,\"arr\":[4,23,12,546,2],\"b\":\"مرحبا\"}"},
                {"{\"a\": true, \"arr\": [\"asd\", \"qwe21w\", \"123145\",  ], \"fg\": \"مرحبا\"}",
                 "{\"a\":true,\"arr\":[\"asd\",\"qwe21w\",\"123145\"],\"fg\":\"مرحبا\"}"}};
            for (int i = 0; i < testCases.size(); i++) {
                NJson::TJsonValue output = NImpl::JsonFromFstResult(testCases[i].Input);
                UNIT_ASSERT_EQUAL_C(JsonToString(output, true), testCases[i].ExpectedOutput, i);
            }
        }

        Y_UNIT_TEST(TestTokenByteSpans) {
            TVector<TString> tokens = StringSplitter("a ab abc d").Split(' ');
            const TVector<TSpan> tokenByteSpans = NImpl::GetByteSpansForTokens(tokens);
            const TVector<TSpan> expectedTokenByteSpans = {
                {0, 1},
                {2, 4},
                {5, 8},
                {9, 10}
            };
            UNIT_ASSERT_EQUAL(tokenByteSpans.size(), expectedTokenByteSpans.size());
            for (size_t i = 0; i < tokenByteSpans.size(); ++i) {
                UNIT_ASSERT_EQUAL_C(tokenByteSpans[i], expectedTokenByteSpans[i], i);
            }
        }

        Y_UNIT_TEST(TestTokenIndex) {
            TVector<TString> tokens = StringSplitter("a ab abc d").Split(' ');
            const TVector<TSpan> tokenByteSpans = NImpl::GetByteSpansForTokens(tokens);
            const TVector<size_t> tokenIndices = {0, 0, 1, 1, 1, 2, 2, 2, 2, 3};
            for (int i = 0; i < tokenIndices.size(); ++i) {
                UNIT_ASSERT_EQUAL_C(NImpl::GetTokenIndex(tokenByteSpans, i), tokenIndices[i], i);
            }
        }

        Y_UNIT_TEST(TestTokenSpan) {
            TVector<TString> tokens = StringSplitter("a ab abc").Split(' ');
            const TVector<TSpan> tokenByteSpans = NImpl::GetByteSpansForTokens(tokens);
            const TVector<TSpanTestCase> cases = {
                {.ByteSpan{0, 1}, .TokenSpan{0, 1}},
                {.ByteSpan{0, 2}, .TokenSpan{0, 1}},
                {.ByteSpan{0, 3}, .TokenSpan{0, 2}}, 
                {.ByteSpan{0, 4}, .TokenSpan{0, 2}},
                {.ByteSpan{0, 5}, .TokenSpan{0, 2}},
                {.ByteSpan{0, 8}, .TokenSpan{0, 3}},
                {.ByteSpan{1, 3}, .TokenSpan{1, 2}},
                {.ByteSpan{1, 4}, .TokenSpan{1, 2}},
                {.ByteSpan{2, 3}, .TokenSpan{1, 2}},
                {.ByteSpan{2, 5}, .TokenSpan{1, 2}},
                {.ByteSpan{2, 8}, .TokenSpan{1, 3}},
                {.ByteSpan{3, 4}, .TokenSpan{1, 2}},
                {.ByteSpan{5, 7}, .TokenSpan{2, 3}},
                {.ByteSpan{6, 8}, .TokenSpan{2, 3}}
            };
            for (TSpanTestCase case_ : cases) {
                UNIT_ASSERT_EQUAL(NImpl::GetTokenSpan(tokenByteSpans, case_.ByteSpan), case_.TokenSpan);
            }
        }
    }

    Y_UNIT_TEST_SUITE(testNumberEntities)
    {
        const TString numberFstFarPath = BinaryPath(MODELS_BASE_PATH + "/full_number_fst.far");
        TNumberEntityParser numberEntityParser(numberFstFarPath, 10);

        Y_UNIT_TEST(TestNumberEntities)
        {
            const TVector<TCase<TNumberBlock>> testCases {
                {
                    // Alice set an alarm at 2 20/07/1222
                    "أليسا اضبطي منبه على الساعة الثانية من يوم عشرين من شهر تموز من عام الف ومئتين واثنين وعشرين",
                    TNumberBlock(
                        /* Span */ {.Begin = 121, .End = 167},
                        /* Text */ " الف ومئتين واثنين وعشرين",
                        /* Number */ 1222)
                },
                {
                    // Alice st an alarm at 2 every 2 years, 4 months and 25 days
                    "أليسا اضبطي منبه على الساعة الثانية كل سنتين واربع شهور وخمس وعشرين يوما",
                    TNumberBlock(
                        /* Span */ {.Begin = 101, .End = 123},
                        /* Text */ " وخمس وعشرين",
                        /* Number */ 25)
                },
                {
                    // Alice set an alarm after 305 hours and 25 minutes
                    "أليسا اضبطي منبه بعد ثلاثمئه وخمس ساعات وخمس وعشرون دقيقه",
                    TNumberBlock(
                        /* Span */ {.Begin = 37, .End = 61},
                        /* Text */ " ثلاثمئه وخمس",
                        /* Number */ 305)
                }
            };
            for (int i = 0; i < testCases.size(); i++) {
                const TString input = NNlu::NormalizeText(testCases[i].Input, LANG_ARA);
                const TVector<TNumberBlock> entities =
                    ConvertToEntityList<TNumberBlock>(numberEntityParser.GetJsonEntitiesWithRawByteSpans(input));
                UNIT_ASSERT_C(!entities.empty(), i);
                AssertEqualTEntityBlockBase(entities[0], testCases[i].ExpectedOutput, i);
                UNIT_ASSERT_EQUAL_C(entities[0].GetNumber(), testCases[i].ExpectedOutput.GetNumber(), i);
            }
        }

        Y_UNIT_TEST(TestNumberWithTokenSpans) {
            const TVector<TJsonEntityTestCase> testCases {
                {
                    // Alice set an alarm at 2 20/07/1222
                    "أليسا اضبطي منبه على الساعة الثانية من يوم عشرين من شهر تموز من عام الف ومئتين واثنين وعشرين",
                    NJson::TJsonMap({
                        {BEGIN.data(),  14},
                        {END.data(),    18},
                        {TEXT.data(),   "الف ومئتين واثنين وعشرين"},
                        {CONTENT.data(), 1222}
                    })
                },
                {
                    // Alice turn on washing machine after 15072 hours
                    "أليسا شغلي الغسالة بعد خمسة عشر ألف واثنان وسبعون ساعة",
                    NJson::TJsonMap({
                        {BEGIN.data(),  4},
                        {END.data(),    9},
                        {TEXT.data(),   "خمسه عشر الف واثنان وسبعون"},
                        {CONTENT.data(), 15072}
                    })
                }
            };
            TestJsonEntities(testCases, numberEntityParser);
        }
    }

    Y_UNIT_TEST_SUITE(testFloatEntities)
    {
        const TString floatFstFarPath = BinaryPath(MODELS_BASE_PATH + "/float_fst.far");
        TFloatEntityParser floatEntityParser(floatFstFarPath, 10);

        Y_UNIT_TEST(TestFloatEntities)
        {
            const TVector<TCase<TFloatBlock>> testCases {
                {
                    // Alice how much is 4.5 Dollars in Riyal
                    "أليسا كم يعادل اربعة وخمس بالعشرة دولار بالريال",
                    TFloatBlock(
                        /* Span */ {.Begin = 26, .End = 61},
                        /* Text */ " اربعه وخمس بالعشره",
                        /* FloatValue] */ 4.5)
                },
                {
                    // Alice how much is 87.029 Dollars in Riyal
                    "أليسا ما قيمة سبع وثمانين وتسعة وعشرين بالالف دولار بالريال",
                    TFloatBlock(
                        /* Span */ {.Begin = 24, .End = 83},
                        /* Text */ " سبع وثمانين وتسعه وعشرين بالالف",
                        /* FloatValue */ 87.029)
                },
                {
                    // Alice how much is 5.25 Dollars in Riyal
                    "أليسا كم يعادل خمسين وربع دولار بالريال",
                    TFloatBlock(
                        /* Span */ {.Begin = 26, .End = 46},
                        /* Text */ " خمسين وربع",
                        /* FloatValue */ 50.25)
                },
                {
                    // Alice how much is 140.6 Dollars in Riyal
                    "أليسا كم يعادل مئة واربعين وثلاث اخماس دولار بالريال",
                    TFloatBlock(
                        /* Span */ {.Begin = 26, .End = 70},
                        /* Text */ " مئه واربعين وثلاث اخماس",
                        /* FloatValue */ 140.6)
                }
            };
            for (int i = 0; i < testCases.size(); i++) {
                const TString input = NNlu::NormalizeText(testCases[i].Input, LANG_ARA);
                const TVector<TFloatBlock> entities =
                    ConvertToEntityList<TFloatBlock>(floatEntityParser.GetJsonEntitiesWithRawByteSpans(input));
                UNIT_ASSERT_C(!entities.empty(), i);
                AssertEqualTEntityBlockBase(entities[0], testCases[i].ExpectedOutput, i);
                UNIT_ASSERT_EQUAL_C(entities[0].GetFloatValue(), testCases[i].ExpectedOutput.GetFloatValue(), i);
            }
        }

        Y_UNIT_TEST(TestFloatWithTokenSpans) {
            const TVector<TJsonEntityTestCase> testCases {
                {
                    // Alice how much is 24.6 Dollars in Riyal
                    "أليسا كم يعادل اربعة وعشرين وسته بالعشرة دولار بالريال",
                    NJson::TJsonMap({
                        {BEGIN.data(),  3},
                        {END.data(),    7},
                        {TEXT.data(),   "اربعه وعشرين وسته بالعشره"},
                        {CONTENT.data(), 24.6}
                    })
                },
                {
                    // Alice how much is 87.079 Dollars in Riyal
                    "أليسا ما قيمة سبع وثمانين وتسعة وسبعين بالالف دولار بالريال",
                    NJson::TJsonMap({
                        {BEGIN.data(),  3},
                        {END.data(),    8},
                        {TEXT.data(),   "سبع وثمانين وتسعه وسبعين بالالف"},
                        {CONTENT.data(), 87.079}
                    })
                },
                {
                    // Alice how much is 87.00079 Dollars in Riyal
                    "أليسا ما قيمة سبع وثمانين وتسعة وسبعين بالمئة الالف دولار بالريال",
                    NJson::TJsonMap({
                        {BEGIN.data(),  3},
                        {END.data(),    9},
                        {TEXT.data(),   "سبع وثمانين وتسعه وسبعين بالمئه الالف"},
                        {CONTENT.data(), 87.00079}
                    })
                },
                {
                    // Alice how much is 1068.039 Dollars in Riyal
                    "أليسا ما قيمة الف وثمان وستين فاصلة تسعة وثلاثين بالالف دولار بالريال",
                    NJson::TJsonMap({
                        {BEGIN.data(),  3},
                        {END.data(),    10},
                        {TEXT.data(),   "الف وثمان وستين فاصله تسعه وثلاثين بالالف"},
                        {CONTENT.data(), 1068.039}
                    })
                },
                {
                    // Alice how much is 1068.39 Dollars in Riyal
                    "أليسا ما قيمة الف وثمان وستين فاصلة تسعة وثلاثين دولار بالريال",
                    NJson::TJsonMap({
                        {BEGIN.data(),  3},
                        {END.data(),    9},
                        {TEXT.data(),   "الف وثمان وستين فاصله تسعه وثلاثين"},
                        {CONTENT.data(), 1068.39}
                    })
                }
            };
            TestJsonEntities(testCases, floatEntityParser);
        }
    }

    Y_UNIT_TEST_SUITE(testTimeEntities)
    {
        const TString timeFstFarPath = BinaryPath(MODELS_BASE_PATH + "/time_fst.far");
        TTimeEntityParser timeEntityParser(timeFstFarPath, 10);

        Y_UNIT_TEST(TestTimeEntities)
        {
            const TVector<TCase<TTimeBlock>> testCases {
                {
                    // Alice set an alarm at 13:15 pm
                    "أليسا اضبطي المنبه على الساعه الثالثه عشره و ربع مساء",
                    TTimeBlock(
                        /* Span */ {.Begin = 54, .End = 97},
                        /* Text */ " الثالثه عشره و ربع مساء",
                        /* IsRelative */ false,
                        /* Hours */ 13,
                        /* Minutes */ 15,
                        /* Seconds */ 0,
                        /* Repeat */ false,
                        /* DayPart */ TTimeBlock::EDayPart::Pm)
                },
                {
                    // Alice set an alarm at 05:04 am
                    "أليسا اضبطي المنبه على الساعه الخامسه واربع دقائق صباحا",
                    TTimeBlock(
                        /* Span */ {.Begin = 54, .End = 102},
                        /* Text */ " الخامسه واربع دقائق صباحا",
                        /* IsRelative */ false,
                        /* Hours */ 5,
                        /* Minutes */ 4,
                        /* Seconds */ 0,
                        /* Repeat */ false,
                        /* DayPart */ TTimeBlock::EDayPart::Am)
                },
                {
                    // Alice set an alarm at 5:40 am
                    "أليسا اضبطي المنبه على الساعه السادسه الا عشرين دقيقه فجرا",
                    TTimeBlock(
                        /* Span */ {.Begin = 54, .End = 107},
                        /* Text */ " السادسه الا عشرين دقيقه فجرا",
                        /* IsRelative */ false,
                        /* Hours */ 5,
                        /* Minutes */ 40,
                        /* Seconds */ 0,
                        /* Repeat */ false,
                        /* DayPart */ TTimeBlock::EDayPart::Am)
                },
                {
                    // Alice set an alarm after 305 hours, 25minutes and 37 seconds from now
                    "أليسا اضبطي منبه بعد ثلاثمئه وخمس ساعات وخمس وعشرون دقيقه وسبع وثلاثون ثانيه من الان",
                    TTimeBlock(
                        /* Span */ {.Begin = 37, .End = 141},
                        /* Text */ " ثلاثمئه وخمس ساعات وخمس وعشرون دقيقه وسبع وثلاثون ثانيه ",
                        /* IsRelative */ true,
                        /* Hours */ 305,
                        /* Minutes */ 25,
                        /* Seconds */ 37,
                        /* Repeat */ false,
                        /* DayPart */ TTimeBlock::EDayPart::Empty)
                },
                {
                    // Alice set an alarm at 2:20 pm
                    "أليسا اضبطي المنبه على الساعه الثانيه والعشرين ظهرا",
                    TTimeBlock(
                        /* Span */ {.Begin = 54, .End = 95},
                        /* Text */ " الثانيه والعشرين ظهرا",
                        /* IsRelative */ false,
                        /* Hours */ 2,
                        /* Minutes */ 20,
                        /* Seconds */ 0,
                        /* Repeat */ false,
                        /* DayPart */ TTimeBlock::EDayPart::Pm)
                }
            };
            for (int i = 0; i < testCases.size(); i++) {
                const TString input = NNlu::NormalizeText(testCases[i].Input, LANG_ARA);
                const TVector<TTimeBlock> entities =
                    ConvertToEntityList<TTimeBlock>(timeEntityParser.GetJsonEntitiesWithRawByteSpans(input));
                UNIT_ASSERT_C(!entities.empty(), i);
                AssertEqualTEntityBlockBase(entities[0], testCases[i].ExpectedOutput, i);
                UNIT_ASSERT_EQUAL_C(entities[0].GetHours(), testCases[i].ExpectedOutput.GetHours(), i);
                UNIT_ASSERT_EQUAL_C(entities[0].GetMinutes(), testCases[i].ExpectedOutput.GetMinutes(), i);
                UNIT_ASSERT_EQUAL_C(entities[0].GetSeconds(), testCases[i].ExpectedOutput.GetSeconds(), i);
                UNIT_ASSERT_EQUAL_C(entities[0].GetIsRelative(), testCases[i].ExpectedOutput.GetIsRelative(), i);
                UNIT_ASSERT_EQUAL_C(entities[0].GetRepeat(), testCases[i].ExpectedOutput.GetRepeat(), i);
                UNIT_ASSERT_EQUAL_C(entities[0].GetDayPart(), testCases[i].ExpectedOutput.GetDayPart(), i);
            }
        }

        Y_UNIT_TEST(TestTimeEntitiesTokenSpan){
            const TVector<TCase<TTimeBlock>> testCases {
                {
                    // Alice set an alarm at 13:15 pm
                    "أليسا اضبطي المنبه على الساعه الثالثه عشره و ربع مساء",
                    TTimeBlock(
                        /* Span */ {.Begin = 5, .End = 10},
                        /* Text */ "الثالثه عشره و ربع مساء",
                        /* IsRelative */ false,
                        /* Hours */ 13,
                        /* Minutes */ 15,
                        /* Seconds */ 0,
                        /* Repeat */ false,
                        /* DayPart */ TTimeBlock::EDayPart::Pm)
                },
                {
                    // Alice set an alarm at 05:04 am
                    "أليسا اضبطي المنبه على الساعه الخامسه واربع دقائق صباحا",
                    TTimeBlock(
                        /* Span */ {.Begin = 5, .End = 9},
                        /* Text */ "الخامسه واربع دقائق صباحا",
                        /* IsRelative */ false,
                        /* Hours */ 5,
                        /* Minutes */ 4,
                        /* Seconds */ 0,
                        /* Repeat */ false,
                        /* DayPart */ TTimeBlock::EDayPart::Am)
                }
            };
            for (int i = 0; i < testCases.size(); i++) {
                const TString input = NNlu::NormalizeText(testCases[i].Input, LANG_ARA);
                const TVector<TTimeBlock> entities =
                    ConvertToEntityList<TTimeBlock>(timeEntityParser.GetJsonEntities(input));
                UNIT_ASSERT_C(!entities.empty(), i);
                AssertEqualTEntityBlockBase(entities[0], testCases[i].ExpectedOutput, i);
                UNIT_ASSERT_EQUAL_C(entities[0].GetHours(), testCases[i].ExpectedOutput.GetHours(), i);
                UNIT_ASSERT_EQUAL_C(entities[0].GetMinutes(), testCases[i].ExpectedOutput.GetMinutes(), i);
                UNIT_ASSERT_EQUAL_C(entities[0].GetSeconds(), testCases[i].ExpectedOutput.GetSeconds(), i);
                UNIT_ASSERT_EQUAL_C(entities[0].GetIsRelative(), testCases[i].ExpectedOutput.GetIsRelative(), i);
                UNIT_ASSERT_EQUAL_C(entities[0].GetRepeat(), testCases[i].ExpectedOutput.GetRepeat(), i);
                UNIT_ASSERT_EQUAL_C(entities[0].GetDayPart(), testCases[i].ExpectedOutput.GetDayPart(), i);
            }
        }

        Y_UNIT_TEST(TestTimeWithTokenSpans) {
            const TVector<TJsonEntityTestCase> testCases {
                {
                    // Alice set an alarm after 305 hours, 25 minutes and 37 seconds from now
                    "أليسا اضبطي منبه بعد ثلاثمئه وخمس ساعات وخمس وعشرون دقيقه وسبع وثلاثون ثانيه من الان",
                    NJson::TJsonMap({
                        {BEGIN.data(),              4},
                        {END.data(),                13},
                        {TEXT.data(),               "ثلاثمئه وخمس ساعات وخمس وعشرون دقيقه وسبع وثلاثون ثانيه"},
                        {
                            CONTENT.data(), NJson::TJsonMap({
                                {HOURS.data(),              305},
                                {MINUTES.data(),            25},
                                {SECONDS.data(),            37},
                                {HOURS_RELATIVE.data(),     true},
                                {MINUTES_RELATIVE.data(),   true},
                                {SECONDS_RELATIVE.data(),   true},
                            })
                        },
                        {IS_RELATIVE.data(),        true}
                    })
                },
                {
                    // Alice remove pause for timer with 1000000000 hours and  13000 minutes
                    "ازالة وضع الايقاف للموقت لمدة ألف ساعة وثلاثة عشر الف دقيقة",
                    NJson::TJsonMap({
                        {BEGIN.data(),              5},
                        {END.data(),                11},
                        {TEXT.data(),               "الف ساعه وثلاثه عشر الف دقيقه"},
                        {
                            CONTENT.data(), NJson::TJsonMap({
                                {HOURS.data(),              1000},
                                {MINUTES.data(),            13000},
                                {HOURS_RELATIVE.data(),     true},
                                {MINUTES_RELATIVE.data(),   true}
                            })
                        },
                        {IS_RELATIVE.data(),        true}
                    })
                },
                {
                    // Alice set an alarm at 02:20 pm
                    "أليسا اضبطي المنبه على الساعه الثانيه والعشرين ظهرا",
                    NJson::TJsonMap({
                        {BEGIN.data(),              5},
                        {END.data(),                8},
                        {TEXT.data(),               "الثانيه والعشرين ظهرا"},
                        {
                            CONTENT.data(), NJson::TJsonMap({
                                {HOURS.data(),              2},
                                {MINUTES.data(),            20},
                                {PERIOD.data(),             "pm"}
                            })
                        },
                        {IS_RELATIVE.data(),        false}
                    })
                },
                {
                    // Alice set a timer for 50 minutes
                    "اضبطي المؤقت لخمسون دقيقة",
                    NJson::TJsonMap({
                        {BEGIN.data(),              2},
                        {END.data(),                4},
                        {TEXT.data(),               "لخمسون دقيقه"},
                        {
                            CONTENT.data(), NJson::TJsonMap({
                                {MINUTES.data(),            50},
                            })
                        },
                        {IS_RELATIVE.data(),        false}
                    })
                },
                {
                    // Alice set a timer for 30 minutes
                    "اضبطي المؤقت لمدة تلاتون دقيقة",
                    NJson::TJsonMap({
                        {BEGIN.data(),              3},
                        {END.data(),                5},
                        {TEXT.data(),               "تلاتون دقيقه"},
                        {
                            CONTENT.data(), NJson::TJsonMap({
                                {MINUTES.data(),            30},
                                {MINUTES_RELATIVE.data(),            true}
                            })
                        },
                        {IS_RELATIVE.data(),        true}
                    })
                }
            };
            TestJsonEntities(testCases, timeEntityParser);
        }
    }

    Y_UNIT_TEST_SUITE(testDateEntities)
    {        
        const TString dateFstFarPath = BinaryPath(MODELS_BASE_PATH + "/date_fst.far");
        TDateEntityParser dateEntityParser(dateFstFarPath, 10);
        
        Y_UNIT_TEST(TestDateEntities)
        {
            const TVector<TCase<TDateBlock>> testCases {
                {
                    // Alice set an alarm at 2 on 20/07/1222
                    "أليسا اضبطي منبه على الساعة الثانية من يوم عشرين من شهر تموز من عام الف ومئتين واثنين وعشرين",
                    TDateBlock(
                        /* Span */ {.Begin = 77, .End = 167},
                        /* Text */ " عشرين من شهر تموز من عام الف ومئتين واثنين وعشرين",
                        /* IsRelative */ false,
                        /* Days */ 20,
                        /* Months */ 7,
                        /* Years */ 1222,
                        /* Repeat */ false)
                },
                {
                    // Alice set an alarm at 2 every 2 years, 4 months and 25 days
                    "أليسا اضبطي منبه على الساعة الثانية كل سنتين واربع شهور وخمس وعشرين يوما",
                    TDateBlock(
                        /* Span */ {.Begin = 65, .End = 132},
                        /* Text */ " كل سنتين واربع شهور وخمس وعشرين يوما",
                        /* IsRelative */ true,
                        /* Days */ 25,
                        /* Months */ 4,
                        /* Years */ 2,
                        /* Repeat */ true)
                },
                {
                    // Alice set an alarm at 2 on the day before yesterday
                    "أليسا اضبطي منبه على الساعة 2 قبل أمس",
                    TDateBlock(
                        /* Span */ {.Begin = 53, .End = 66},
                        /* Text */ "قبل امس",
                        /* IsRelative */ true,
                        /* Days */ -2,
                        /* Months */ 0,
                        /* Years */ 0,
                        /* Repeat */ false)
                }
            };
            for (int i = 0; i < testCases.size(); i++) {
                const TString input = NNlu::NormalizeText(testCases[i].Input, LANG_ARA);
                const TVector<TDateBlock> entities =
                    ConvertToEntityList<TDateBlock>(dateEntityParser.GetJsonEntitiesWithRawByteSpans(input));
                UNIT_ASSERT_C(!entities.empty(), i);
                AssertEqualTEntityBlockBase(entities[0], testCases[i].ExpectedOutput, i);
                UNIT_ASSERT_EQUAL_C(entities[0].GetDays(), testCases[i].ExpectedOutput.GetDays(), i);
                UNIT_ASSERT_EQUAL_C(entities[0].GetMonths(), testCases[i].ExpectedOutput.GetMonths(), i);
                UNIT_ASSERT_EQUAL_C(entities[0].GetYears(), testCases[i].ExpectedOutput.GetYears(), i);
                UNIT_ASSERT_EQUAL_C(entities[0].GetIsRelative(), testCases[i].ExpectedOutput.GetIsRelative(), i);
                UNIT_ASSERT_EQUAL_C(entities[0].GetRepeat(), testCases[i].ExpectedOutput.GetRepeat(), i);
            }
        }

        Y_UNIT_TEST(TestDateEntitiesTokenSpan) {
            const TVector<TCase<TDateBlock>> testCases {
                {
                    // Alice set an alarm at 2 on 20/07/1222
                    "أليسا اضبطي منبه على الساعة الثانية من يوم عشرين من شهر تموز من عام الف ومئتين واثنين وعشرين",
                    TDateBlock(
                        /* Span */ {.Begin = 8, .End = 18},
                        /* Text */ "عشرين من شهر تموز من عام الف ومئتين واثنين وعشرين",
                        /* IsRelative */ false,
                        /* Days */ 20,
                        /* Months */ 7,
                        /* Years */ 1222,
                        /* Repeat */ false)
                },
                {
                    // Alice set an alarm at 2 every 2 years, 4 months and 25 days
                    "أليسا اضبطي منبه على الساعة الثانية كل سنتين واربع شهور وخمس وعشرين يوما",
                    TDateBlock(
                        /* Span */ {.Begin = 6, .End = 13},
                        /* Text */ "كل سنتين واربع شهور وخمس وعشرين يوما",
                        /* IsRelative */ true,
                        /* Days */ 25,
                        /* Months */ 4,
                        /* Years */ 2,
                        /* Repeat */ true)
                },
                {
                    // Alice set an alarm at 2 on the day before yesterday
                    "أليسا اضبطي منبه على الساعة 2 قبل أمس",
                    TDateBlock(
                        /* Span */ {.Begin = 6, .End = 8},
                        /* Text */ "قبل امس",
                        /* IsRelative */ true,
                        /* Days */ -2,
                        /* Months */ 0,
                        /* Years */ 0,
                        /* Repeat */ false)
                }
            };
            for (int i = 0; i < testCases.size(); i++) {
                const TString input = NNlu::NormalizeText(testCases[i].Input, LANG_ARA);
                const TVector<TDateBlock> entities =
                    ConvertToEntityList<TDateBlock>(dateEntityParser.GetJsonEntities(input));
                UNIT_ASSERT_C(!entities.empty(), i);
                AssertEqualTEntityBlockBase(entities[0], testCases[i].ExpectedOutput, i);
                UNIT_ASSERT_EQUAL_C(entities[0].GetDays(), testCases[i].ExpectedOutput.GetDays(), i);
                UNIT_ASSERT_EQUAL_C(entities[0].GetMonths(), testCases[i].ExpectedOutput.GetMonths(), i);
                UNIT_ASSERT_EQUAL_C(entities[0].GetYears(), testCases[i].ExpectedOutput.GetYears(), i);
                UNIT_ASSERT_EQUAL_C(entities[0].GetIsRelative(), testCases[i].ExpectedOutput.GetIsRelative(), i);
                UNIT_ASSERT_EQUAL_C(entities[0].GetRepeat(), testCases[i].ExpectedOutput.GetRepeat(), i);
            }
        }

        Y_UNIT_TEST(TestDateWithTokenSpans) {
            const TVector<TJsonEntityTestCase> testCases {
                {
                    // Alice set an alarm at 2 every 2 years, 4 months and 25 days
                    "أليسا اضبطي منبه على الساعة الثانية كل سنتين واربع شهور وخمس وعشرين يوما",
                    NJson::TJsonMap({
                        {BEGIN.data(),              6},
                        {END.data(),                13},
                        {TEXT.data(),               "كل سنتين واربع شهور وخمس وعشرين يوما"},
                        {
                            CONTENT.data(), NJson::TJsonMap({
                                {DAYS.data(),               25},
                                {MONTHS.data(),             4},
                                {YEARS.data(),              2},
                                {DAYS_RELATIVE.data(),      true},
                                {MONTHS_RELATIVE.data(),    true},
                                {YEARS_RELATIVE.data(),     true},
                                {REPEAT.data(),             true}
                            })
                        },
                        {IS_RELATIVE.data(),        true}
                    })
                },
                {
                    // Alice how the weather will be like on Thursday
                    "أليسا كيف سيكون الطقس يوم الخميس",
                    NJson::TJsonMap({
                        {BEGIN.data(),              4},
                        {END.data(),                6},
                        {TEXT.data(),               "يوم الخميس"},
                        {
                            CONTENT.data(), NJson::TJsonMap({
                                {WEEK_DAY.data(),               3},
                            })
                        }
                    })
                }
            };
            TestJsonEntities(testCases, dateEntityParser);
        }
    }

    Y_UNIT_TEST_SUITE(testWeekDaysEntities)
    {
        const TString weekDaysFstFarPath = BinaryPath(MODELS_BASE_PATH + "/weekdays_fst.far");
        TWeekDaysEntityParser weekDaysentityParser(weekDaysFstFarPath, 10);

        Y_UNIT_TEST(TestWeekDaysEntities)
        {
            const TVector<TCase<TWeekDaysBlock>> testCases {
                {
                    // Alice set an alarm at 2 every Sunday and Wednesday
                    "أليسا اضبطي منبه على الساعة الثانية كل أحد وأربعاء",
                    TWeekDaysBlock(
                        /* Span */ {.Begin = 65, .End = 92},
                        /* Text */ " كل احد واربعاء",
                        /* Days */ TBitMap<7>(0b1000100),
                        /* Repeat */ true)
                },
                {
                    // Alice set an alarm at 2 from Sunday to Thursday except Tuesday
                    "أليسا اضبطي منبه على الساعة الثانية من الاحد الى الخميس مع تكرار ما عدا الثلاثاء",
                    TWeekDaysBlock(
                        /* Span */ {.Begin = 65, .End = 146},
                        /* Text */ " من الاحد الى الخميس مع تكرار ما عدا الثلاثاء",
                        /* Days */ TBitMap<7>(0b1001101),
                        /* Repeat */ true)
                },
                {
                    // Alice set an alarm at 2 on Sunday, Monday and Wednesday
                    "أليسا اضبطي منبه على الساعة الثانية من الاحد والاثنين والاربعاء",
                    TWeekDaysBlock(
                        /* Span */ {.Begin = 70, .End = 117},
                        /* Text */ " الاحد والاثنين والاربعاء",
                        /* Days */ TBitMap<7>(0b1000101),
                        /* Repeat */ false)
                }
            };
            for (int i = 0; i < testCases.size(); i++) {
                const TString input = NNlu::NormalizeText(testCases[i].Input, LANG_ARA);
                const TVector<TWeekDaysBlock> entities =
                    ConvertToEntityList<TWeekDaysBlock>(weekDaysentityParser.GetJsonEntitiesWithRawByteSpans(input));
                UNIT_ASSERT_C(!entities.empty(), i);
                AssertEqualTEntityBlockBase(entities[0], testCases[i].ExpectedOutput, i);
                UNIT_ASSERT_EQUAL_C(entities[0].GetDays(), testCases[i].ExpectedOutput.GetDays(), i);
                UNIT_ASSERT_EQUAL_C(entities[0].GetRepeat(), testCases[i].ExpectedOutput.GetRepeat(), i);
            }
        }

        Y_UNIT_TEST(TestWeekDaysWithTokenSpans) {
            const TVector<TJsonEntityTestCase> testCases {
                {
                    // Monday
                    "الإثنين",
                    NJson::TJsonMap({
                        {BEGIN.data(),      0},
                        {END.data(),        1},
                        {TEXT.data(),       "الاثنين"},
                        {
                            CONTENT.data(), NJson::TJsonMap({
                                {WEEK_DAYS_RESULT.data(),   NJson::TJsonArray({1})},
                                {REPEAT.data(),             false}
                            })
                        }
                    })
                },
                {
                    // Friday and Sunday
                    "الجمعة والأحد",
                    NJson::TJsonMap({
                        {BEGIN.data(),      0},
                        {END.data(),        2},
                        {TEXT.data(),       "الجمعه والاحد"},
                        {
                            CONTENT.data(), NJson::TJsonMap({
                                {WEEK_DAYS_RESULT.data(),   NJson::TJsonArray({5, 7})},
                                {REPEAT.data(),             false}
                            })
                        }
                    })
                },
                {
                    // Alice set an alarm at 7 am everyday except Thursday and Friday
                    "قم بوضع منبه على الساعة 7 صباح كل يوم ما عدا يومي الخميس والجمعة",
                    NJson::TJsonMap({
                        {BEGIN.data(),      7},
                        {END.data(),        14},
                        {TEXT.data(),       "كل يوم ما عدا يومي الخميس والجمعه"},
                        {
                            CONTENT.data(), NJson::TJsonMap({
                                {WEEK_DAYS_RESULT.data(),   NJson::TJsonArray({1, 2, 3, 6, 7})},
                                {REPEAT.data(),             true}
                            })
                        }
                    })
                }
            };
            TestJsonEntities(testCases, weekDaysentityParser);
        }
    }

    Y_UNIT_TEST_SUITE(testDatetimeEntities)
    {
        const TString datetimeFstFarPath = BinaryPath(MODELS_BASE_PATH + "/datetime_fst.far");
        TDatetimeEntityParser datetimeEntityParser(datetimeFstFarPath, 10);

        Y_UNIT_TEST(TestDatetimeWithTokenSpans) {
            const TVector<TJsonEntityTestCase> testCases {
                {
                    // Alice set an alarm at 2 on 20/05/1999
                    "أليسا اضبطي منبه على الساعة الثانية يوم عشرين أيار من عام ألف وتسعمائة وتسعة وتسعون",
                    NJson::TJsonMap({
                        {BEGIN.data(),              5},
                        {END.data(),                15},
                        {TEXT.data(),               "الثانيه يوم عشرين ايار من عام الف وتسعمائه وتسعه وتسعون"},
                        {
                            CONTENT.data(), NJson::TJsonMap({
                                {DAYS.data(),               20},
                                {MONTHS.data(),             5},
                                {YEARS.data(),              1999},
                                {HOURS.data(),              2},
                            })
                        }
                    })
                },
                {
                    // Alice set an alarm at 04:20 on 10/12/2022
                    "أليسا اضبطي منبه على الساعة الرابعة والثلث في العاشر من كانون الأول من عام ألفين واثنين وعشرين",
                    NJson::TJsonMap({
                        {BEGIN.data(),              5},
                        {END.data(),                17},
                        {TEXT.data(),               "الرابعه والثلث في العاشر من كانون الاول من عام الفين واثنين وعشرين"},
                        {
                            CONTENT.data(), NJson::TJsonMap({
                                {DAYS.data(),               10},
                                {MONTHS.data(),             12},
                                {YEARS.data(),              2022},
                                {HOURS.data(),              4},
                                {MINUTES.data(),              20}
                            })
                        }
                    })
                },
                {
                    // Alice set an alarm on 10/12/2022
                    "أليسا اضبطي منبه في العاشر من كانون الأول من عام ألفين واثنين وعشرين",
                    NJson::TJsonMap({
                        {BEGIN.data(),              4},
                        {END.data(),                13},
                        {TEXT.data(),               "العاشر من كانون الاول من عام الفين واثنين وعشرين"},
                        {
                            CONTENT.data(), NJson::TJsonMap({
                                {DAYS.data(),               10},
                                {MONTHS.data(),             12},
                                {YEARS.data(),              2022}
                            })
                        }
                    })
                },
                {
                    // Alice set an alarm at 04:20
                    "أليسا اضبطي منبه على الساعة الرابعة والثلث",
                    NJson::TJsonMap({
                        {BEGIN.data(),              5},
                        {END.data(),                7},
                        {TEXT.data(),               "الرابعه والثلث"},
                        {
                            CONTENT.data(), NJson::TJsonMap({
                                {HOURS.data(),              4},
                                {MINUTES.data(),              20}
                            })
                        }
                    })
                }
            };
            TestJsonEntities(testCases, datetimeEntityParser);
        }
    }

    Y_UNIT_TEST_SUITE(testDatetimeRangeEntities)
    {
        const TString datetimeRangeFstFarPath = BinaryPath(MODELS_BASE_PATH + "/datetime_range_fst.far");
        TDatetimeRangeEntityParser datetimeRangeEntityParser(datetimeRangeFstFarPath, 10);

        Y_UNIT_TEST(TestDatetimeRangeWithTokenSpans) {
            const TVector<TJsonEntityTestCase> testCases{
                {
                    // Alice turn on light from 2 on 20/05/1999 to 04:20 on 15/06/1999
                    "أليسا شغلي الضوء من الساعة الثانية يوم عشرين أيار من عام ألف وتسعمائة وتسعة وتسعين حتى الساعة "
                    "الرابعة وثلث يوم الخامس عشر من حزيران عام الف وتسعمائة وتسع وتسعين",
                    NJson::TJsonMap({
                        {BEGIN.data(),              3},
                        {END.data(),                29},
                        {
                            TEXT.data(),
                            "من الساعه الثانيه يوم عشرين ايار من عام الف وتسعمائه وتسعه وتسعين حتى الساعه الرابعه وثلث"
                            " يوم الخامس عشر من حزيران عام الف وتسعمائه وتسع وتسعين"
                        },
                        {
                            CONTENT.data(), NJson::TJsonMap({
                                {
                                    DATETIME_RANGE_START.data(), NJson::TJsonMap({
                                        {DAYS.data(),               20},
                                        {MONTHS.data(),             5},
                                        {YEARS.data(),              1999},
                                        {HOURS.data(),              2}
                                    })
                                },
                                {
                                    DATETIME_RANGE_END.data(), NJson::TJsonMap({
                                        {DAYS.data(),               15},
                                        {MONTHS.data(),             6},
                                        {YEARS.data(),              1999},
                                        {HOURS.data(),              4},
                                        {MINUTES.data(),              20}
                                    })
                                }
                            })
                        }
                    })
                },
                {
                    // Alice what is the weather like this week
                    "أليسا كيف سيكون الطقس هذا الأسبوع",
                    NJson::TJsonMap({
                        {BEGIN.data(),              4},
                        {END.data(),                6},
                        {
                            TEXT.data(),
                            "هذا الاسبوع"
                        },
                        {
                            CONTENT.data(), NJson::TJsonMap({
                                {
                                    DATETIME_RANGE_START.data(), NJson::TJsonMap({
                                        {WEEKS.data(),               0},
                                        {WEEKS_RELATIVE.data(),             true}
                                    })
                                },
                                {
                                    DATETIME_RANGE_END.data(), NJson::TJsonMap({
                                        {WEEKS.data(),               1},
                                        {WEEKS_RELATIVE.data(),             true}
                                    })
                                }
                            })
                        }
                    })
                },
                {
                    // Alice what is the weather like at the weekend
                    "أليسا كيف سيكون الطقس في عطلة نهاية الأسبوع",
                    NJson::TJsonMap({
                        {BEGIN.data(),              5},
                        {END.data(),                8},
                        {
                            TEXT.data(),
                            "عطله نهايه الاسبوع"
                        },
                        {
                            CONTENT.data(), NJson::TJsonMap({
                                {
                                    DATETIME_RANGE_START.data(), NJson::TJsonMap({
                                        {WEEKS.data(),               0},
                                        {WEEKS_RELATIVE.data(),             true},
                                        {WEEKEND.data(),             true}
                                    })
                                },
                                {
                                    DATETIME_RANGE_END.data(), NJson::TJsonMap({
                                        {WEEKS.data(),               1},
                                        {WEEKS_RELATIVE.data(),             true},
                                        {WEEKEND.data(),             true}
                                    })
                                }
                            })
                        }
                    })
                },
                {
                    // Alice what is the weather in the next month
                    "أليسا كيف سيكون الطقس الشهر القادم في السعودية",
                    NJson::TJsonMap({
                        {BEGIN.data(),              4},
                        {END.data(),                6},
                        {
                            TEXT.data(),
                            "الشهر القادم"
                        },
                        {
                            CONTENT.data(), NJson::TJsonMap({
                                {
                                    DATETIME_RANGE_START.data(), NJson::TJsonMap({
                                        {MONTHS.data(),               1},
                                        {MONTHS_RELATIVE.data(),             true}
                                    })
                                },
                                {
                                    DATETIME_RANGE_END.data(), NJson::TJsonMap({
                                        {MONTHS.data(),               2},
                                        {MONTHS_RELATIVE.data(),             true}
                                    })
                                }
                            })
                        }
                    })
                },
                {
                    // Alice what was the weather like last year
                    "أليسا كيف كان الطقس السنة الماضية في دمشق",
                    NJson::TJsonMap({
                        {BEGIN.data(),              4},
                        {END.data(),                6},
                        {
                            TEXT.data(),
                            "السنه الماضيه"
                        },
                        {
                            CONTENT.data(), NJson::TJsonMap({
                                {
                                    DATETIME_RANGE_START.data(), NJson::TJsonMap({
                                        {YEARS.data(),               -1},
                                        {YEARS_RELATIVE.data(),             true}
                                    })
                                },
                                {
                                    DATETIME_RANGE_END.data(), NJson::TJsonMap({
                                        {YEARS.data(),               0},
                                        {YEARS_RELATIVE.data(),             true}
                                    })
                                }
                            })
                        }
                    })
                },
                {
                    // Alice how the weather will be like from Tuesday to Thursday in Damascus
                    "أليسا كيف سيكون الطقس من الثلاثاء إلى الخميس في دمشق",
                    NJson::TJsonMap({
                        {BEGIN.data(),              4},
                        {END.data(),                8},
                        {
                            TEXT.data(),
                            "من الثلاثاء الى الخميس"
                        },
                        {
                            CONTENT.data(), NJson::TJsonMap({
                                {
                                    DATETIME_RANGE_START.data(), NJson::TJsonMap({
                                        {WEEK_DAY.data(),               1}
                                    })
                                },
                                {
                                    DATETIME_RANGE_END.data(), NJson::TJsonMap({
                                        {WEEK_DAY.data(),               3}
                                    })
                                }
                            })
                        }
                    })
                }
            };
            TestJsonEntities(testCases, datetimeRangeEntityParser);
        }
    }

} // namespace NAlice
