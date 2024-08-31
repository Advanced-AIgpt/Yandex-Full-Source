#include "centaur_teasers.h"
#include "schedule_service.h"

#include <alice/hollywood/library/combinators/analytics_info/builder.h>
#include <alice/hollywood/library/combinators/metrics/names.h>
#include <alice/hollywood/library/combinators/request/request.h>
#include <alice/hollywood/library/combinators/response/response_builder.h>
#include <alice/library/proto/proto.h>
#include <catboost/private/libs/data_types/pair.h>
#include <google/protobuf/repeated_field.h>

#include <alice/hollywood/library/combinators/protos/used_scenarios.pb.h>
#include <alice/megamind/protos/scenarios/combinator_request.pb.h>
#include <alice/megamind/protos/scenarios/combinator_response.pb.h>
#include <alice/protos/api/renderer/api.pb.h>
#include <alice/protos/data/scenario/centaur/teasers.pb.h>
#include <alice/protos/data/scenario/data.pb.h>

#include <alice/hollywood/library/combinators/combinators/centaur/teasers/prepare_teasers.h>
#include <alice/hollywood/library/combinators/combinators/centaur/teasers/prepare_teaser_settings_screen.h>
#include <alice/hollywood/library/combinators/combinators/centaur/teasers/set_teaser_settings.h>

#include <apphost/lib/proto_answers/http.pb.h>
#include <google/protobuf/wrappers.pb.h>

using namespace NAlice::NScenarios;


namespace NAlice::NHollywood::NCombinators {


using DirectivePointer = google::protobuf::internal::RepeatedPtrIterator<const TDirective>;

TCentaurTeasersCombinator::TCentaurTeasersCombinator(THwServiceContext& ctx) 
    : Ctx(ctx)
    , CombinatorRequest(Ctx.GetProtoOrThrow<TCombinatorRequest>(AH_ITEM_COMBINATOR_REQUEST_NAME))
    , Request(TCombinatorRequestWrapper(CombinatorRequest, Ctx.Logger(), Ctx))
{
}

void TCentaurTeasersCombinator::Run() {
    TCombinatorResponse resp;
    // TODO process availiable scenarios here and fill arguments
    google::protobuf::StringValue args;
    resp.MutableResponse()->MutableContinueArguments()->PackFrom(std::move(args));
    LOG_INFO(Ctx.Logger()) << "Returning continue arguments";
    Ctx.AddProtobufItemToApphostContext(resp, AH_ITEM_COMBINATOR_RESPONSE_NAME);
}

void TCentaurTeasersCombinator::Continue() {
    if (const auto collectCardsSemanticFrame = Request.Input().FindSemanticFrame(COLLECT_CARDS_FRAME_NAME)) {
        TPrepareTeasers{Ctx, Request}.Do();
    } else if (const auto setTeaserConfigurationSemanticFrame = Request.Input().FindSemanticFrame(COLLECT_TEASERS_PREVIEW_FRAME_NAME)) {
        TPrepareTeaserSettingsScreen{Ctx, Request}.Do();
    } else if (const auto setTeaserConfigurationSemanticFrame = Request.Input().FindSemanticFrame(SET_TEASER_CONFIGURATION_FRAME_NAME)) {
        TSetTeaserSettings{Ctx, Request}.Do(*setTeaserConfigurationSemanticFrame);
    } else {
        LOG_ERROR(Ctx.Logger()) << "Semantic frame not supported";
    }
}

void TCentaurTeasersCombinator::Finalize() {
    LOG_INFO(Ctx.Logger()) << "Centaur Teasers combinator starts finalize stage";
    const auto renderedResponse = Ctx.GetProtoOrThrow<TScenarioRunResponse>(RESPONSE_ITEM);
    const auto usedScenarios = Ctx.GetMaybeProto<TCombinatorUsedScenarios>(COMBINATOR_USED_SCENARIOS_ITEM);
    const auto combinatorRequest = Ctx.GetProtoOrThrow<TCombinatorRequest>(AH_ITEM_COMBINATOR_REQUEST_NAME);

    const auto request = TCombinatorRequestWrapper(combinatorRequest, Ctx.Logger(), Ctx);
    TCombinatorRunResponseBuilder builder{};

    if (usedScenarios.Defined()) {
        for (const auto& usedScenario : usedScenarios->GetScenarioNames()) {
            builder.AddUsedScenario(usedScenario);
        }
    } else {
        LOG_WARNING(Ctx.Logger()) << "No used_scenarios item for combinator finalization";
    }

    TCombinatorAnalyticsInfoBuilder analyticsInfoBuilder{request};
    analyticsInfoBuilder.SetCombinatorProductName(CENTAUR_TEASERS_COMBINATOR_PSN);

    builder.AnalyticsInfo(std::move(analyticsInfoBuilder).MoveProto());
    builder.ScenarioResponse(renderedResponse);

    Ctx.AddProtobufItemToApphostContext(std::move(builder).MoveProto(), AH_ITEM_COMBINATOR_RESPONSE_NAME);
}


void TCentaurCombinatorRunHandle::Do(THwServiceContext& ctx) const {
    TCentaurTeasersCombinator{ctx}.Run();
}

void TCentaurCombinatorContinueHandle::Do(THwServiceContext& ctx) const {
    TCentaurTeasersCombinator{ctx}.Continue();
}

void TCentaurCombinatorFinalizeHandle::Do(THwServiceContext& ctx) const {
    TCentaurTeasersCombinator{ctx}.Finalize();
}

} // namespace NAlice::NHollywood::NCombinators
