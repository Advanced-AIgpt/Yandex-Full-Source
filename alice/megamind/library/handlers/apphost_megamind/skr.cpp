#include "skr.h"

#include "blackbox.h"
#include "combinators.h"
#include "continue_setup.h"
#include "fallback_response.h"
#include "misspell.h"
#include "node.h"
#include "on_utterance.h"
#include "personal_intents.h"
#include "polyglot.h"
#include "postpone_log_writer.h"
#include "speechkit_session.h"
#include "stage_timers.h"
#include "util.h"
#include "walker.h"

#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/apphost_request/util.h>
#include <alice/megamind/library/apphost_request/protos/client.pb.h>
#include <alice/megamind/library/apphost_request/protos/fake_item.pb.h>
#include <alice/megamind/library/apphost_request/protos/uniproxy_request.pb.h>
#include <alice/megamind/library/handlers/apphost_megamind/grpc/register.h>
#include <alice/megamind/library/request_composite/composite.h>
#include <alice/megamind/library/request_composite/event.h>
#include <alice/megamind/library/request_composite/client/client.h>
#include <alice/megamind/library/requestctx/common.h>
#include <alice/megamind/library/requestctx/rtlogtoken.h>
#include <alice/megamind/library/speechkit/request.h>
#include <alice/megamind/library/speechkit/request_build.h>
#include <alice/megamind/library/speechkit/request_parser.h>
#include <alice/megamind/library/util/status.h>
#include <alice/megamind/library/worldwide/language/is_alice_worldwide_language.h>

#include <alice/library/asr_hypothesis_picker/asr_hypothesis_picker.h>
#include <alice/library/experiments/experiments.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/logger/logger.h>
#include <alice/library/logger/rtlog_adapter.h>
#include <alice/library/metrics/names.h>
#include <alice/library/metrics/sensors.h>
#include <alice/library/network/common.h>
#include <alice/library/proto/protobuf.h>

#include <apphost/lib/common/constants.h>
#include <apphost/lib/proto_answers/http.pb.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/http/io/headers.h>
#include <library/cpp/logger/priority.h>
#include <library/cpp/json/json_value.h>

#include <util/datetime/base.h>
#include <util/string/builder.h>
#include <util/string/join.h>

using namespace NAlice::NMegamindAppHost;

namespace NAlice::NMegamind {
namespace {

const TString ALICE_LOGGER_OPTIONS_HEADER_NAME = "X-Alice-Logger-Options";

void AddLoggerOptionsToContext(TItemProxyAdapter& itemAdapter, TRTLogger& logger, const THttpHeaders& headers,
    const bool fullLoggingEnabledByExp) {

    TMaybe<NAlice::TLoggerOptions> loggerOptions;
    if (const auto* loggerOptionsHeader = headers.FindHeader(ALICE_LOGGER_OPTIONS_HEADER_NAME)) {
        try {
            NAlice::TLoggerOptions loggerOptionsFromRequest;
            ProtoFromBase64String(loggerOptionsHeader->Value(), loggerOptionsFromRequest);
            loggerOptions = std::move(loggerOptionsFromRequest);
        } catch (...) {
            LOG_ERROR(logger) << "Unable to parse logger options item, base64 string: "
                              << loggerOptionsHeader->Value();
        }
    } else {
        LOG_DEBUG(logger) << "Unable to find " << ALICE_LOGGER_OPTIONS_HEADER_NAME << " header";
    }
    if (fullLoggingEnabledByExp) {
        if (!loggerOptions) {
            loggerOptions.ConstructInPlace();
        }
        loggerOptions->SetSetraceLogLevel(NAlice::ELogLevel::ELL_DEBUG);
    }

    if (loggerOptions) {
        itemAdapter.PutIntoContext(*loggerOptions, AH_ITEM_LOGGER_OPTIONS);
    }
}

class TSetupEventComponent : public TEventComponent {
public:
    static TErrorOr<TSetupEventComponent> Create(TSpeechKitInitContext& ctx) {
        return TSetupEventComponent{*ctx.EventProtoPtr};
    };

    const TEventProto& Event() const override {
        return EventProto_;
    }

    TEventWrapper EventWrapper() const override {
        return IEvent::CreateEvent(Event()).release();
    }

private:
    TSetupEventComponent(const TEventProto& eventProto)
        : EventProto_{eventProto}
    {
    }

private:
    std::reference_wrapper<const TEventProto> EventProto_;
};

class TSetupClientComponent : public TClientComponent {
public:
    static TErrorOr<TSetupClientComponent> Create(TSpeechKitInitContext& initCtx) {
        Y_ASSERT(initCtx.Proto);

        const auto& options = initCtx.Proto->GetRequest().GetAdditionalOptions();

        TMaybe<TString> clientIp;
        TMaybe<TString> authToken;
        if (const TString& value = options.GetOAuthToken(); !value.Empty()) {
            authToken.ConstructInPlace(value);
        }
        if (const TString& value = options.GetBassOptions().GetClientIP(); !value.Empty()) {
            clientIp.ConstructInPlace(value);
        }

        return TSetupClientComponent{*initCtx.Proto, std::move(clientIp), std::move(authToken)};
    }

    const TString* ClientIp() const override {
        return ClientIp_.Get();
    }

    const TString* AuthToken() const override {
        return AuthToken_.Get();
    }

    const TClientFeatures& ClientFeatures() const override {
        return ClientFeatures_;
    }

    const TExpFlags& ExpFlags() const override {
        return ExpFlags_;
    }

    const TDeviceState& DeviceState() const override {
        return DeviceState_;
    }

private:
    TSetupClientComponent(const TSpeechKitRequestProto& proto, TMaybe<TString>&& clientIp, TMaybe<TString>&& authToken)
        : ClientIp_{std::move(clientIp)}
        , AuthToken_{std::move(authToken)}
        , ExpFlags_{CreateExpFlags(proto.GetRequest().GetExperiments())}
        , ClientFeatures_{proto.GetApplication(), ExpFlags_}
        , DeviceState_{proto.GetRequest().GetDeviceState()}
    {
    }

    TMaybe<TString> ClientIp_;
    TMaybe<TString> AuthToken_;
    TExpFlags ExpFlags_;
    TClientFeatures ClientFeatures_;
    TDeviceState DeviceState_;
};

class TSetupAppHostComposite : public TRequestComposite<TRequestParts, TSetupEventComponent, TSetupClientComponent> {
public:
    TSetupAppHostComposite(TSpeechKitInitContext& initCtx, TStatus& status)
        : TRequestComposite<TRequestParts, TSetupEventComponent, TSetupClientComponent>{initCtx, status}
    {
    }
};

void PrepareUniproxyProto(const TRequestCtx& requestCtx, TUniproxyRequestInfoProto& proto) {
    for (const auto& header : requestCtx.Headers()) {
        auto* headerProto = proto.AddHeaders();
        headerProto->SetName(header.Name());
        headerProto->SetValue(header.Value());
    }

    proto.SetUri(requestCtx.Uri().PrintS());
    proto.SetCgi(requestCtx.Cgi().Print());
}

TMaybe<TString> GetUtteranceTranslationLanguagePair(const IContext& context) {
    if (context.HasExpFlag(EXP_DISABLE_REQUEST_TRANSLATION)) {
        return Nothing();
    }
    if (const auto languagePair = GetExperimentValueWithPrefix(context.ExpFlags(), EXP_REQUEST_TRANSLATION_PREFIX)) {
        return TString(languagePair.GetRef());
    }
    const auto srcLanguage = context.Language();
    const auto dstLanguage = ConvertAliceWorldWideLanguageToOrdinar(srcLanguage);
    if (srcLanguage != dstLanguage) {
        return Join('-', IsoNameByLanguage(srcLanguage), IsoNameByLanguage(dstLanguage));
    }
    return Nothing();
}

class TAppHostInitSkrImpl final : public TAppHostInitSkr {
public:
    TAppHostInitSkrImpl(IAppHostCtx& ahCtx)
        : AhCtx_{ahCtx}
    {
    }

protected:
    TStatus OnSuccess(const IContext& ctx) override {
        auto& ahCtx = AhCtx();

        ahCtx.ItemProxyAdapter().PutIntoContext(CreateClientItem(ctx.SpeechKitRequest()), AH_ITEM_SKR_CLIENT_INFO);

        TMaybe<TString> utterance;
        if (auto err = AppHostMisspellSetup(ahCtx, ctx.SpeechKitRequest(), utterance)) {
            return std::move(*err);
        }
        if (auto err = AppHostBlackBoxSetup(ahCtx, ctx.SpeechKitRequest())) {
            return std::move(*err);
        }
        if (auto err = AppHostPersonalIntentsSetup(ahCtx, ctx.SpeechKitRequest())) {
            return std::move(*err);
        }

        if (const auto languagePair = GetUtteranceTranslationLanguagePair(ctx)) {
            if (auto err = AppHostPolyglotSetup(ahCtx, ctx.SpeechKitRequest(), utterance, *languagePair)) {
                return std::move(*err);
            }
        }

        if (utterance.Defined() && !utterance->Empty()) {
            if (auto err = AppHostOnUtteranceReadySetup(ahCtx, *utterance, ctx)) {
                return std::move(*err);
            }
        }

        return Success();
    }

    TSpeechKitRequest CreateSkr(TSpeechKitInitContext& initCtx) override {
        TStatus error;
        Composite_.ConstructInPlace(initCtx, error);
        if (error) {
            ythrow TSkrCreationError(std::move(*error));
        }
        return TSpeechKitRequest{*Composite_};
    }

    IAppHostCtx& AhCtx() override {
        return AhCtx_;
    }

private:
    static TClientItem CreateClientItem(const TSpeechKitRequest& skr) {
        TClientItem clientItem;

        // TODO (petrk) Write a serialization function in TClientComponent.
        if (const auto* token = skr.AuthToken()) {
            clientItem.SetAuthToken(*token);
        }
        if (const auto* clientIp = skr.ClientIp()) {
            clientItem.SetClientIp(*clientIp);
        }
        *clientItem.MutableClientInfo() = skr.ClientInfoProto();
        *clientItem.MutableExperiments() = skr.ExperimentsProto();
        *clientItem.MutableDeviceState() = skr.Proto().GetRequest().GetDeviceState();
        *clientItem.MutableSupportedFeatures() = skr.Proto().GetRequest().GetAdditionalOptions().GetSupportedFeatures();
        *clientItem.MutableUnsupportedFeatures() = skr.Proto().GetRequest().GetAdditionalOptions().GetUnsupportedFeatures();

        return clientItem;
    }

private:
    IAppHostCtx& AhCtx_;
    TMaybe<TSetupAppHostComposite> Composite_;
};

class TSkrAppHostRequestCtx : public TAppHostRequestCtx {
public:
    TSkrAppHostRequestCtx(IAppHostCtx& ahCtx, NAppHostHttp::THttpRequest& httpRequestProto)
        : TAppHostRequestCtx{ahCtx, ConstructInitializer(ahCtx, httpRequestProto)}
        , HttpRequestProto_{httpRequestProto}
    {
    }

    const TString& Body() const override {
        return HttpRequestProto_.GetContent();
    }

private:
    static TInitializer ConstructInitializer(IAppHostCtx& ahCtx, NAppHostHttp::THttpRequest& httpRequestProto) {
        ahCtx.GlobalCtx().ServiceSensors().IncRate(NSignal::APPHOST_REQUEST_INCOMING);

        auto stageTimers = std::make_unique<TStageTimersAppHost>(ahCtx.ItemProxyAdapter());
        stageTimers->Register(NMegamind::TS_STAGE_START_REQUEST);

        if (auto err = GetFirstProtoItem(ahCtx.ItemProxyAdapter(), NAppHost::PROTO_HTTP_REQUEST, httpRequestProto)) {
            ythrow TRequestCtx::TBadRequestException{} << "No item '" << NAppHost::PROTO_HTTP_REQUEST << "' found";
        }

        THttpHeaders headers;
        for (const auto& header : httpRequestProto.GetHeaders()) {
            headers.AddHeader(header.GetName(), header.GetValue());
        }

        TStringBuf path{httpRequestProto.GetPath()};
        path.SkipPrefix("/megamind"); // TODO (petrk@) rewrite it. Maybe s/^.*\(/speechkit/app.*)$//.

        TCgiParameters cgi;
        NUri::TUri uri;
        if (!NNetwork::TryParseUri(path, uri, cgi)) {
            ythrow TRequestCtx::TBadRequestException{} << "Unable to parse request uri from " << path;
        }

        return TInitializer{ahCtx, std::move(uri), std::move(cgi), std::move(headers), std::move(stageTimers)};
    }

private:
    NAppHostHttp::THttpRequest& HttpRequestProto_;
};

void CreateRequiredNodeMeta(IAppHostCtx& ahCtx, const TSpeechKitRequestProto& proto, const TEventComponent::TEventProto& event) {
    NMegamindAppHost::TRequiredNodeMeta nodeMeta;
    nodeMeta.SetSpeechKitRequestId(proto.GetHeader().GetRequestId());
    nodeMeta.SetHypothesisNumber(event.GetHypothesisNumber());
    nodeMeta.SetEndOfUtterance(event.GetEndOfUtterance());
    ahCtx.ItemProxyAdapter().PutIntoContext(nodeMeta, AH_ITEM_REQUIRED_NODE_META);

    TAppHostNodeHandler::UpdateLoggerRequestId(ToString(ahCtx.ItemProxyAdapter().NodeLocation().Path), nodeMeta, ahCtx.Log());
}

void AddFakeItemToContext(TItemProxyAdapter& ipa) {
    NMegamindAppHost::TFakeItem fakeItem;
    fakeItem.SetDummy(true);
    ipa.PutIntoContext(fakeItem, AH_ITEM_FAKE_ITEM);
}

void AddProcessIdMetric(const TString& processId, NMetrics::ISensors& sensors) {
    if (processId.empty()) {
        return;
    }
    NMonitoring::TLabels labels;
    labels.Add(NSignal::HOST, "");
    labels.Add(NSignal::NAME, NSignal::STACK_REQUESTS_PER_SECOND);
    labels.Add(NSignal::PROCESS_ID, processId);
    sensors.IncRate(labels);
}

} // namespace

void RegisterAppHostMegamindHandlers(IGlobalCtx& globalCtx, TRegistry& registry) {
    static const TAppHostSkrNodeHandler skrHandler{globalCtx, /* useAppHostStreaming= */ false};
    registry.Add("/speechkit_request", [](NAppHost::IServiceContext& ctx) { skrHandler.RunSync(ctx); });

    RegisterAppHostUtteranceHandlers(globalCtx, registry);
    RegisterAppHostWalkerHandlers(globalCtx, registry);
    RegisterCombinatorHandlers(globalCtx, registry);
    RegisterPostponeLogWriterHander(globalCtx, registry);
    RegisterContinueSetupHandler(globalCtx, registry);
    NRpc::RegisterRpcHandlers(globalCtx, registry);
    TAppHostFallbackResponseNodeHandler::Register(globalCtx, registry);
}

// TAppHostSkrNodeHandler ------------------------------------------------------
TStatus TAppHostSkrNodeHandler::Execute(IAppHostCtx& ahCtx) const {
    NAppHostHttp::THttpRequest httpRequestProto;
    TSkrAppHostRequestCtx requestCtx{ahCtx, httpRequestProto};
    return TAppHostInitSkrImpl{ahCtx}.ParseHttp(requestCtx);
}

TRTLogger TAppHostSkrNodeHandler::CreateLogger(NAppHost::IServiceContext& ctx) const {
    NAppHostHttp::THttpRequest httpRequest;
    const auto items = ctx.GetProtobufItemRefs(NAppHost::PROTO_HTTP_REQUEST);
    if (items.empty() || !items.front().Fill(&httpRequest)) {
        return GlobalCtx.RTLogger("");
    }

    TRTLoggerTokenConstructor rtLogToken(ToString(ctx.GetRUID()));
    TMaybe<ELogPriority> logPriorityFromRequest;
    for (const auto& h : httpRequest.GetHeaders()) {
        rtLogToken.CheckHeader(h.GetName(), h.GetValue());
        if (h.GetName() == ALICE_LOGGER_OPTIONS_HEADER_NAME) {
            NAlice::TLoggerOptions proto;
            try {
                ProtoFromBase64String(h.GetValue(), proto);
                const auto logLevel = MapUniproxyLogLevelToMegamindLogLevel(proto.GetSetraceLogLevel());
                if (logLevel.IsSuccess()) {
                    logPriorityFromRequest = logLevel.Value();
                }
            } catch (...) {
                Cerr << "Unable to parse logger options item, base64 string: " << h.GetValue();
            }
        }
    }

    return GlobalCtx.RTLogger(rtLogToken.GetToken(), /* session= */ false, logPriorityFromRequest);
}

// TAppHostInitSkr -------------------------------------------------------------
TAppHostInitSkr::TSkrCreationError::TSkrCreationError(TError&& error)
    : Error{std::move(error)}
{
}

TUsefulContextForReranker CollectContextUsefulForReranker(TSpeechKitInitContext& initCtx) {
    const auto clientInfo = TClientInfo(initCtx.Proto->GetApplication());

    TIoTUserInfo ioTUserInfo;
    ProtoFromBase64String(initCtx.Proto->GetIoTUserInfoData(), ioTUserInfo);
    TUsefulContextForReranker::TIoTInfo ioTInfo;
    for (const auto& scenario : ioTUserInfo.GetScenarios()) {
        ioTInfo.IoTScenariosNamesAndTriggers.push_back(scenario.GetName());
        for (const auto& trigger : scenario.GetTriggers()) {
            if (trigger.GetType() == TIoTUserInfo_TScenario_TTrigger_ETriggerType_VoiceScenarioTriggerType) {
                ioTInfo.IoTScenariosNamesAndTriggers.push_back(trigger.GetVoiceTriggerPhrase());
            }
        }
    }

    const auto& deviceState = initCtx.Proto->GetRequest().GetDeviceState();
    const bool musicIsOn =
        (deviceState.GetMusic().GetPlayer().HasPause() && !deviceState.GetMusic().GetPlayer().GetPause()) ||
        (deviceState.GetBluetooth().GetPlayer().HasPause() && !deviceState.GetBluetooth().GetPlayer().GetPause()) ||
         deviceState.GetAudioPlayer().GetPlayerState() == TDeviceState_TAudioPlayer_TPlayerState_Playing;
    TUsefulContextForReranker::TPlayersState playersState{
        .MusicIsOn = musicIsOn,
        .RadioIsOn = false,
        .TimerIsOn = AnyOf(deviceState.GetTimers().GetActiveTimers(),
                           [](const auto& timer) { return !timer.GetPaused(); }),
        .VideoIsOn = deviceState.GetVideo().GetCurrentlyPlaying().HasPaused() &&
                     !deviceState.GetVideo().GetCurrentlyPlaying().GetPaused(),
        .AlarmIsOn = deviceState.GetAlarmState().GetCurrentlyPlaying(),
    };
    if (const auto& radio = deviceState.GetRadio(); radio.fields().contains("player")) {
        if (const auto radioPlayer = radio.fields().at("player").struct_value(); radioPlayer.fields().contains("pause")) {
            playersState.RadioIsOn = !radioPlayer.fields().at("pause").bool_value();
        }
    }

    TUsefulContextForReranker contextForReranker{
        .ClientInfo = std::move(clientInfo),
        .IoTInfo = std::move(ioTInfo),
        .PlayerState = std::move(playersState),
    };
    return contextForReranker;
}

TVector<TAsrHypothesisWideWords> CollectAsrHypothesesWords(TSpeechKitInitContext& initCtx) {
    TVector<TAsrHypothesisWideWords> asrHypothesesWords;
    for (const auto& asrHypothesisProto : initCtx.EventProtoPtr->GetAsrResult()) {
        auto& hypothesisWords = asrHypothesesWords.emplace_back();
        for (const auto& word : asrHypothesisProto.GetWords()) {
            hypothesisWords.push_back(UTF8ToWide(word.GetValue()));
        }
    }
    return asrHypothesesWords;
}

void RerankAsrHypotheses(TSpeechKitInitContext& initCtx, TRTLogger& logger) {
    const bool disableAsrHypothesesReranking =
            initCtx.Proto->GetRequest().GetExperiments().GetStorage().contains(TString{EXP_DISABLE_ASR_HYPOTHESES_RERANKING});
    if (initCtx.EventProtoPtr->AsrResultSize() && !disableAsrHypothesesReranking) {
        const auto contextForReranker = CollectContextUsefulForReranker(initCtx);
        const auto asrHypothesesWords = CollectAsrHypothesesWords(initCtx);

        TRTLogAdapter adapter{logger};
        const auto bestHypothesisId = PickBestHypothesis(asrHypothesesWords, contextForReranker, &adapter);
        initCtx.EventProtoPtr->SetOriginalZeroAsrHypothesisIndex(bestHypothesisId);
        if (bestHypothesisId) {
            initCtx.EventProtoPtr->MutableAsrResult(0)->Swap(initCtx.EventProtoPtr->MutableAsrResult(bestHypothesisId));

            auto& zeroHypo = *initCtx.EventProtoPtr->MutableAsrResult(0);
            TVector<TString> zeroHypoWords;
            for (const auto& word : zeroHypo.GetWords()) {
                zeroHypoWords.push_back(word.GetValue());
            }
            *zeroHypo.MutableUtterance() = JoinSeq(" ", zeroHypoWords);
            *zeroHypo.MutableNormalized() = JoinSeq(" ", zeroHypoWords);
        }
    }
}

TStatus TAppHostInitSkr::ParseHttp(TRequestCtx& requestCtx) {
    auto& ahCtx = AhCtx();

    TUniproxyRequestInfoProto uniproxyItemRequest;
    PrepareUniproxyProto(requestCtx, uniproxyItemRequest);

    auto& globalCtx = ahCtx.GlobalCtx();
    TSpeechKitInitContext initCtx{requestCtx.Cgi(), requestCtx.Headers(), uniproxyItemRequest.GetUri(), globalCtx.RngSeedSalt()};

    if (const auto err = ParseSkRequest(requestCtx, initCtx)) {
        return std::move(*err);
    }

    RerankAsrHypotheses(initCtx, ahCtx.Log());

    if (!initCtx.EventProtoPtr->HasType()) {
        ythrow TRequestCtx::TBadRequestException{} << "No event type in request: (" << initCtx.EventProtoPtr->Utf8DebugString() << ')';
    }

    auto& itemProxyAdapter = ahCtx.ItemProxyAdapter();

    AddFakeItemToContext(itemProxyAdapter);

    // Create node meta and also update logger request id.
    CreateRequiredNodeMeta(ahCtx, *initCtx.Proto, *initCtx.EventProtoPtr);

    // Add logline into AppHost eventlog.
    LogSkrInfo(ahCtx.ItemProxyAdapter(), initCtx.Proto->GetApplication().GetUuid(), initCtx.Proto->GetHeader().GetRefMessageId(), initCtx.EventProtoPtr->GetHypothesisNumber());

    itemProxyAdapter.PutIntoContext(*initCtx.EventProtoPtr, AH_ITEM_SKR_EVENT);
    itemProxyAdapter.PutIntoContext(*initCtx.Proto, AH_ITEM_SPEECHKIT_REQUEST);

    const TContext ctx{CreateSkr(initCtx), /* responses= */{}, requestCtx};

    AddProcessIdMetric(ctx.SpeechKitRequest()->GetRequest().GetAdditionalOptions().GetBassOptions().GetProcessId(), ctx.Sensors());

    uniproxyItemRequest.SetClientName(ctx.SpeechKitRequest().ClientInfo().Name);
    itemProxyAdapter.PutIntoContext(uniproxyItemRequest, AH_ITEM_UNIPROXY_REQUEST);

    bool fullLoggingEnabledByExp = false;
    for (const auto& [exp, _] : ctx.SpeechKitRequest().ExperimentsProto().GetStorage()) {
        itemProxyAdapter.AddFlag(exp);
        fullLoggingEnabledByExp |= exp == EXP_ENABLE_FULL_RTLOG;
    }
    AddLoggerOptionsToContext(itemProxyAdapter, requestCtx.RTLogger(), initCtx.Headers, fullLoggingEnabledByExp);
    if (IsAliceWorldWideLanguage(ctx.Language())) {
        itemProxyAdapter.AddFlag(AH_ITEM_IS_ALICE_WORLDWIDE_FLAG);
    }

    const auto* session = ctx.Session();
    if (session != nullptr) {
        AppHostSpeechKitSessionSetup(ahCtx, session->Proto());
    }

    return OnSuccess(ctx);
}

} // namespace NAlice::NMegamind
