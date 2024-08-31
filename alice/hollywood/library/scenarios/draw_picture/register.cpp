#include "draw_picture.h"
#include "draw_picture_ranked.h"
#include "draw_picture_resources.h"

#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/scenarios/draw_picture/nlg/register.h>

namespace NAlice::NHollywood {

    REGISTER_SCENARIO(
        "draw_picture",
        AddHandle<TDrawPictureRunHandle>()
            .AddHandle<TDrawPictureRankedRunHandle>()
            .SetResources<TDrawPictureResources>()
            .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NDrawPicture::NNlg::RegisterAll)
    );

} // namespace NAlice::NHollywood
