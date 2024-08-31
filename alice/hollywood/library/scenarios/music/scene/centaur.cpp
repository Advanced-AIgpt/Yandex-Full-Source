#include "centaur.h"

#include <alice/hollywood/library/scenarios/music/centaur/common.h>
#include <alice/hollywood/library/scenarios/music/common.h>
#include <alice/hollywood/library/scenarios/music/music_request_builder/music_request_mode.h>
#include <alice/hollywood/library/scenarios/music/proto/centaur.pb.h>
#include <alice/hollywood/library/scenarios/music/proto/music_arguments.pb.h>
#include <alice/hollywood/library/scenarios/music/util/music_proxy_request.h>

#include <alice/hollywood/library/http_proxy/request_meta_provider.h>
#include <alice/library/experiments/flags.h>
#include <alice/megamind/protos/blackbox/blackbox.pb.h>

namespace NAlice::NHollywoodFw::NMusic {

namespace {

NHollywood::NMusic::TMusicRequestMeta ConstructMusicRequestMeta(const TContinueRequest& request) {
    NHollywood::NMusic::TMusicRequestMeta meta;
    *meta.MutableRequestMeta() = request.GetRequestMeta();
    *meta.MutableClientInfo() = request.GetContinueRequest().GetBaseRequest().GetClientInfo();
    meta.SetEnableCrossDc(request.Flags().IsExperimentEnabled(NExperiments::EXP_HW_MUSIC_ENABLE_CROSS_DC));

    NHollywood::TMusicArguments args;
    if (request.GetArguments(args)) {
        meta.SetUserId(args.GetAccountStatus().GetUid());
    }

    return meta;
}

TMaybe<NHollywood::NMusic::NCentaur::TCollectMainScreenResponse> TryGetCollectMainScreenResponse(const TSource& source) {
    NHollywood::NMusic::NCentaur::TCollectMainScreenResponse collectMainScreenResponse;
    if (source.GetSource(NHollywood::NMusic::NCentaur::COLLECT_MAIN_SCREEN_RESPONSE_ITEM, collectMainScreenResponse)) {
        return std::move(collectMainScreenResponse);
    }
    return Nothing();
}

} // namespace

TMusicScenarioSceneCentaur::TMusicScenarioSceneCentaur(const TScenario* owner)
    : TScene{owner, "centaur"}
{
    RegisterRenderer(&TMusicScenarioSceneCentaur::Render);
}

TRetMain TMusicScenarioSceneCentaur::Main(const TMusicScenarioSceneArgsCentaur&, const TRunRequest& req, TStorage&, const TSource&) const {
    NHollywood::TMusicArguments args;
    if (const auto* dataSource = req.GetDataSource(NAlice::EDataSourceType::BLACK_BOX)) {
        const TStringBuf uid = dataSource->GetUserInfo().GetUid();
        args.MutableAccountStatus()->SetUid(uid.data(), uid.size());
    }
    return TReturnValueContinue(args);
}

TRetSetup TMusicScenarioSceneCentaur::ContinueSetup(const TMusicScenarioSceneArgsCentaur&,
                                                    const TContinueRequest& request,
                                                    const TStorage&) const
{
    TSetup setup{request};

    // attach sub-graph request
    NHollywood::NMusic::NCentaur::TCollectMainScreenRequest collectMainScreenRequest;
    *collectMainScreenRequest.MutableMusicRequestMeta() = ConstructMusicRequestMeta(request);
    setup.AttachRequest(NHollywood::NMusic::NCentaur::COLLECT_MAIN_SCREEN_REQUEST_ITEM, collectMainScreenRequest);

    return setup;
}

TRetContinue TMusicScenarioSceneCentaur::Continue(const TMusicScenarioSceneArgsCentaur&,
                                                  const TContinueRequest& request,
                                                  TStorage&,
                                                  const TSource& source) const
{
    if (const auto collectMainScreenResponse = TryGetCollectMainScreenResponse(source)) {
        return TReturnValueRender(&TMusicScenarioSceneCentaur::Render, collectMainScreenResponse->GetScenarioData());
    } else {
        LOG_ERR(request.Debug().Logger()) << "No response from music infinite feed found";
        return TError{TError::EErrorDefinition::ExternalError};
    }
}

TRetResponse TMusicScenarioSceneCentaur::Render(const NData::TScenarioData& scenarioData,
                                                TRender& render) const
{
    render.SetScenarioData(NData::TScenarioData{scenarioData});
    return TReturnValueSuccess();
}

} // namespace NAlice::NHollywoodFw::NMusic
