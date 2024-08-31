#include "matcher.h"
#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywood::NFood {

Y_UNIT_TEST_SUITE(MenuMatcher) {

    struct TExpected {
        TVector<TNluCartItem> NluCart;
        TVector<TMatcherCartItem> MatcherCart;
        TVector<TString> UnknownItems;
    };

    void TestMenuMatcher(const THashMap<TString, TString>& frame, const TExpected& expected,
        const TMenuMatcher& matcher, const NJson::TJsonValue& menu)
    {
        const TVector<TNluCartItem> nluCart = ReadNluCartItemsFromSlots(frame);
        UNIT_ASSERT_EQUAL(nluCart, expected.NluCart);

        TVector<TMatcherCartItem> matcherCart;
        TVector<TString> unknownItems;
        matcher.Convert(nluCart, menu, &matcherCart, &unknownItems);
        UNIT_ASSERT_EQUAL(matcherCart, expected.MatcherCart);
        UNIT_ASSERT_EQUAL(unknownItems, expected.UnknownItems);
    }

    void TestMenuMatcher(const THashMap<TString, TString>& frame, const TExpected& expected) {
        const NJson::TJsonValue menu = ReadHardcodedMenuSample();
        TMenuMatcher matcher;
        TestMenuMatcher(frame, expected, matcher, menu);
        matcher.AddMenuToNormalizationCache(menu);
        TestMenuMatcher(frame, expected, matcher, menu);
    }

    Y_UNIT_TEST(TMenuMatcher1) {
        // "закажи в макдональдсе две больших картошки и биг мак"
        TestMenuMatcher(
            {
                {"item_count1", "2"},
                {"item_text1", "больших картошки"},
                {"item_name1", "french_fries.big"},
                {"item_text2", "биг мак"},
                {"item_name2", "big_mac"},
            },
            TExpected{
                .NluCart = {
                    {"больших картошки", "french_fries.big", true, 2},
                    {"биг мак", "big_mac", false, 1}
                },
                .MatcherCart = {
                    {
                        .IsAvailable = true,
                        .Id = 12904476,
                        .Name = "Картофель Фри",
                        .Price = 64,
                        .Quantity = 2,
                        .Description = "Вкусные, обжаренные в растительном фритюре и слегка посоленные палочки картофеля.",
                        .Weight = "110 г",
                        .Options = {
                            {
                                .GroupId = 3286596,
                                .OptionId = 34536396,
                                .Name = "Большой",
                                .Price = 13,
                                .IsExplicit = true,
                            },
                            {
                                .GroupId = 3286599,
                                .OptionId = 34536426,
                                .Name = "Без Соуса",
                                .Price = 0,
                                .IsExplicit = false,
                            }
                        },
                        .OptionsNlg = {"Большой"}
                    },
                    {
                        .IsAvailable = true,
                        .Id = 12904671,
                        .Name = "Биг Мак",
                        .Price = 135,
                        .Quantity = 1,
                        .Description = "Два бифштекса из 100% говядины на специальной булочке «Биг Мак», заправленной луком, двумя кусочками маринованных огурчиков, ломтиком сыра «Чеддер», салатом, и специальным соусом «Биг Мак».",
                        .Weight = "210 г",
                        .Options = {},
                        .OptionsNlg = {}
                    },
                },
                .UnknownItems = {},
            });
    }

    Y_UNIT_TEST(TMenuMatcher2) {
        // "закажи в макдональдсе биг тейсти маккомбо с кока колой зеро и быстрей вези это сюда"
        TestMenuMatcher(
            {
                {"item_text1", "биг тейсти маккомбо"},
                {"item_name1", "big_tasty.combo"},
                {"item_text2", "кока колой зеро"},
                {"item_name2", "coca_cola_zero"},
                {"item_text3", "быстрей вези это сюда"},
            },
            TExpected{
                .NluCart = {
                    {"биг тейсти маккомбо", "big_tasty.combo", false, 1},
                    {"кока колой зеро", "coca_cola_zero", false, 1},
                    {"быстрей вези это сюда", "", false, 1}
                },
                .MatcherCart = {
                    {
                        .IsAvailable = true,
                        .Id = 17804267,
                        .Name = "Биг Тейсти МакКомбо Большой",
                        .Price = 349,
                        .Quantity = 1,
                        .Description = "МакКомбо Большой состоит из Биг Тейсти, Картофель фри (бол.), напиток на выбор: прохладительный напиток (0.5л) или сок (0.6л).",
                        .Weight = "1 г",
                        .Options = {
                            {
                                .GroupId = 5721590,
                                .OptionId = 58027181,
                                .Name = "Кока-Кола Зеро",
                                .Price = 0,
                                .IsExplicit = true,
                            },
                            {
                                .GroupId = 5721587,
                                .OptionId = 58343192,
                                .Name = "Картофель фри",
                                .Price = 0,
                                .IsExplicit = false,
                            }
                        },
                        .OptionsNlg = {"Кока-Кола Зеро"}
                    }
                },
                .UnknownItems = {
                    "быстрей вези это сюда"
                },
            });
    }

    Y_UNIT_TEST(TMenuMatcher3) {
        // "закажи в макдональдсе фреш макмаффин"
        TestMenuMatcher(
            {
                {"item_text1", "фреш макмаффин"}
            },
            TExpected{
                .NluCart = {
                    {"фреш макмаффин", "", false, 1}
                },
                .MatcherCart = {
                    {
                        .IsAvailable = false,
                        .Id = 12904188,
                        .Name = "Фреш МакМаффин",
                        .Price = 119,
                        .Quantity = 1,
                        .Description = "Булочка, котлета из свинины, сыр Чеддер, салат Айсберг, помидор, соус Мак Чикен",
                        .Weight = "150 г",
                        .Options = {},
                        .OptionsNlg = {}
                    }
                },
                .UnknownItems = {},
            });
    }

    Y_UNIT_TEST(Unavailable) {
        // "закажи в макдональдсе макмаффин"
        TestMenuMatcher(
            {
                {"item_text1", "макмаффин"},
                {"item_name1", "mcmuffin"},
            },
            TExpected{
                .NluCart = {
                    {"макмаффин", "mcmuffin", false, 1}
                },
                .MatcherCart = {
                    {
                        .IsAvailable = false,
                        .Id = 12904191,
                        .Name = "МакМаффин с яйцом и свиной котлетой",
                        .Price = 115,
                        .Quantity = 1,
                        .Description = "Булочка с яйцом, свиная котлета, сыр Чеддер",
                        .Weight = "161 г",
                        .Options = {},
                        .OptionsNlg = {}
                    },
                },
                .UnknownItems = {},
            });
    }

    Y_UNIT_TEST(Shrimps) {
        // "закажи в макдональдсе креветки"
        TestMenuMatcher(
            {
                {"item_text1", "креветки"},
                {"item_name1", "shrimps"},
            },
            TExpected{
                .NluCart = {
                    {
                        .SpokenName = "креветки",
                        .NameId = "shrimps",
                        .IsQuantityDefinedByUser = false,
                        .Quantity = 1,
                    }
                },
                .MatcherCart = {
                    {
                        .IsAvailable = true,
                        .Id = 12904716,
                        .Name = "Большие Креветки (6 шт.)",
                        .Price = 244,
                        .Quantity = 1,
                        .Description = "Жареные креветки в хрустящей панировке. Еще аппетитнее с соусом 1000 островов. Легко. Изыскано. Вкусно",
                        .Weight = "108 г",
                        .Options = {
                            {
                                .GroupId = 3286839,
                                .OptionId = 34537380,
                                .Name = "Без Соуса",
                                .Price = 0,
                                .IsExplicit = false,
                            },
                        },
                        .OptionsNlg = {}
                    },
                },
                .UnknownItems = {},
            });
    }
}

}  // namespace NAlice::NHollywood::NFood
