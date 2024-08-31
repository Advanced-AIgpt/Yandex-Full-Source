#include "scenarios_detectors.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/charset/wide.h>
#include <util/generic/yexception.h>
#include <util/string/vector.h>


namespace NAlice {

namespace {

template <class TCommandDetector>
void CheckUtteranceIsDetected(const TString& utterance, TCommandDetector detector) {
    TVector<TUtf16String> words = SplitString(UTF8ToWide(utterance), u" ");
    const auto errorMessage = TString{"Error processing \""} + utterance + "\"";
    UNIT_ASSERT_C(detector(TAsrHypothesis{words}), errorMessage);
}

template <class TCommandDetector>
void CheckUtteranceIsNotDetected(const TString& utterance, TCommandDetector detector) {
    TVector<TUtf16String> words = SplitString(UTF8ToWide(utterance), u" ");
    const auto errorMessage = TString{"Error processing \""} + utterance + "\"";
    UNIT_ASSERT_C(!detector(TAsrHypothesis{words}), errorMessage);
}

}  // namespace

Y_UNIT_TEST_SUITE(TestScenariosDetectors) {
    Y_UNIT_TEST(TestIoTCommandsDetector) {
        const TVector<TString> testingUtterances{
            "алиса включи свет",
            "запусти пылесос",
            "включи люстру яркость 100%",
            "сделай кофе на кофеварке",
            "включи свет",
            "выруби лампочку",
            "вруби лампу",
            "запусти кондиционер",
            "включи 5 канал на телевизоре",
            "включи чайник",
            "выключи розетку через 2 часа",
        };
        for (const auto& utterance : testingUtterances) {
            CheckUtteranceIsDetected(utterance, [&](const auto& hypo) -> bool { return IsIoTCommand(hypo); });
        }
    }

    Y_UNIT_TEST(TestIoTCommandsDetectorWithIoTUserInfo) {
        const TVector<TString> testingUtterances {
            "включи музыку",
            "время уборки",
            "пора убираться",
        };

        for (const auto& utterance : testingUtterances) {
            CheckUtteranceIsNotDetected(utterance, [&](const auto& hypo) -> bool { return IsIoTCommand(hypo); });
        }

        TVector<TString> userIoTScenarios{"включи музыку", "время уборки", "пора убираться"};
        for (const auto& utterance : testingUtterances) {
            CheckUtteranceIsDetected(utterance, [&](const auto& hypo) -> bool {
                return IsIoTCommand(hypo, userIoTScenarios);
            });
        }
    }

    Y_UNIT_TEST(TestShortCommandsDetector) {
        const TVector<TString> testingUtterances {
            "стоп",
            "хватит",
            "алиса останови",
            "алиса стоп",
            "отключись",
            "алиса выключи",
            "вниз",
            "алиса пауза",
            "продолжай",
            "стой",
            "громче",
            "алиса потише",
            "выключи музыку",
            "нет",
            "дальше",
            "следующий трек",
            "лайк",
            "заткнись",
            "алиса отмена",
        };
        for (const auto& utterance : testingUtterances) {
            CheckUtteranceIsDetected(utterance, IsShortCommand);
        }
    }

    Y_UNIT_TEST(TestArtihmeticCommandsDetector) {
        const TVector<TString> testingUtterances {
            "алиса сколько будет два плюс два",
            "сколько будет тридцать пять умножить на пятьсот",
            "триста три умножить на два",
            "алиса сколько будет три поделить на четыре",
            "пятьдесят умножить на сорок умножить на пятьдесят",
            "два умножить на два",
            "пять минус один"
        };
        for (const auto& utterance : testingUtterances) {
            CheckUtteranceIsDetected(utterance, IsArithmeticsCommand);
        }
    }

    Y_UNIT_TEST(TestItemSelectorCommandsDetector) {
        const TVector<TString> testingUtterances {
            "алиса поставь номер один",
            "включи три",
            "включи номер девятнадцать",
            "алиса первое видео",
            "два поставь",
            "алиса запусти видео номер семь",
            "включи шесть",
            "поставь девятое видео",
            "четыре",
            "пятое",
        };
        for (const auto& utterance : testingUtterances) {
            CheckUtteranceIsDetected(utterance, IsItemSelectorCommand);
        }
    }
}

}  // namespace NAlice
