#include "centaur_main_screen.h"

#include <alice/hollywood/library/combinators/analytics_info/builder.h>
#include <alice/hollywood/library/combinators/combinators/centaur/main_screen/add_widget_from_gallery.h>
#include <alice/hollywood/library/combinators/combinators/centaur/main_screen/prepare_div_patch.h>
#include <alice/hollywood/library/combinators/combinators/centaur/main_screen/prepare_main_screen.h>
#include <alice/hollywood/library/combinators/combinators/centaur/main_screen/prepare_widget_gallery.h>

#include <alice/hollywood/library/combinators/protos/used_scenarios.pb.h>
#include <alice/hollywood/library/combinators/response/response_builder.h>
#include <alice/megamind/protos/scenarios/combinator_request.pb.h>
#include <alice/megamind/protos/scenarios/combinator_response.pb.h>
#include <alice/megamind/protos/scenarios/frame.pb.h>
#include <alice/protos/data/scenario/data.pb.h>
#include <google/protobuf/wrappers.pb.h>

namespace NAlice::NHollywood::NCombinators {

TCentaurMainScreenCombinator::TCentaurMainScreenCombinator(THwServiceContext& ctx)
    : Ctx(ctx),
      CombinatorRequest(Ctx.GetProtoOrThrow<TCombinatorRequest>(AH_ITEM_COMBINATOR_REQUEST_NAME)),	
      Request(CombinatorRequest, Ctx.Logger(), Ctx)
{
}

void TCentaurMainScreenCombinator::Run() {
    LOG_INFO(Ctx.Logger()) << "CentaurMainScreen combinator starts run stage";
    TCombinatorResponse resp;
    // TODO process availiable scenarios here and fill arguments
    google::protobuf::StringValue args;
    resp.MutableResponse()->MutableContinueArguments()->PackFrom(std::move(args));
    LOG_INFO(Ctx.Logger()) << "Returning continue arguments";
    Ctx.AddProtobufItemToApphostContext(resp, AH_ITEM_COMBINATOR_RESPONSE_NAME);
}

void TCentaurMainScreenCombinator::Continue() {
    LOG_INFO(Ctx.Logger()) << "CentaurMainScreen combinator starts continue stage";
    if (const auto mainScreenSemanticFrame = Request.Input().FindSemanticFrame(CENTAUR_COLLECT_MAIN_SCREEN_FRAME)) {
        TFrame frame = TFrame::FromProto(*mainScreenSemanticFrame);
        if (frame.FindSlot("widget_gallery_position")) {
            TPrepareWidgetGallery{Ctx, CombinatorRequest}.Do(*mainScreenSemanticFrame);
        } else if (Request.HasExpFlag(PATCH_MAIN_SCREEN_EXP_FLAG_NAME)
            && frame.FindSlot("widget_config_data_slot") && frame.FindSlot("widget_main_screen_position")) {
                TPrepareDivPatch{Ctx, CombinatorRequest}.Do(*mainScreenSemanticFrame);
        } else {
            TPrepareMainScreen{Ctx, CombinatorRequest}.Do();
        }
    } else if (const auto addWidgetSemanticFrame = Request.Input().FindSemanticFrame(CENTAUR_ADD_WIDGET_FROM_GALLERY_FRAME)) {
        TAddWidgetFromGallery{Ctx, CombinatorRequest}.Do(*addWidgetSemanticFrame);
    } else {
        LOG_ERROR(Ctx.Logger()) << "Semantic frame not supported";
    }
}

void TCentaurMainScreenCombinator::Finalize() {
    LOG_INFO(Ctx.Logger()) << "CentaurMainScreen combinator starts finalize stage";
    const auto renderedResponse =
        Ctx.GetProtoOrThrow<TScenarioRunResponse>(RESPONSE_ITEM);
    LOG_INFO(Ctx.Logger()) << "Successfully got response from div_renderer. Start building combinator response";
    const auto usedScenarios = Ctx.GetMaybeProto<TCombinatorUsedScenarios>(COMBINATOR_USED_SCENARIOS_ITEM);

    TCombinatorRunResponseBuilder builder{};

    if (usedScenarios.Defined()) {
        for (const auto& usedScenario : usedScenarios->GetScenarioNames()) {
            builder.AddUsedScenario(usedScenario);
        }
    } else {
        LOG_WARNING(Ctx.Logger()) << "No used_scenarios item for combinator finalization";
    }

    TCombinatorAnalyticsInfoBuilder analyticsInfoBuilder{Request};
    analyticsInfoBuilder.SetCombinatorProductName(CENTAUR_MAIN_SCREEN_COMBINATOR_PSN);

    builder.AnalyticsInfo(std::move(analyticsInfoBuilder).MoveProto());
    builder.ScenarioResponse(renderedResponse);
    LOG_INFO(Ctx.Logger()) << "Combinator response is ready";

    Ctx.AddProtobufItemToApphostContext(std::move(builder).MoveProto(), AH_ITEM_COMBINATOR_RESPONSE_NAME);
    LOG_INFO(Ctx.Logger()) << "Combinator response has been put into context";
}

void TCentaurMainScreenRunHandle::Do(THwServiceContext& ctx) const {
    TCentaurMainScreenCombinator{ctx}.Run();
}

void TCentaurMainScreenContinueHandle::Do(THwServiceContext& ctx) const {
    TCentaurMainScreenCombinator{ctx}.Continue();
}

void TCentaurMainScreenFinalizeHandle::Do(THwServiceContext& ctx) const {
    TCentaurMainScreenCombinator{ctx}.Finalize();
}

} // namespace NAlice::NHollywood::NCombinators
