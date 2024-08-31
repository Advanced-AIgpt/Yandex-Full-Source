#include "orders_status.h"

#include <alice/hollywood/library/scenarios/market/orders_status/proto/bass_apply_arguments.pb.h>

#include <alice/hollywood/library/bass_adapter/bass_adapter.h>
#include <alice/hollywood/library/bass_adapter/bass_renderer.h>
#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/request/request.h>

#include <alice/megamind/protos/blackbox/blackbox.pb.h>

#include <alice/library/proto/proto.h>

#include <util/generic/variant.h>
#include <util/string/cast.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NMarket {

namespace {

const TString PRODUCT_SCENARIO_NAME { TStringBuf("market_orders_status") };

TStringBuf ORDERS_STATUS_NLG_TEMPLATE_NAME = "market_orders_status";

TStringBuf USER_LOGGED_IN_FRAME_NAME = "alice.market.common.user_logged_in";
TStringBuf ORDERS_STATUS_FRAME_NAME = "alice.market.orders_status";

struct TFrameInfo {
    TString VinsIntentName;
    TString AnalyticsIntentName;
};

const THashMap<TStringBuf, TFrameInfo> FRAMES {
    {
        ORDERS_STATUS_FRAME_NAME,
        {
            .VinsIntentName = TString {"personal_assistant.scenarios.market_orders_status"},
            .AnalyticsIntentName = TString {"orders_status"},
        }
    },
    {
        USER_LOGGED_IN_FRAME_NAME,
        {
            .VinsIntentName = TString {"personal_assistant.scenarios.market_orders_status__login"},
            .AnalyticsIntentName = TString {"user_logged_in"},
        }
    },
};

const TPtrWrapper<TSemanticFrame> GetSemanticFrame(const TScenarioInputWrapper& requestInput)
{
    if (const auto framePtr = requestInput.FindSemanticFrame(USER_LOGGED_IN_FRAME_NAME)) {
        return framePtr;
    }
    if (const auto framePtr = requestInput.FindSemanticFrame(ORDERS_STATUS_FRAME_NAME)) {
        return framePtr;
    }
    return TPtrWrapper<TSemanticFrame>(nullptr, USER_LOGGED_IN_FRAME_NAME);
}

} // namespace

void TOrdersStatusRunHandle::TImpl::Do()
{
    if (!RequestWrapper().ClientInfo().HasScreen()) {
        LOG_INFO(Logger()) << "Unsupported platform";
        return ReturnIrrelevant();
    }

    const auto framePtr = GetSemanticFrame(RequestWrapper().Input());
    if (!framePtr) {
        LOG_INFO(Logger()) << "Orders status frame not found";
        return ReturnIrrelevant();
    }

    const NAlice::TBlackBoxUserInfo* userInfo = GetUserInfoProto(RequestWrapper());
    if (!userInfo) {
        LOG_INFO(Logger()) << "blackbox data source is required for MarketOrdersStatus";
        return ReturnIrrelevant();
    }

    TRunResponseBuilder builder(&NlgWrapper());

    // save apply args
    TBassApplyArguments applyArgs;
    *applyArgs.MutableRequestInput() = RequestWrapper().Input().Proto();
    *applyArgs.MutableSemanticFrame() = *framePtr;
    *applyArgs.MutableUserInfo() = *userInfo;
    builder.SetApplyArguments(applyArgs);

    AddResponse(std::move(builder));
}

void TOrdersStatusRunHandle::TImpl::ReturnIrrelevant()
{
    TRunResponseBuilder builder(&NlgWrapper());
    SetIrrelevantResponse(builder);

    auto& analyticsInfoBuilder = builder.GetResponseBodyBuilder()->CreateAnalyticsInfoBuilder();
    analyticsInfoBuilder.SetProductScenarioName(PRODUCT_SCENARIO_NAME);
    analyticsInfoBuilder.SetIntentName(FRAMES.at(ORDERS_STATUS_FRAME_NAME).AnalyticsIntentName);

    return AddResponse(std::move(builder));
}

void TOrdersStatusApplyPrepareHandle::TImpl::Do()
{
    const TBassApplyArguments applyArgs = RequestWrapper().UnpackArguments<TBassApplyArguments>();
    const TScenarioInputWrapper input(applyArgs.GetRequestInput());
    const TSemanticFrame& currentFrame = applyArgs.GetSemanticFrame();

    TFrame vinsFrame { FRAMES.at(currentFrame.GetName()).VinsIntentName };
    const TString& uid = applyArgs.GetUserInfo().GetUid();
    vinsFrame.AddSlot(TSlot{"uid", "string", TSlot::TValue(uid)});
    const auto bassRequest = PrepareBassVinsRequest(
        Logger(),
        RequestWrapper(),
        input,
        vinsFrame,
        /* sourceTextProvider = */ nullptr,
        Ctx().RequestMeta,
        /* imageSearch = */ false,
        Ctx().AppHostParams,
        /* forbidWebSearch = */ false,
        /* datasources = */ {},
        /* splitUuid = */ false);
    AddBassRequestItems(Ctx(), bassRequest);
}

void TOrdersStatusApplyRenderHandle::TImpl::Do()
{
    const auto bassResponseBody = RetireBassRequest(Ctx());
    const TBassApplyArguments applyArgs = RequestWrapper().UnpackArguments<TBassApplyArguments>();
    const TSemanticFrame& currentFrame = applyArgs.GetSemanticFrame();

    TApplyResponseBuilder builder(&NlgWrapper());
    TBassResponseRenderer bassRenderer(
        RequestWrapper(),
        RequestWrapper().Input(),
        builder,
        Logger(),
        false /* suggestAutoAction */);
    bassRenderer.Render(
        ORDERS_STATUS_NLG_TEMPLATE_NAME,
        bassResponseBody,
        FRAMES.at(currentFrame.GetName()).AnalyticsIntentName);
    builder.GetResponseBodyBuilder()->
        GetAnalyticsInfoBuilder().SetProductScenarioName(PRODUCT_SCENARIO_NAME);

    TFrameNluHint hint;
    hint.SetFrameName(ToString(USER_LOGGED_IN_FRAME_NAME));
    builder.GetResponseBodyBuilder()->AddNluHint(std::move(hint));

    AddResponse(std::move(builder));
}

REGISTER_SCENARIO(
    "market_orders_status",
    AddHandle<TOrdersStatusRunHandle>()
    .AddHandle<TOrdersStatusApplyPrepareHandle>()
    .AddHandle<TOrdersStatusApplyRenderHandle>()
    .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NMarket::NNlg::RegisterAll));

} // namespace NAlice::NHollywood::NMarket

