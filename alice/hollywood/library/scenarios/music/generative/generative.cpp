#include "generative.h"

#include <alice/hollywood/library/scenarios/music/music_backend_api/music_queue/music_queue.h>

#include <alice/megamind/protos/common/device_state.pb.h>
#include <alice/megamind/protos/common/location.pb.h>

namespace NAlice::NHollywood::NMusic {

namespace {

const TVector<TString> ALLOWED_GENERATIVE_STATIONS = {"generative:focus", "generative:energy", "generative:relax"};

} // namespace

std::unique_ptr<NScenarios::TScenarioRunResponse> HandleThinClientGenerative(
    TScenarioHandleContext& ctx,
    const TScenarioRunRequestWrapper& request,
    TNlgWrapper& nlg,
    TString generativeStationId,
    bool isNewContentRequestedByUser)
{
    if (Find(ALLOWED_GENERATIVE_STATIONS, generativeStationId) == ALLOWED_GENERATIVE_STATIONS.end()) {
        generativeStationId = ALLOWED_GENERATIVE_STATIONS[ctx.Rng.RandomInteger(ALLOWED_GENERATIVE_STATIONS.size())];
    }
    auto& logger = ctx.Ctx.Logger();
    LOG_INFO(logger) << "Handling thin client generative...";
    LOG_INFO(logger) << "Generative station id is " << generativeStationId;

    THwFrameworkRunResponseBuilder response{ctx, &nlg, ConstructBodyRenderer(request)};
    auto args = MakeMusicArguments(logger, request, TMusicArguments_EExecutionFlowType_ThinClientDefault,
                                   isNewContentRequestedByUser);
    auto& generativeRequest = *args.MutableGenerativeRequest();
    generativeRequest.SetStationId(generativeStationId);

    const TStringBuf uid = GetUid(request);
    args.MutableAccountStatus()->SetUid(uid.data(), uid.size());

    response.SetContinueArguments(args);

    return std::move(response).BuildResponse();
}

} // namespace NAlice::NHollywood::NMusic
