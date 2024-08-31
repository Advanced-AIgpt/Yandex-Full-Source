#pragma once

//
// Random number, based on NEW hollywood framework
// Вспомогательный Header-file для разбора семантических фреймов
//

// Основной Framework Hollywood
#include <alice/hollywood/library/framework/framework.h>

#include <alice/hollywood/library/scenarios/random_number/proto/random_number_render_state.pb.h>
#include <alice/hollywood/library/scenarios/random_number/proto/random_number_scenario_state.pb.h>

namespace NAlice::NHollywoodFw::NRandomNumber {

//
// Константы по умолчанию для "Загадай случайное число"
//
inline constexpr TStringBuf SCENE_NAME_RANDOM = "scene_rnd";
inline constexpr TStringBuf FRAME_NAME_RANDOM = "alice.random_number";

//
// Фрейм для "Загадай случайное число"
//
struct TFrameRandomNumber : public TFrame {
    TFrameRandomNumber(const TRequest::TInput& input)
        : TFrame(input, FRAME_NAME_RANDOM)
        , LowerBound(this, "lower_bound")
        , UpperBound(this, "upper_bound")
    {
    }

    TOptionalSlot<i64> LowerBound;
    TOptionalSlot<i64> UpperBound;
};

//
// Сцена для "Загадай случайное число"
//
class TRandomNumberSceneRandom : public TScene<TRandomNumberSceneArgsRandom> {
public:
    TRandomNumberSceneRandom(const TScenario* owner);
    TRetMain Main(const TRandomNumberSceneArgsRandom&,
                  const TRunRequest&,
                  TStorage&,
                  const TSource&) const override;
    // Рендер для "Загадай случайное число"
    // В данном случае сцены "Загадай случайное число" и "Брось кубик" имеют принципиально разные рендеры
    // поэтому их удобнее расположить как члены сцен.
    // В других ситуациях удобнее может быть использовать общие рендеры как free function или как часть сценарного класса
    static TRetResponse RenderRandom(const TRandomNumberRenderStateRandom&, TRender&);
};

}  // namespace NAlice::NHollywoodFw::NRandomNumber
