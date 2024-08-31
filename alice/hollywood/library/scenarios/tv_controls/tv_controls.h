#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/scenarios/tv_controls/proto/tv_controls.pb.h>

namespace NAlice::NHollywoodFw::NTvControls {

    class TTvControlsScenario: public TScenario {
    public:
        TTvControlsScenario();

        TRetScene Dispatch(const TRunRequest&,
                           const TStorage&,
                           const TSource&) const;

        static TRetResponse RenderIrrelevant(const TTvControlsRenderIrrelevant&, TRender&);
    };

    class TRelevantInability: public TScene<TTvControlsRenderNotSupported> {
    public:
        TRelevantInability(const TScenario* owner);

        TRetMain Main(const TTvControlsRenderNotSupported& args, const TRunRequest& request, TStorage&, const TSource&) const override;
        static TRetResponse RenderInability(const TTvControlsRenderNotSupported& args, TRender& render);
    };

}