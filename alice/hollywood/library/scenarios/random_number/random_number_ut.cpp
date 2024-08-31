#include "random_number.h"
#include "random_number_scene_rnd.h"
#include "random_number_scene_dice.h"

#include <alice/hollywood/library/framework/framework_ut.h>

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/json/json.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/string/util.h>

namespace NAlice::NHollywoodFw::NRandomNumber {

namespace {

} // anonimous namespace

Y_UNIT_TEST_SUITE(RandomNumber) {
    //
    // Тестирование функции диспетчера для "Случайное число" и "Брось кубик"
    //
    Y_UNIT_TEST(RandomNumberDispatcherRandom) {
        TTestEnvironment testData(NProductScenarios::RANDOM_NUMBER, "ru-ru");

        // Задаем параметры для функции диспетчера
        testData.AddSemanticFrame("alice.random_number", "{}");
        testData.AddSemanticFrameSlot("alice.random_number", "lower_bound", 20);
        testData.AddSemanticFrameSlot("alice.random_number", "upper_bound", 30);

        // Вызываем диспетчер
        UNIT_ASSERT(testData >> TTestDispatch(&TRandomNumberScenario::Dispatch) >> testData);

        // Проверяем результаты
        UNIT_ASSERT(testData.GetSelectedSceneName() == SCENE_NAME_RANDOM);
        const auto& msg = testData.GetSelectedSceneArguments();
        TRandomNumberSceneArgsRandom sceneArguments;
        UNIT_ASSERT(msg.UnpackTo(&sceneArguments));
        UNIT_ASSERT(sceneArguments.GetLowerBound() == 20);
        UNIT_ASSERT(sceneArguments.GetUpperBound() == 30);

    }
    Y_UNIT_TEST(RandomNumberDispatcherThrowDice) {
        TTestEnvironment testData(NProductScenarios::RANDOM_NUMBER, "ru-ru");

        // Задаем параметры для функции диспетчера
        testData.AddSemanticFrame("alice.throw_dice", "{}");
        testData.AddSemanticFrameSlot("alice.throw_dice", "num_dices", 1);
        testData.AddSemanticFrameSlot("alice.throw_dice", "num_dices", 2);

        // Вызываем диспетчер
        UNIT_ASSERT(testData >> TTestDispatch(&TRandomNumberScenario::Dispatch) >> testData);

        // Проверяем результаты
        UNIT_ASSERT(testData.GetSelectedSceneName() == SCENE_NAME_DICE);
        const auto& msg = testData.GetSelectedSceneArguments();
        TRandomNumberSceneArgsDice sceneArguments;
        UNIT_ASSERT(msg.UnpackTo(&sceneArguments));
        UNIT_ASSERT(sceneArguments.GetDiceCount() == 2);
        UNIT_ASSERT(!sceneArguments.HasEdgeCount());
    }

    //
    // Тестирование функции рендеринга для "Загадай случайное число" (&TRandomNumberScenario::RenderRandom)
    //
    Y_UNIT_TEST(RandomNumberRenderRandom) {
        TTestEnvironment testData(NProductScenarios::RANDOM_NUMBER, "ru-ru");

        // Задаем параметры для функции рендерера
        TRandomNumberRenderStateRandom renderStateRandom;
        renderStateRandom.SetLowerBound(1);
        renderStateRandom.SetUpperBound(8);
        renderStateRandom.SetValue(2);

        // Вызываем рендер
        UNIT_ASSERT(testData >> TTestRender(&TRandomNumberSceneRandom::RenderRandom, renderStateRandom) >> testData);
        UNIT_ASSERT(!testData.IsIrrelevant());
        UNIT_ASSERT(testData.ContainsText("2."));
    }

    //
    // Тестирование функции рендеринга для "Брось кубик" (&TRandomNumberScenario::RenderDice)
    //
    Y_UNIT_TEST(RandomNumberRenderDice) {
        TTestEnvironment testData(NProductScenarios::RANDOM_NUMBER, "ru-ru");

        // Задаем параметры для функции рендерера
        TRandomNumberRenderStateDice renderStateDice;
        renderStateDice.SetDiceCount(5);
        renderStateDice.AddValues(3);
        renderStateDice.AddValues(5);
        renderStateDice.SetSum(8);
        renderStateDice.SetNewSession(false);

        // Вызываем рендер
        UNIT_ASSERT(testData >> TTestRender(&TRandomNumberSceneDice::RenderDice, renderStateDice) >> testData);
        UNIT_ASSERT(!testData.IsIrrelevant());
        // Из за вариативности NLG проверяем или фразы, завершающиеся на 8 очков или фразу "Сумма очков 5 кубиков: 8.
        UNIT_ASSERT_STRING_CONTAINS(testData.GetText(), "3 и 5");
        UNIT_ASSERT_STRING_CONTAINS(testData.GetVoice(), "#nom 3. #nom 5.");
        UNIT_ASSERT(testData.ContainsText("сумма 8.") || testData.ContainsText("в сумме 8."));
    }

    //
    // Тестирование иррелевантного ответа диспетчера
    //
    Y_UNIT_TEST(RandomNumberIrrelevant) {
        TTestEnvironment testData(NProductScenarios::RANDOM_NUMBER, "ru-ru");

        // Вызываем диспетчер
        UNIT_ASSERT(testData >> TTestDispatch(&TRandomNumberScenario::Dispatch) >> testData);
        UNIT_ASSERT(testData.IsIrrelevant());
    }

} // Y_UNIT_TEST_SUITE

} // namespace NAlice::NHollywoodFw::NRandomNumber
