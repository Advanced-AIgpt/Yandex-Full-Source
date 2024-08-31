#include "render.h"

#include "common.h"

#include <alice/hollywood/library/frame/frame.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/logger/logger.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <library/cpp/iterator/mapped.h>
#include <library/cpp/json/writer/json_value.h>

#include <util/string/builder.h>
#include <util/string/join.h>

#include <memory>
#include <utility>

using namespace NAlice::NScenarios;
using namespace NAlice::NHollywood;

namespace NAlice::NHollywood::NTvChannels {

namespace {

TString ExtractTitle(const NJson::TJsonValue& doc) {
    const auto& titleParts = doc["title"].GetArraySafe();
    return JoinSeq("",
                   MakeMappedRange(titleParts, [](const auto& titlePart) { return titlePart["text"].GetString(); }));
}

NScenarios::TDirective CreateOpenUriDirective(const TString& uri) {
    NScenarios::TDirective directive;

    auto* openUri = directive.MutableOpenUriDirective();
    openUri->SetName("smart_tv_switch_tv_channel");
    openUri->SetUri(uri);

    return directive;
}

std::unique_ptr<TScenarioRunResponse> RenderFailed(TRTLogger& logger, const TFrame& frame,
                                                   const TScenarioHandleContext& ctx,
                                                   const TScenarioRunRequestWrapper& request) {
    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder builder(&nlgWrapper);
    auto& bodyBuilder = builder.CreateResponseBodyBuilder(&frame);
    TNlgData nlgData{logger, request};

    bodyBuilder.AddRenderedTextWithButtonsAndVoice(NLG_TEMPLATE_NAME, "render_failed_result", {}, nlgData);

    return std::move(builder).BuildResponse();
}

TMaybe<TFrame> getFrame(const TScenarioRunRequestWrapper& request) {
    if (request.HasExpFlag(FORM_V2_ENABLED_FLAG)) {
        if (const auto frame = request.Input().TryCreateRequestFrame(NTvChannels::FRAME_V2_NUM); frame.Defined()) {
            return frame;
        }
        else if (const auto frame = request.Input().TryCreateRequestFrame(NTvChannels::FRAME_V2_TEXT); frame.Defined()) {
            return frame;
        }
    }

    if (const auto frame = request.Input().TryCreateRequestFrame(NTvChannels::FRAME); frame.Defined()) {
        return frame;
    }
    return Nothing();
}

std::unique_ptr<TScenarioRunResponse> RenderRun(TRTLogger& logger, const TScenarioHandleContext& ctx,
                                                const TScenarioRunRequestWrapper& request) {
    if (request.BaseRequestProto().GetInterfaces().GetLiveTvScheme()) {
        if (auto frame = getFrame(request)) {
            const TMaybe<NJson::TJsonValue> responseItems = RetireHttpResponseJsonMaybe(ctx);
            NJson::TJsonValue doc;
            if (responseItems.Defined() &&
            responseItems.GetRef().GetValueByPath("response.results.[0].groups.[0].documents.[0]", doc)) {
                const TString url = doc["url"].GetString();
                const TString title = ExtractTitle(doc);
                TString number;
                if (doc["properties"].Has("number")) {
                    number = doc["properties"]["number"].GetString();
                }
                return RenderSwitchTvChannel(logger, frame.GetRef(), url, title, number, ctx, request);
            } else {
                LOG_DEBUG(ctx.Ctx.Logger()) << "Channel was not found";
                if (frame.GetRef().FindSlot("unknownChannel")) {
                    // не нашелся канал .* - наверное это ютюбный канал, а это не к нам
                    LOG_DEBUG(ctx.Ctx.Logger()) << "Request was about unknownChannel";
                    return NTvChannels::RenderIrrelevant(logger, ctx, request);
                }
                return RenderFailed(logger, frame.GetRef(), ctx, request);
            }
        }
    } else if (const auto frame = request.Input().TryCreateRequestFrame(FRAME); frame.Defined()) {
        return NTvChannels::RenderIrrelevant(logger, ctx, request, frame);
    }

    return NTvChannels::RenderIrrelevant(logger, ctx, request);
}

} // namespace

std::unique_ptr<TScenarioRunResponse> RenderSwitchTvChannel(TRTLogger& logger, const TFrame& frame, const TString& url,
                                                            const TString& title, const TString& number,
                                                            const TScenarioHandleContext& ctx,
                                                            const TScenarioRunRequestWrapper& request) {
    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder builder(&nlgWrapper);
    auto& bodyBuilder = builder.CreateResponseBodyBuilder(&frame);

    if (!title.empty()) {
        TNlgData nlgData{logger, request};
        nlgData.Context["channel"] = title;
        bodyBuilder.AddRenderedTextWithButtonsAndVoice(NLG_TEMPLATE_NAME, "render_success_result", {}, nlgData);
    }

    bodyBuilder.AddDirective(CreateOpenUriDirective(url));

    TStringBuilder actionDescription;
    actionDescription << "Переключаем на телеканал '" << title << "'";
    if (!number.empty()) {
        actionDescription << " под номером " << number << ".";
    }

    auto& analyticsInfoBuilder = bodyBuilder.CreateAnalyticsInfoBuilder();
    analyticsInfoBuilder.SetProductScenarioName(NAlice::NProductScenarios::TV_CHANNELS);
    analyticsInfoBuilder.AddAction("switch_tv_channel", "switch tv channel", actionDescription);

    return std::move(builder).BuildResponse();
}

void TSwitchTvChannelRenderHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

    auto response = RenderRun(ctx.Ctx.Logger(), ctx, request);

    ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
}

} // namespace NAlice::NHollywood::NTvChannels
