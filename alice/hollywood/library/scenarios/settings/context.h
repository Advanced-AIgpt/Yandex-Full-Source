#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>

namespace NAlice::NHollywood {

using namespace NAlice::NScenarios;

struct TSettingsRunContext {
    TSettingsRunContext(const TScenarioHandleContext& ctx)
        : RequestProto(GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM))
        , Request(RequestProto, ctx.ServiceCtx)
        , NlgWrapper(TNlgWrapper::Create(ctx.Ctx.Nlg(), Request, ctx.Rng, ctx.UserLang))
        , Builder(&NlgWrapper)
        , BodyBuilder(Builder.CreateResponseBodyBuilder())
        , Logger(ctx.Ctx.Logger())
        , NlgData(Logger, Request)
    {}

    const TScenarioRunRequest RequestProto;
    const TScenarioRunRequestWrapper Request;
    TNlgWrapper NlgWrapper;
    TRunResponseBuilder Builder;
    TResponseBodyBuilder& BodyBuilder;
    TRTLogger& Logger;
    TNlgData NlgData;
};

} // namespace NAlice::NHollywood
