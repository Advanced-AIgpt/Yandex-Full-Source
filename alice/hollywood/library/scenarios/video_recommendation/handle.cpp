#include "handle.h"

#include "state_updater.h"

#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <alice/library/json/json.h>
#include <alice/library/logger/logger.h>
#include <alice/library/video_common/defs.h>
#include <alice/megamind/protos/scenarios/request.pb.h>

#include <util/stream/str.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood {

namespace {

const TString FRAME_NAME = "personal_assistant.scenarios.video_recommendation";

const TStateUpdater::TFormDescription FORM_DESCRIPTION = {
    {TString{NVideoCommon::SLOT_ABOUT}, {"string"}},
    {TString{NVideoCommon::SLOT_GENRE}, {"custom.video_film_genre"}},
    {TString{NVideoCommon::SLOT_COUNTRY}, {"custom.video_recommendation_country"}},
    {TString{NVideoCommon::SLOT_RELEASE_DATE}, {"custom.year_adjective", "custom.year"}}
};

} // namespace

void TVideoRecommendationRunHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);

    const auto& videoDatabase = ctx.Ctx.ScenarioResources<TVideoDatabase>();

    TStateUpdater updater(FRAME_NAME, FORM_DESCRIPTION, videoDatabase, requestProto);
    updater.Update();

    const TScenarioRunRequestWrapper request(requestProto, ctx.ServiceCtx);
    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder builder(&nlgWrapper);

    auto& bodyBuilder = builder.CreateResponseBodyBuilder();

    TNlgData nlgData(ctx.Ctx.Logger(), request);
    nlgData.Context = updater.GetNlgContext();
    bodyBuilder.AddRenderedTextWithButtonsAndVoice(/* nlgTemplateName = */ "video_recommendation",
                                                   /* phraseName = */ "render_result",
                                                   /* buttons = */ {},
                                                   nlgData);

    auto response = std::move(builder).BuildResponse();

    auto& responseBody = *response->MutableResponseBody();
    *responseBody.MutableSemanticFrame() = updater.GetSemanticFrame();
    *responseBody.MutableEntities() = updater.GetEntities();
    responseBody.MutableState()->PackFrom(updater.GetState());

    const auto frameActions = updater.GetFrameActions();
    responseBody.MutableFrameActions()->insert(frameActions.begin(), frameActions.end());

    if (const auto& maybeShowGalleryDirective = updater.GetShowGalleryDirective()) {
        *responseBody.MutableLayout()->AddDirectives()->MutableShowGalleryDirective() = *maybeShowGalleryDirective;
    }

    ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
}

REGISTER_SCENARIO("video_recommendation",
                   AddHandle<TVideoRecommendationRunHandle>()
                   .SetResources<TVideoDatabase>()
                   .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NVideoRecommendation::NNlg::RegisterAll));

} // namespace NAlice::NHollywood
