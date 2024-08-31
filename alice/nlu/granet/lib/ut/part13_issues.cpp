#include <alice/nlu/granet/lib/ut/granet_tester.h>
#include <alice/nlu/libs/ut_utils/ut_utils.h>

using namespace NGranet;

Y_UNIT_TEST_SUITE(GranetPart13_Issues) {

    Y_UNIT_TEST(FairyTaleIssue) {
        TGranetTester tester(R"(
            form f:
                root:
                    [найди+ $FairyTaleGroup+ $AnyFairyTaleName]
                    [$FairyTaleGroup+ $AnyFairyTaleName*]
            filler:
                мне
                ты
                что я могу самое
                мне я знаю что ты можешь
                ты можешь
            $FairyTaleGroup:
                [любую* $Fairytale]
            $AnyFairyTaleName:
                $About зайца
                .+
            $Fairytale:
                сказку
            $About:
                про
        )");
        tester.TestHasMatch("что я могу самое найди мне я знаю что ты можешь сказку про зайца", true);
    }

    Y_UNIT_TEST(DislikeIssue1) {
        TGranetTester tester(R"(
            form f:
                root:   [$DontLike $Object]
            filler:     его
            $DontLike:  нафиг
            $It:        %lemma|его
            $Object:    $It
        )");
        tester.TestHasMatch("его нафиг", true);
    }

    Y_UNIT_TEST(DislikeIssue2) {
        TGranetTester tester(R"(
            form f:
                root:
                    [нейтральное* $Positive+]
                    %negative
                    $Negative+
            $Positive:  [это? $PosBad]
            $PosBad:    $Bad
            $Negative:  [ты $NegBad+]
            $NegBad:    $Bad* бред | $Bad
            $Bad:       тупая
            filler:     тупая
        )");
        tester.TestHasMatch(TString("тупая ") * 40, true);
    }

    Y_UNIT_TEST(Podcast) {
        TGranetTester tester(R"(
            form f:
                root:
                    [($TurnOn* $ForMe?) подкаст $UnknownPodcast*]
            $ForMe:
                мне
            $TurnOn:
                включи
            $UnknownPodcast:
                .+
        )");
        tester.TestHasMatch("подкаст атеизм", true);
    }

    Y_UNIT_TEST(ThereminEntity) {
        TGranetTester tester(R"(
            entity custom.theremin.beat_enum:
                root:
                    %lemma
                    %value "nylon_guitar"
                    [гитара нейлон]
                    нейлон

                    %value "guitar"
                    гитара
            filler:
                алиса
                яндекс
                и
                мне
                нам
                на
                инструмент
                инструмента
                инструментов
        )");
        tester.TestEntityFinder({
            "дай звук 'гитары на нейлоне'(custom.theremin.beat_enum:nylon_guitar)",
        });
    }

    Y_UNIT_TEST(FillerOutsideSlot) {
        TGranetTester tester(R"(
            form music_fairytale:
                slots:
                    fairy_tale:
                        type: string
                        source: $FairyTaleName
            root:                       [$Tell+ $FairyTaleGroup+ $AnyFairyTaleName]
            filler:                     $Common.Filler; $nonsense; а; мне; нам; %lemma; эту; онлайн; сборник; $Intro; $NotFairyTaleName
            $Common.Filler:             плиз; %weight 0.03; пожалуйста
            $Intro:                     привет
            $NotFairyTaleName:          сначала; сама
            $FairyTaleGroup:            [$Attribute* $Fairytale]
            $Attribute:                 интересная
            $Fairytale:                 %lemma; сказка; сказочка; аудиосказка
            $Tell:                      запусти; %weight 0.03; прочитай
            $About:                     про; о; об; о том; о том как; про то; про то как
            $FairyTaleName:
                $Author? $About? ($PopularFairyTale | $custom.fairy_tale | $StarostinFairyTaleName) $Author?
                $About $StarostinTopic
                $Author
            $AnyFairyTaleName:
                $FairyTaleName
                $About? $Anything
            $Anything:                  .+
            $PopularFairyTale:          %lemma; золушка
            $Author:                    %lemma; чуковский
            $StarostinFairyTaleName:    %lemma; Снегурочка; %weight 0.001; Колобок
            $StarostinTopic:            %lemma; алёнушка; %weight 0.0001; колобок
        )");
        tester.AddEntity("custom.fairy_tale", "колобок", "", -4);
        tester.TestTagger("music_fairytale", true, "прочитай сказку 'колобок'(fairy_tale) сначала пожалуйста");
    }

    Y_UNIT_TEST(Deterministic_DIALOG_8372) {
        const TVector<TString> firstFormGrammars = {
            NAlice::NUtUtils::NormalizeText(R"(
                form f1:
                    slots:
                        device:
                            type: user.iot.device
                            source: $user.iot.device
                        group:
                            type: user.iot.group
                            source: $user.iot.group
                    root:
                        не важно
            )"),
            NAlice::NUtUtils::NormalizeText(R"(
                form f1:
                    slots:
                        group:
                            type: user.iot.group
                            source: $user.iot.group
                        device:
                            type: user.iot.device
                            source: $user.iot.device
                    root:
                        не важно
            )"),
        };
        const TString secondFormGrammar = NAlice::NUtUtils::NormalizeText(R"(
            form f2:
                slots:
                    device:
                        type: user.iot.device
                        source: $user.iot.device
                    group:
                        type: user.iot.group
                        source: $user.iot.group
                root:
                    $Location
                $Location:
                    $user.iot.device
                    $user.iot.group
        )");
        for (const TString& firstFormGrammar : firstFormGrammars) {
            TGranetTester tester(firstFormGrammar + secondFormGrammar);
            // tester.EnableLog(true);
            tester.AddEntity("user.iot.device", "станция", "", -4);
            tester.AddEntity("user.iot.group", "станция", "", -4);
            tester.TestTagger("f2", true, "'станция'(group)");
        }
    }
}
