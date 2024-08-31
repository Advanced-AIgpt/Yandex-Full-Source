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
// Константы по умолчанию для "Брось кубик"
//
inline constexpr TStringBuf SCENE_NAME_DICE = "scene_dice";
inline constexpr TStringBuf FRAME_NAME_DICE = "alice.throw_dice";
inline const TString FRAME_NAME_DICE_MORE = "alice.throw_dice.more";

//
// Фрейм для "Брось кубик"
//
struct TFrameThrowDice : public TFrame {
    TFrameThrowDice(const TRequest::TInput& input)
        : TFrame(input, FRAME_NAME_DICE)
        , NumDices(this, "num_dices")
        , NumThrows(this, "num_throw")
        , NumEdges(this, "num_edges")
        , GameCode(this, "game_type")
    {
    }

    TArraySlot<i64> NumDices;
    TOptionalSlot<i64> NumThrows;
    TOptionalSlot<i64> NumEdges;
    TOptionalSlot<TString> GameCode;
};

//
// Бизнес логика сценария "Брось кубик(и)"
//
class TRandomNumberSceneDice : public TScene<TRandomNumberSceneArgsDice> {
public:
    TRandomNumberSceneDice(const TScenario* owner);
    TRetMain Main(const TRandomNumberSceneArgsDice&,
                  const TRunRequest&,
                  TStorage&,
                  const TSource&) const override;
    // Рендер для "Брось кубик"
    // В данном случае сцены "Загадай случайное число" и "Брось кубик" имеют принципиально разные рендеры
    // поэтому их удобнее расположить как члены сцен.
    // В других ситуациях удобнее может быть использовать общие рендеры как free function или как часть сценарного класса
    static TRetResponse RenderDice(const TRandomNumberRenderStateDice&, TRender&);
};


}  // namespace NAlice::NHollywoodFw::NRandomNumber
