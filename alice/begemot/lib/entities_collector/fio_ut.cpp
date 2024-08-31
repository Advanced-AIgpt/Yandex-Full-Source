#include "fio.h"
#include <alice/library/proto/proto.h>
#include <library/cpp/scheme/scheme.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NBg;
using namespace NBg::NAliceEntityCollector;

Y_UNIT_TEST_SUITE(Fio) {

    NSc::TValue CollectValues(const TVector<NGranet::TEntity>& entities) {
        NSc::TValue values;
        values.SetArray();
        for (const NGranet::TEntity& entity : entities) {
            values.Push(NSc::TValue::FromJson(entity.Value));
        }
        return values;
    }

    void TestFioValues(TStringBuf markupStr, TStringBuf expectedStr) {
        const NSc::TValue expected = NSc::TValue::FromJson(expectedStr);
        const auto markup = NAlice::ParseProtoText<NProto::TExternalMarkupProto>(markupStr);
        const NSc::TValue actual = CollectValues(CollectPASkillsFio(markup));
        UNIT_ASSERT_VALUES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(Entity) {
        const auto markup = NAlice::ParseProtoText<NProto::TExternalMarkupProto>(R"(
            Tokens {Text: "зови"}
            Tokens {Text: "меня"}
            Tokens {Text: "ваней"}
            Morph {
                Tokens {Begin: 0, End: 1}
                Lemmas {
                    Text: "звать"
                    Language: "ru"
                    Grammems: "V sg imper 2p ipf tran"
                }
            }
            Morph {
                Tokens {Begin: 1, End: 2}
                Lemmas {
                    Text: "я"
                    Language: "ru"
                    Grammems: "SPRO acc sg 1p"
                    Grammems: "SPRO gen sg 1p"
                }
            }
            Morph {
                Tokens {Begin: 2, End: 3}
                Lemmas {
                    Text: "ваня"
                    Language: "ru"
                    Grammems: "S persn ins sg m anim"
                }
            }
        )");
        const TVector<NGranet::TEntity> expected = {
            {
                .Interval = {2, 3},
                .Type = "YANDEX.FIO",
                .Value = R"({"first_name":"ваня"})",
                .LogProbability = -5,
            }
        };
        const TVector<NGranet::TEntity> actual = CollectPASkillsFio(markup);
        UNIT_ASSERT_VALUES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(Full) {
        const auto markup = NAlice::ParseProtoText<NProto::TExternalMarkupProto>(R"(
            Tokens {Text: "имя"}
            Tokens {Text: "иванов"}
            Tokens {Text: "иван"}
            Tokens {Text: "иванович"}
            Fio {
                Tokens {Begin: 1, End: 4}
                Type: "fioname"
                FirstName: "иван"
                LastName: "иванов"
                Patronymic: "иванович"
            }
            Morph {
                Tokens {Begin: 0, End: 1}
                Lemmas {
                    Text: "имя"
                    Language: "ru"
                    Grammems: "S acc sg n inan"
                    Grammems: "S nom sg n inan"
                }
            }
            Morph {
                Tokens {Begin: 1, End: 2}
                Lemmas {
                    Text: "иванов"
                    Language: "ru"
                    Grammems: "S famn nom sg m anim"
                }
                Lemmas {
                    Text: "иван"
                    Language: "ru"
                    Grammems: "S persn acc pl m anim"
                    Grammems: "S persn gen pl m anim"
                }
                Lemmas {
                    Text: "иванов"
                    Language: "ru"
                    Grammems: "A acc sg plen poss m inan"
                    Grammems: "A nom sg plen poss m"
                }
            }
            Morph {
                Tokens {Begin: 2, End: 3}
                Lemmas {
                    Text: "иван"
                    Language: "ru"
                    Grammems: "S persn nom sg m anim"
                }
            }
            Morph {
                Tokens {Begin: 3, End: 4}
                Lemmas {
                    Text: "иванович"
                    Language: "ru"
                    Grammems: "S patrn nom sg m anim"
                }
            }
        )");
        const TVector<NGranet::TEntity> expected = {
            {
                .Interval = {1, 4},
                .Type = "YANDEX.FIO",
                .Value = R"({"first_name":"иван","last_name":"иванов","patronymic_name":"иванович"})",
                .LogProbability = -5,
            }
        };
        const TVector<NGranet::TEntity> actual = CollectPASkillsFio(markup);
        UNIT_ASSERT_VALUES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(Compound) {
        const TStringBuf markup = R"(
            Tokens {Text: "анна"}
            Tokens {Text: "вероника"}
            Tokens {Text: "дорогуш"}
            Morph {
                Tokens {Begin: 0, End: 1}
                Lemmas {
                    Text: "анна"
                    Language: "ru"
                    Grammems: "S persn nom sg f anim"
                }
            }
            Morph {
                Tokens {Begin: 1, End: 2}
                Lemmas {
                    Text: "вероника"
                    Language: "ru"
                    Grammems: "S persn nom sg f anim"
                }
                Lemmas {
                    Text: "вероника"
                    Language: "ru"
                    Grammems: "S nom sg f inan"
                }
            }
            Morph {
                Tokens {Begin: 2, End: 3}
                Lemmas {
                    Text: "дорогуш"
                    Language: "ru"
                    Grammems: "S famn nom sg m anim"
                }
                Lemmas {
                    Text: "дорогуша"
                    Language: "ru"
                    Grammems: "S acc pl mf anim"
                    Grammems: "S gen pl mf anim"
                }
                Lemmas {
                    Text: "дорогуш"
                    Language: "ru"
                    Grammems: "S famn abl pl f anim"
                    Grammems: "S famn abl sg f anim"
                    Grammems: "S famn acc pl f anim"
                    Grammems: "S famn acc sg f anim"
                    Grammems: "S famn dat pl f anim"
                    Grammems: "S famn dat sg f anim"
                    Grammems: "S famn gen pl f anim"
                    Grammems: "S famn gen sg f anim"
                    Grammems: "S famn ins pl f anim"
                    Grammems: "S famn ins sg f anim"
                    Grammems: "S famn nom pl f anim"
                    Grammems: "S famn nom sg f anim"
                }
            }
        )";
        const TStringBuf expected = R"([
            {
                "first_name": "анна"
            },
            {
                "first_name": "вероника",
                "last_name": "дорогуш"
            }
        ])";
        TestFioValues(markup, expected);
    }

    Y_UNIT_TEST(ToIvan) {
        const TStringBuf markup = R"(
            Tokens {Text: "позвонить"}
            Tokens {Text: "ивану"}
            Tokens {Text: "громову"}
            Fio {
                Tokens {Begin: 1, End: 3}
                Type: "finame"
                FirstName: "иван"
                LastName: "громов"
            }
            Morph {
                Tokens {Begin: 0, End: 1}
                Lemmas {
                    Text: "позвонить"
                    Language: "ru"
                    Grammems: "V inf pf intr"
                }
            }
            Morph {
                Tokens {Begin: 1, End: 2}
                Lemmas {
                    Text: "иван"
                    Language: "ru"
                    Grammems: "S persn dat sg m anim"
                }
            }
            Morph {
                Tokens {Begin: 2, End: 3}
                Lemmas {
                    Text: "громов"
                    Language: "ru"
                    Grammems: "S famn dat sg m anim"
                }
                Lemmas {
                    Text: "громова"
                    Language: "ru"
                    Grammems: "S famn acc sg f anim"
                }
            }
        )";
        const TStringBuf expected = R"([
            {
                "first_name": "иван",
                "last_name": "громов"
            }
        ])";
        TestFioValues(markup, expected);
    }

    Y_UNIT_TEST(CallIvanov) {
        const TStringBuf markup = R"(
            Tokens {Text: "набери"}
            Tokens {Text: "иванова"}
            Morph {
                Tokens {Begin: 0, End: 1}
                Lemmas {
                    Text: "набирать"
                    Language: "ru"
                    Grammems: "V sg imper 2p pf"
                }
            }
            Morph {
                Tokens {Begin: 1, End: 2}
                Lemmas {
                    Text: "иванов"
                    Language: "ru"
                    Grammems: "S famn acc sg m anim"
                    Grammems: "S famn gen sg m anim"
                }
                Lemmas {
                    Text: "иванов"
                    Language: "ru"
                    Grammems: "A nom sg plen poss f"
                    Grammems: "A acc sg plen poss m anim"
                    Grammems: "A gen sg plen poss m"
                    Grammems: "A gen sg plen poss n"
                }
                Lemmas {
                    Text: "иванова"
                    Language: "ru"
                    Grammems: "S famn nom sg f anim"
                }
                Lemmas {
                    Text: "иваново"
                    Language: "ru"
                    Grammems: "S geo gen sg n inan"
                }
            }
        )";
        const TStringBuf expected = R"([
            {
                "last_name": "иванов"
            }
        ])";
        TestFioValues(markup, expected);
    }
}
