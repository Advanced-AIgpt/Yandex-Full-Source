#pragma once

#include "draw_picture_impl.h"

#include <alice/hollywood/library/base_scenario/scenario.h>


namespace NAlice::NHollywood {

    class TDrawPictureRankedRunHandle : public TScenario::THandleBase {
    public:
        TString Name() const override {
            return "ranked";
        }

        void Do(TScenarioHandleContext& ctx) const override;
        void Fallback(TDrawPictureImpl& impl) const;

    };

} // namespace NAlice::NHollywood
