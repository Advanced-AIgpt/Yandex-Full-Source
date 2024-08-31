#include <alice/hollywood/library/scenarios/weather/util/translations.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywood::NWeather {

Y_UNIT_TEST_SUITE(Translations) {
    Y_UNIT_TEST(Translate) {
        TTranslations trans{TRTLogger::NullLogger(), ELanguage::LANG_RUS};

        UNIT_ASSERT_VALUES_EQUAL(trans.Translate("light-rain"), "небольшой дождь");
        UNIT_ASSERT_VALUES_EQUAL(trans.Translate("mist"), "дымка");
        UNIT_ASSERT_VALUES_EQUAL(trans.Translate("pollution-substance-co"), "моноксид углерода");
        UNIT_ASSERT_VALUES_EQUAL(trans.Translate("pollution-substance-co"), "моноксид углерода");
        UNIT_ASSERT_VALUES_EQUAL(trans.Translate("pollution-affect-aqi-4-6-names-so2"), "Люди, страдающие астмой, могут ощущать стеснение в груди, дискомфорт при дыхании");

        UNIT_ASSERT_VALUES_EQUAL(trans.Translate("wroooong-key"), "wroooong-key");
    }
}

} // namespace NAlice::NHollywood::NWeather
