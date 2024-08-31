#include "random_number_scene_rnd.h"

#include <alice/library/json/json.h>
#include <alice/library/scled_animations/scled_animations_builder.h>
#include <alice/library/scled_animations/scled_animations_directive.h>

#include <alice/megamind/protos/scenarios/response.pb.h>

#include <util/string/printf.h>

namespace NAlice::NHollywoodFw::NRandomNumber {

namespace {

//
// Константы по умолчанию для "Загадай случайное число"
//
constexpr int DEFAULT_FROM = 1;
constexpr int DEFAULT_TO = 100;

constexpr int N_SCLED_ANITED_STEPS = 12;
constexpr int SCLED_ANITED_DURATION = 180;

//
// Вспомогательная функция для сегментной анимации
//
TString ConstructRandomNumScled(const int value) {
    if (value > 99 || value < 0) {
        return "";
    }
    return TString::Join("  ", Sprintf("% 3d", value), " ");
}

} // anonimous namespace

/*
    Конструктор регистрации для TRandomNumberSceneRandom
*/
TRandomNumberSceneRandom::TRandomNumberSceneRandom(const TScenario* owner)
    : TScene(owner, SCENE_NAME_RANDOM)
{
    // Регистрируем рендереры - загадай случайное число....
    RegisterRenderer(&TRandomNumberSceneRandom::RenderRandom);
}

/*
    Главная логика работы TRandomNumberSceneRandom (функция Main())
*/
TRetMain TRandomNumberSceneRandom::Main(const TRandomNumberSceneArgsRandom& args,
                                        const TRunRequest& runRequest,
                                        TStorage& storage,
                                        const TSource&) const
{
    // Извлекаем данные из TStorage
    TRandomNumberScenarioState scenarioState;
    switch (storage.GetScenarioState(scenarioState, std::chrono::seconds(300))) {
        case TStorage::EScenarioStateResult::Absent:
        case TStorage::EScenarioStateResult::Expired:
            // Данные сценария не существуют или прошло слишком много времени
            // Контроль времени задан в SetScenarioStateTimeout(std::chrono::seconds(300));
            scenarioState.Clear();
            break;
        case TStorage::EScenarioStateResult::NewSession:
        case TStorage::EScenarioStateResult::Present:
            // Будем использовать данные сценария
            // (даже если началась новая сессия)
            break;
    }

    // Добавляем / переписываем значения из args
    if (args.HasLowerBound()) {
        scenarioState.SetLowerBound(args.GetLowerBound());
    }
    if (args.HasUpperBound()) {
        scenarioState.SetUpperBound(args.GetUpperBound());
    }

    // Сохраняем ScenarioState обратно в TStorage
    storage.SetScenarioState(scenarioState);

    // Основная бизнес-логика сценария
    i64 lowerBound = scenarioState.HasLowerBound() ? scenarioState.GetLowerBound() : DEFAULT_FROM;
    i64 upperBound = scenarioState.HasUpperBound() ? scenarioState.GetUpperBound() : DEFAULT_TO;
    if (lowerBound > upperBound) {
        std::swap(lowerBound, upperBound);
    }
    int result = runRequest.System().Random().RandomInteger(lowerBound, upperBound + 1);
    LOG_INFO(runRequest.Debug().Logger()) << "RANDOM_NUMBER: Result: " << result << " in range: " << lowerBound << "-" << upperBound;

    // Готовим данные для ответа
    TRandomNumberRenderStateRandom rst;
    rst.SetLowerBound(lowerBound);
    rst.SetUpperBound(upperBound);
    rst.SetValue(result);
    return TReturnValueRender(&TRandomNumberSceneRandom::RenderRandom, rst);
}

/*
    Рендерер для "назови случайное число"
    Включает в себя обычный рендерер NLG + сегментная анимация для Мини-2
*/
TRetResponse TRandomNumberSceneRandom::RenderRandom(const TRandomNumberRenderStateRandom& state, TRender& render) {
    render.CreateFromNlg("random_number", "render_result", state);

    // Add scled animation to random
    TString pattern = ConstructRandomNumScled(state.GetValue());
    if (!pattern.Empty() && render.GetRequest().Client().GetInterfaces().GetHasScledDisplay()) {
        NAlice::TScledAnimationBuilder scled;

        // Добавить появление числа от 0 до 255 (полная яркость) за 500 мс
        scled.AddAnim(pattern, 0, 255, 500, TScledAnimationBuilder::AnimModeFade);
        // Задержать число на экране на 2 секунды
        scled.AddDraw(pattern, 255, 2000);
        // Плавно погасить число за 500 мс
        scled.AddAnim(pattern, 255, 0, 500, TScledAnimationBuilder::AnimModeFade);

        // Добавляем анимацию "бросания кубика" в крайнем левом сегменте
        static TVector<TScledAnimationBuilder::TFullPattern> animation = {
            TScledAnimationBuilder::String2Pattern("#1000000     "),
            TScledAnimationBuilder::String2Pattern("#0100000     "),
            TScledAnimationBuilder::String2Pattern("#0010000     "),
            TScledAnimationBuilder::String2Pattern("#0001000     "),
            TScledAnimationBuilder::String2Pattern("#0000100     "),
            TScledAnimationBuilder::String2Pattern("#0000010     "),
        };
        for (int i = 0; i < N_SCLED_ANITED_STEPS; i++) {
            scled.SetDraw(animation[i % animation.size()], 255,
                          SCLED_ANITED_DURATION * i,
                          SCLED_ANITED_DURATION * (i + 1));
        }
        NScledAnimation::TScledAnimationOptions options;
        options.AddTtslayPlaceholder = true;
        NScledAnimation::AddDrawScled(render.GetResponseBody(), scled, options);
    }
    return TReturnValueSuccess();
}

} // namespace NAlice::NHollywoodFw::NRandomNumber
