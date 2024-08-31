#include "zen_search.h"

#include <alice/hollywood/library/context/context.h>
#include <alice/hollywood/library/frame/callback.h>
#include <alice/hollywood/library/frame/frame.h>
#include <alice/hollywood/library/frame/slot.h>
#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/resources/resources.h>
#include <alice/hollywood/library/response/response_builder.h>

#include "alice/megamind/protos/common/frame.pb.h"

#include "util/generic/maybe.h"

#include <alice/library/proto/proto.h>

#include <util/generic/variant.h>
#include <util/string/cast.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood {

namespace {

constexpr TStringBuf SEARCH_SLOT = "search";

constexpr TStringBuf SEARCH_FRAME = "alice.zen_search";
constexpr TStringBuf CONTEXT_FRAME = "alice.zen_context_search";
constexpr TStringBuf SEARCH_START_FRAME = "alice.zen_context_search_start";

constexpr TStringBuf DIPLINK_PPEFIX = "zen://open_feed?";
constexpr TStringBuf DIPLINK_ZENF = "zenf";
constexpr TStringBuf DIPLINK_ZENF_VALUE = "alice";
constexpr TStringBuf DIPLINK_EXPORT_PARAMS = "export_params";
constexpr TStringBuf DIPLINK_SCROLL = "scroll_to_zen";
constexpr TStringBuf DIPLINK_SCROLL_VALUE = "1";
constexpr TStringBuf DIPLINK_SEARCH = "alice_search";

const TString FRAME_NAME = "alice.zen_context_search";

const TString& GetSlotValue(const TFrame& frame, const TStringBuf slotName){
    const auto slot = frame.FindSlot(slotName);

    return slot->Value.AsString();
}

void FillStartResponse(const TNlgData& nlgData,  TResponseBodyBuilder& bodyBuilder) {

    auto& analyticsInfo = bodyBuilder.CreateAnalyticsInfoBuilder();
    analyticsInfo.SetProductScenarioName("zen_context_search_start");
    analyticsInfo.SetIntentName("zen_context_search_start");

    bodyBuilder.AddRenderedTextWithButtonsAndVoice("zen_search_result", "start_search", /* buttons = */ {}, nlgData);

    TFrameNluHint nluHint;
    nluHint.SetFrameName(FRAME_NAME);

    bodyBuilder.AddNluHint(std::move(nluHint));
    bodyBuilder.SetExpectsRequest(true);
}

TMaybe<TFrame> GetFrame(const TScenarioInputWrapper& input) {
    for (const auto frameName : {SEARCH_START_FRAME, CONTEXT_FRAME, SEARCH_FRAME}) {
        if (const auto frame = input.FindSemanticFrame(frameName)) {
            return TFrame::FromProto(*frame);
        }
    }
    return Nothing();
}

TString CreateDipLink(const TString& searchQuery) {
    TCgiParameters exportParamsCgi;
    exportParamsCgi.InsertEscaped(DIPLINK_SEARCH, searchQuery);

    TCgiParameters paramsCgi;
    paramsCgi.InsertEscaped(DIPLINK_ZENF, DIPLINK_ZENF_VALUE);
    paramsCgi.InsertEscaped(DIPLINK_EXPORT_PARAMS, exportParamsCgi.Print());
    paramsCgi.InsertEscaped(DIPLINK_SCROLL, DIPLINK_SCROLL_VALUE);

    return TString::Join(DIPLINK_PPEFIX, paramsCgi.Print());
}

} // namespace

void TZenSearchRunHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

    auto nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder builder(&nlgWrapper);
    TNlgData nlgData{ctx.Ctx.Logger(), request};

    const auto frame = GetFrame(request.Input());
    Y_ENSURE(frame.Defined(), "frame is not defined");
    auto& bodyBuilder = builder.CreateResponseBodyBuilder(frame.Get());

    // устройство не сможет открыть ссылку для сценария
    if (!request.Interfaces().GetCanOpenLinkIntent()) {
        LOG_INFO(ctx.Ctx.Logger()) << "Device can't open link";
        bodyBuilder.AddRenderedTextWithButtonsAndVoice("zen_search_unsupported_device", "render_result", /* buttons = */ {}, nlgData);

        auto response = std::move(builder).BuildResponse();
        ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
        return;
    }

    if (frame->Name() == SEARCH_START_FRAME)
    {
        FillStartResponse(nlgData, bodyBuilder);
        auto response = std::move(builder).BuildResponse();
        ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
        return;
    }

    auto& analyticsInfo = bodyBuilder.CreateAnalyticsInfoBuilder();
    analyticsInfo.SetProductScenarioName("zen_search");
    analyticsInfo.SetIntentName("zen_search");

    const auto& search = GetSlotValue(*frame.Get(), SEARCH_SLOT);
    if (!search) {
        LOG_INFO(ctx.Ctx.Logger()) << "Can't find search string";
        bodyBuilder.AddRenderedTextWithButtonsAndVoice("zen_search_empty", "render_result", /* buttons = */ {}, nlgData);

        auto response = std::move(builder).BuildResponse();
        ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
        return;
    }

    bodyBuilder.AddRenderedTextWithButtonsAndVoice("zen_search_result", "open_zen", /* buttons = */ {}, nlgData);

    TDirective directive;
    TOpenUriDirective& openUriDirective = *directive.MutableOpenUriDirective();
    openUriDirective.SetUri(CreateDipLink(search));

    bodyBuilder.AddDirective(std::move(directive));

    auto response = std::move(builder).BuildResponse();
    ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
}

REGISTER_SCENARIO("zen_search",
                  AddHandle<TZenSearchRunHandle>()
                  .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NZenSearch::NNlg::RegisterAll));

} // namespace NAlice::NHollywood
