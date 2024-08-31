#include "run_render_handle.h"

#include "common.h"
#include "intents.h"

#include <alice/hollywood/library/bass_adapter/bass_adapter.h>
#include <alice/hollywood/library/bass_adapter/bass_renderer.h>
#include <alice/hollywood/library/datasync_adapter/datasync_adapter.h>
#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/scenarios/music/analytics_info/analytics_info.h>
#include <alice/hollywood/library/scenarios/music/util/util.h>

#include <alice/megamind/protos/property/property.pb.h>

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/json/json.h>

#include <library/cpp/json/json_value.h>

#include <memory>


using namespace NAlice::NScenarios;
using namespace ru::yandex::alice::memento::proto;

namespace NAlice::NHollywood::NMusic {

namespace {

NJson::TJsonValue FilterOutTextAndDivCardBlocks(const NJson::TJsonValue& blocks) {
    NJson::TJsonValue filtredBlocks(NJson::JSON_ARRAY);
    for (const auto& block : blocks.GetArray()) {
        const NJson::TJsonValue* type = nullptr;
        if (!block.GetValuePointer("type", &type) ||
            (type->GetString() != "text_card" && type->GetString() != "div_card" && type->GetString() != "div2_card"))
        {
            filtredBlocks.AppendValue(block);
        }
    }
    return filtredBlocks;
}

void SavePushesFromDataSyncResponse(TMusicHardcodedArguments& args, const NJson::TJsonValue& responseBody, const bool resetPushes) {
    auto pushRaw = JsonFromString(responseBody["value"].GetString()).GetMap();
    args.SetPushesSent(resetPushes ? 0 : pushRaw["pushes_sent"].GetUInteger());
    args.SetLastPushTimestamp(resetPushes ? 0: pushRaw["last_push_timestamp"].GetUInteger());
}

} // namespace

namespace NImpl {

[[nodiscard]] std::unique_ptr<TScenarioRunResponse> MusicHardcodedRenderDoImpl(
    const TScenarioRunRequestWrapper& runRequest,
    TMaybe<TMorningShowProfile> morningShowProfile,
    TMusicHardcodedArguments& applyArgs,
    NJson::TJsonValue bassResponse,
    TScenarioHandleContext& ctx,
    TNlgWrapper& nlgWrapper)
{
    const auto intent = CreateMusicHardcodedIntent(ctx.Ctx.Logger(), runRequest);
    Y_ENSURE(intent, "Failed to create music hardcoded intent");

    bassResponse["blocks"] = FilterOutTextAndDivCardBlocks(bassResponse["blocks"]);
    THwFrameworkRunResponseBuilder builder(ctx, &nlgWrapper, ConstructBodyRenderer(runRequest));
    auto& bodyBuilder = builder.CreateCommitCandidate(applyArgs);
    TBassResponseRenderer bassRenderer(runRequest, runRequest.Input(), builder, ctx.Ctx.Logger());

    intent->RenderResponse(bassResponse, bassRenderer);

    FillAnalyticsInfoMusicEvent(ctx.Ctx.Logger(), bassResponse, &bodyBuilder, runRequest);
    bodyBuilder.GetAnalyticsInfoBuilder().SetProductScenarioName(intent->ProductScenarioName());

    auto responseProto = std::move(builder).BuildResponse();

    if (morningShowProfile) {
        auto* morningShowUserInfo = responseProto->MutableUserInfo()->AddProperties();
        morningShowUserInfo->SetName("custom-morning-show");
        morningShowUserInfo->SetId("custom-morning-show");
        morningShowUserInfo->SetHumanReadable("Morning show customisation");
        *morningShowUserInfo->MutableMorningShowProfile() = std::move(*morningShowProfile);
    }

    return responseProto;
}

} // namespace NImpl

void TBassMusicHardcodedRenderHandle::Do(TScenarioHandleContext& ctx) const {
    const auto bassResponseBody = RetireBassRequest(ctx);
    const auto runRequest = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request(runRequest, ctx.ServiceCtx);
    const auto intent = CreateMusicHardcodedIntent(ctx.Ctx.Logger(), request);

    TMaybe<TMorningShowProfile> morningShowProfileToPlay;
    TMusicHardcodedArguments applyArgs;
    TMaybe<THardcodedMorningShowSemanticFrame> sourceFrame;
    if (intent && intent->ProductScenarioName() == NAlice::NProductScenarios::ALICE_SHOW) {
        if (const auto rawSourceFrame = request.Input().FindSemanticFrame(ALICE_SHOW_INTENT); rawSourceFrame) {
            sourceFrame = rawSourceFrame->GetTypedSemanticFrame().GetHardcodedMorningShowSemanticFrame();
        }
        TMorningShowProfile morningShowProfileStored = ParseMorningShowProfile(request.BaseRequestProto().GetMemento());

        morningShowProfileToPlay.ConstructInPlace(morningShowProfileStored);
        TryUpdateMorningShowProfileFromFrame(morningShowProfileToPlay.GetRef(), sourceFrame, true);

        if (!sourceFrame || sourceFrame->GetOffset().GetNumValue() == 0) {
            SavePushesFromDataSyncResponse(
                applyArgs,
                RetireDataSyncResponseItemsSafe(ctx).GetOrElse(NJson::TJsonValue("{\"value\": \"{}\"}")),
                request.HasExpFlag(EXP_HW_MORNING_SHOW_RESET_PUSHES)
            );
            applyArgs.SetUid(TString{GetUid(request)});
        }
    }

    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    auto response = NImpl::MusicHardcodedRenderDoImpl(request, morningShowProfileToPlay, applyArgs, bassResponseBody, ctx, nlgWrapper);

    ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
}

} // namespace NAlice::NHollywood::NMusic
