#pragma once

//
// Random number, based on NEW hollywood framework
//

// Основной Framework Hollywood
#include <alice/hollywood/library/framework/framework.h>
// Файл с данными для рендеров
#include <alice/hollywood/library/scenarios/random_number/proto/random_number_render_state.pb.h>

namespace NAlice::NHollywoodFw::NRandomNumber {

//
// Диспетчер сценария
//
class TRandomNumberScenario : public TScenario {
public:
    TRandomNumberScenario();

    // Диспетчер
    TRetScene Dispatch(const TRunRequest&,
                       const TStorage&,
                       const TSource&) const;

    // Дополнительные рендереры для сценария
    static TRetResponse RenderIrrelevant(const TRandomNumberRenderIrrelevant&, TRender&);
};

}  // namespace NAlice::NHollywoodFw::NRandomNumber
