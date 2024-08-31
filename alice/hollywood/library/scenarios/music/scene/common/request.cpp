#include "request.h"

#include <alice/hollywood/library/scenarios/music/music_request_builder/music_request_builder.h>
#include <alice/hollywood/library/scenarios/music/music_request_builder/music_request_mode.h>

#include <alice/library/experiments/flags.h>

namespace NAlice::NHollywoodFw::NMusic {

NAppHostHttp::THttpRequest PrepareCommonHttpRequest(const TRequest& request, TStringBuf userId, TStringBuf path, TString name) {
    const bool enableCrossDc = request.Flags().IsExperimentEnabled(NExperiments::EXP_HW_MUSIC_ENABLE_CROSS_DC);

    const auto musicRequestModeInfo = NHollywood::NMusic::TMusicRequestModeInfoBuilder{}
        .SetAuthMethod(NHollywood::NMusic::EAuthMethod::UserId)
        .SetRequesterUserId(userId)
        .SetOwnerUserId(userId)
        .BuildAndMove();

    return NHollywood::NMusic::TMusicRequestBuilder{path, request.GetRequestMeta(), request.Client().GetClientInfo(), request.Debug().Logger(),
                                                    enableCrossDc, musicRequestModeInfo, name}
        .SetMethod(NAppHostHttp::THttpRequest_EMethod_Post)
        .Build(/* logVerbose = */ true);
}

} // namespace NAlice::NHollywoodFw::NMusic
