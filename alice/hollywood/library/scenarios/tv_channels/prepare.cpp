#include "prepare.h"
#include "common.h"
#include "render.h"

#include <alice/hollywood/library/frame/frame.h>
#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <alice/megamind/protos/common/device_state.pb.h>

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/logger/logger.h>
#include <alice/megamind/protos/scenarios/request_meta.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>
#include <alice/nlu/libs/request_normalizer/request_normalizer.h>

#include <library/cpp/iterator/mapped.h>
#include <library/cpp/json/writer/json_value.h>

#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/string/join.h>
#include <util/string/type.h>

#include <memory>
#include <utility>

using namespace NAlice::NScenarios;
using namespace NAlice::NHollywood;

namespace NAlice::NHollywood::NTvChannels {

namespace {

constexpr TStringBuf NUMBER_REQUESTS_EXTENSION = " канал";

TString GetChannel(const TFrame& frame, TString slot_name = "channel") {
    TString channel;
    if (const auto fromPtr = frame.FindSlot(slot_name)) {
        channel = *fromPtr->Value.As<TString>().Get();
    }
    return channel;
}

THttpProxyRequest PrepareSaasRequest(TRTLogger& logger, const TString& deviceId, const TString& channel,
                                     const TRequestMeta& requestMeta) {
    auto normalizedChannel = NNlu::TRequestNormalizer::Normalize(ELanguage::LANG_RUS, channel);
    LOG_DEBUG(logger) << "Normalized channel name:" << normalizedChannel;
    TString text = normalizedChannel;
    if (IsNumber(normalizedChannel)) {
        text += NUMBER_REQUESTS_EXTENSION;
    }

    TStringBuilder builder;
    builder << "%request% << (s_device_id:ffffffff";
    if (!deviceId.empty()) {
        builder << " | s_device_id:" << deviceId;
    }
    builder << ")";

    TCgiParameters params = {std::make_pair("service", "smart_tv_channels"),
                             std::make_pair("kps", "1"),
                             std::make_pair("format", "json"),
                             std::make_pair("numdoc", "1"),
                             std::make_pair("template", builder.c_str()),
                             std::make_pair("text", text)};

    auto path = TString("?") + params.Print();
    auto httpRequest = PrepareHttpRequest(path, requestMeta, logger);
    LOG_INFO(logger) << "Requesting SaaS:" << path;
    return httpRequest;
}
} // namespace

/**
 * Версия хендлера 2.0 - работает на слотах из грамматик tv_channels2_{num,text}
 */
void PrepareHandlerV2(TScenarioHandleContext& ctx) {

    const auto requestProto = GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

    const auto& interfacesProto = request.BaseRequestProto().GetInterfaces();
    if (interfacesProto.GetLiveTvScheme()) {

        bool frameFound = false;
        TString channel = "";

        if (const auto frame = request.Input().TryCreateRequestFrame(FRAME_V2_NUM); frame.Defined()) {
            LOG_INFO(ctx.Ctx.Logger()) << "Found frame: " << FRAME_V2_NUM;
            frameFound = true;
            channel = GetChannel(frame.GetRef(), "numChannel");
            LOG_DEBUG(ctx.Ctx.Logger()) << "numChannel = " << channel;
        }
        else if (const auto frame = request.Input().TryCreateRequestFrame(FRAME_V2_TEXT); frame.Defined()) {
            LOG_INFO(ctx.Ctx.Logger()) << "Found frame: " << FRAME_V2_TEXT;
            frameFound = true;
            channel = GetChannel(frame.GetRef(), "knownChannel");
            LOG_DEBUG(ctx.Ctx.Logger()) << "knownChannel = " << channel;
            if (!channel) {
                channel = GetChannel(frame.GetRef(), "unknownChannel");
                LOG_DEBUG(ctx.Ctx.Logger()) << "unknownChannel = " << channel;
            }
        }

        if (!frameFound) {
            // TODO: remove after AppHost graph change, leave irrelevant only in render stage
            auto response = NTvChannels::RenderIrrelevant(ctx.Ctx.Logger(), ctx, request);
            ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
            return;
        }

        if (!channel.empty()) {
            LOG_DEBUG(ctx.Ctx.Logger()) << "Preparing Saas request";
            const auto &deviceId = requestProto.GetBaseRequest().GetDeviceState().GetDeviceId();
            auto httpRequest = PrepareSaasRequest(ctx.Ctx.Logger(), deviceId, channel, ctx.RequestMeta);
            AddHttpRequestItems(ctx, httpRequest);
            LOG_DEBUG(ctx.Ctx.Logger()) << "Http request successfully added";
        } else {
            LOG_DEBUG(ctx.Ctx.Logger()) << "Empty channel slot";
        }
    } else {
        // TODO: remove after AppHost graph change, leave irrelevant only in render stage
        auto response = NTvChannels::RenderIrrelevant(ctx.Ctx.Logger(), ctx, request);
        ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
    }
}

/**
 * Обработчик для переключения канала через Typed Semantic Frame
 * Такой фрейм приходит, если пользователь использует голосовую кнопку
 * "следующий канал" или "предыдущий канал"
 * Отличается от голосового тем, что uri канала уже пришел, нужно только
 * переключить
 */
void SwitchTvChannelSemanticFrameHandler(
    TScenarioHandleContext& ctx,
    const TScenarioRunRequestWrapper& request,
    const TFrame& frame
) {
    TString channelUri;
    if (const auto fromPtr = frame.FindSlot("uri")) {
        channelUri = *fromPtr->Value.As<TString>().Get();
        const auto response = RenderSwitchTvChannel(
            ctx.Ctx.Logger(),
            frame,
            channelUri,
            "", // title
            "", // number
            ctx,
            request
        );

        LOG_INFO(ctx.Ctx.Logger()) << "returning OpenUriDirective uri=" << channelUri;

        ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);

    } else {
        // не найдено значение uri, куда переключать не понятно
        // это ошибка
    }
}


// prepare SaaS request if needed
void TSwitchTvChannelPrepareHandle::Do(TScenarioHandleContext& ctx) const {

    const auto requestProto = GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

    const auto& interfacesProto = request.BaseRequestProto().GetInterfaces();

    const auto frame = request.Input().TryCreateRequestFrame(FRAME_SWITCH_SF);
    if (frame.Defined()) {
        LOG_INFO(ctx.Ctx.Logger()) << "Found frame: alice.switch_tv_channel_sf";
        SwitchTvChannelSemanticFrameHandler(ctx, request, frame.GetRef());
        return;
    }

    if (request.HasExpFlag(FORM_V2_ENABLED_FLAG)) {
        PrepareHandlerV2(ctx);
        return;
    }

    if (interfacesProto.GetLiveTvScheme()) {
        if (const auto frame = request.Input().TryCreateRequestFrame(NTvChannels::FRAME); frame.Defined()) {
            TString channel = GetChannel(frame.GetRef());
            LOG_INFO(ctx.Ctx.Logger()) << "Found frame: " << FRAME;
            LOG_INFO(ctx.Ctx.Logger()) << "channel = " << channel;

            if (!channel.empty()) {
                LOG_DEBUG(ctx.Ctx.Logger()) << "Preparing Saas request";
                const auto &deviceId = requestProto.GetBaseRequest().GetDeviceState().GetDeviceId();
                auto httpRequest = PrepareSaasRequest(ctx.Ctx.Logger(), deviceId, channel, ctx.RequestMeta);
                AddHttpRequestItems(ctx, httpRequest);
                LOG_DEBUG(ctx.Ctx.Logger()) << "Http request successfully added";
            } else {
                LOG_DEBUG(ctx.Ctx.Logger()) << "Empty 'channel' slot";
            }
        } else {
            // TODO: remove after AppHost graph change, leave irrelevant only in render stage
            auto response = NTvChannels::RenderIrrelevant(ctx.Ctx.Logger(), ctx, request, frame);
            ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
        }
    } else {
        // TODO: remove after AppHost graph change, leave irrelevant only in render stage
        const auto frame = request.Input().TryCreateRequestFrame(NTvChannels::FRAME);
        auto response = NTvChannels::RenderIrrelevant(ctx.Ctx.Logger(), ctx, request, frame);
        ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
    }
}

} // namespace NAlice::NHollywood::NTvChannels
