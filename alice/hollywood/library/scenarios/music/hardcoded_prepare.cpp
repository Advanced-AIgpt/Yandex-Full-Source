#include "common.h"
#include "hardcoded_prepare.h"
#include "intents.h"

#include <alice/hollywood/library/scenarios/music/util/util.h>

#include <alice/library/json/json.h>

namespace NAlice::NHollywood::NMusic {

void TBassMusicHardcodedPrepareHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};
    const auto intent = CreateMusicHardcodedIntent(ctx.Ctx.Logger(), request);
    auto nlg = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    if (!intent) {
        LOG_DEBUG(ctx.Ctx.Logger()) << "Failed to create music hardcoded intent";
        THwFrameworkRunResponseBuilder response(ctx, &nlg, ConstructBodyRenderer(request));
        response.SetIrrelevant();

        auto& bodyBuilder = response.CreateResponseBodyBuilder();
        TNlgData nlgData{ctx.Ctx.Logger(), request};
        bodyBuilder.AddRenderedTextWithButtonsAndVoice(TEMPLATE_MUSIC_PLAY, "error", /* buttons = */ {}, nlgData);
        ctx.ServiceCtx.AddProtobufItem(*std::move(response).BuildResponse(), RESPONSE_ITEM);
        return;
    }

    const auto frameOrResponce = intent->PrepareMusicFrame(ctx, request, nlg);

    struct {
        TScenarioHandleContext& Ctx;
        const TScenarioRunRequestWrapper& Request;

        // normal course of action: go to BASS next
        void operator()(const TFrame& frame) {
            const auto bassRequest = PrepareBassVinsRequest(Ctx.Ctx.Logger(), Request,
                                                            frame, /* sourceTextProvider= */ nullptr,
                                                            Ctx.RequestMeta, /* imageSearch= */ false, Ctx.AppHostParams);
            AddBassRequestItems(Ctx, bassRequest);
        }

        // early exit
        void operator()(const NScenarios::TScenarioRunResponse& response) {
            Ctx.ServiceCtx.AddProtobufItem(response, RESPONSE_ITEM);
        }
    } visitor{ctx, request};

    std::visit(visitor, frameOrResponce);
}

} // namespace NAlice::NHollywood::NMusic
