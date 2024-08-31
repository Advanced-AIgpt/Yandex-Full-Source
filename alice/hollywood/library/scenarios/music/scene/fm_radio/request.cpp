#include "request.h"

#include <alice/hollywood/library/scenarios/music/music_backend_api/api_path/api_path.h>
#include <alice/hollywood/library/scenarios/music/music_request_builder/music_request_builder.h>
#include <alice/hollywood/library/scenarios/music/music_request_builder/music_request_mode.h>
#include <alice/library/experiments/flags.h>
#include <alice/megamind/protos/common/location.pb.h>

using NAlice::NHollywood::NMusic::TMusicRequestBuilder;
using NAlice::NHollywood::NMusic::TMusicRequestModeInfoBuilder;

namespace NAlice::NHollywoodFw::NMusic::NFmRadio {

namespace {

class TRequestPreparer {
public:
    TRequestPreparer(const TMusicScenarioSceneArgsFmRadio& sceneArgs, const TRequest& request)
        : SceneArgs_{sceneArgs}
        , Request_{request}
    {
    }

    NAppHostHttp::THttpRequest Prepare() && {
        const TString path = GetPath();
        const bool enableCrossDc = Request_.Flags().IsExperimentEnabled(NExperiments::EXP_HW_MUSIC_ENABLE_CROSS_DC);
        auto musicRequestModeInfo = TMusicRequestModeInfoBuilder{}
                            .SetAuthMethod(NHollywood::NMusic::EAuthMethod::UserId)
                            .SetOwnerUserId(GetUid())
                            .SetRequesterUserId(GetUid())
                            .BuildAndMove();
        return TMusicRequestBuilder{path, Request_.GetRequestMeta(), Request_.Client().GetClientInfo(), Request_.Debug().Logger(),
                                    enableCrossDc, musicRequestModeInfo, "FmRadioRankedList"}.BuildAndMove(/* logVerbose = */ true);
    }

private:
    TString GetPath() const {
        const auto [lat, lon] = GetLatLon();
        return NHollywood::NMusic::NApiPath::FmRadioRankedList(GetUid(), GetIp(), lat, lon);
    }

    TStringBuf GetIp() const {
        if (const auto options = Request_.Client().TryGetMessage<NScenarios::TOptions>()) {
            return options->GetClientIP();
        }
        return TStringBuf();
    }

    std::pair<float, float> GetLatLon() const {
        if (const auto location = Request_.Client().TryGetMessage<TLocation>()) {
            return {location->GetLat(), location->GetLon()};
        }
        return {0.0, 0.0};
    }

    TStringBuf GetUid() const {
        return SceneArgs_.GetCommonArgs().GetAccountStatus().GetUid();
    }

private:
    const TMusicScenarioSceneArgsFmRadio& SceneArgs_;
    const TRequest& Request_;
};

} // namespace

NAppHostHttp::THttpRequest PrepareHttpRequest(const TMusicScenarioSceneArgsFmRadio& sceneArgs, const TRequest& request) {
    return TRequestPreparer{sceneArgs, request}.Prepare();
}

} // namespace NAlice::NHollywoodFw::NMusic::NFmRadio
