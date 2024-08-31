#include "asr_recognize.h"
#include "support_functions.h"

#include <alice/cuttlefish/library/experiments/flags_json.h>
#include <alice/cuttlefish/library/logging/dlog.h>

#include <voicetech/library/idl/log/events.ev.pb.h>
#include <voicetech/library/settings_manager/proto/settings.pb.h>
#include <voicetech/library/uniproxy2/unistat.h>

#include <library/cpp/json/writer/json.h>

#include <util/string/builder.h>
#include <util/system/hostname.h>

#include <map>


using namespace NAlice::NAsr;
using namespace NJson;
using namespace NVoicetech::NUniproxy2;

namespace {
    const unsigned long long PARTIALS_UPDATE_PERIOD_IS_NO_PARTIALS = 42424242;
    const TString ahProtoRequest = "asr_proto_request";

    const TString langRu = "ru-RU";
    const TString langUk = "uk-UA";
    const TString langTr = "tr-TR";
    const TString langEn = "en-EN";
    const TString langFr = "fr-FR";
    const TString langDe = "de-DE";
    const TString langEs = "es-ES";
    const TString langIt = "it-IT";
    const TString langAr = "ar-SA";

    // inherit voiceproxy+python_proxy hack/logic for replace client selected lang
    void LegacyHackLang(TString& lang) {
        if (lang.StartsWith(TStringBuf("ru"))) {
            lang = langRu;
            return;
        }

        if (lang.StartsWith(TStringBuf("uk"))) {
            lang = langUk;
            return;
        }

        if (lang.StartsWith(TStringBuf("tr"))) {
            lang = langTr;
            return;
        }

        if (lang.StartsWith(TStringBuf("en-TR"))) {
            lang = langTr;
            return;
        }

        if (lang.StartsWith("en")) {
            lang = langEn;
            return;
        }

        if (lang.StartsWith("fr")) {
            lang = langFr;
            return;
        }

        if (lang.StartsWith("de")) {
            lang = langDe;
            return;
        }

        if (lang.StartsWith("es")) {
            lang = langEs;
            return;
        }

        if (lang.StartsWith("it")) {
            lang = langIt;
            return;
        }

        if (lang.StartsWith("ar")) {
            lang = langAr;
            return;
        }

        lang = langRu;
        return;
    }

    const TString mapsyari = "mapsyari";
    const TString maps = "maps";
    const TString queries = "queries";
    const TString notes = "notes";
    const TString spun = "spun";
    const TString dialog_general = "dialog-general";
    const TString quasar_video = "quasar-video";
    const TString dialog_maps = "dialog-maps";
    const TString market = "market";
    const TString counters = "counters";
    const TString quasar_general = "quasar-general";
    const TString general = "general";
    const TString freeform = "freeform";
    const TString dialogeneralfast = "dialogeneralfast";
    const TString dialogeneral = "dialogeneral";
    const TString dialogmaps = "dialogmaps";
    const TString quasar_general_gpu = "quasar-general-gpu";

    class TTopicsMap {
    public:
        TTopicsMap() {
            Map_[queries] = general;
            Map_[notes] = freeform;
            Map_[spun] = dialogeneralfast;
            Map_[dialog_general] = dialogeneral;
            Map_[quasar_video] = dialogeneral;
            Map_[dialog_maps] = dialogmaps;
            Map_[market] = general;
            Map_[counters] = dialogeneralfast;
            Map_[quasar_general] = quasar_general_gpu;
        }
        void MapTopic(TString& topic) {
            auto it = Map_.find(topic);
            if (it != Map_.end()) {
                topic = it->second;
            }
        }

    private:
        std::map<TString, TString> Map_;
    };

    void ApplyForceLanguageAsrExperiment(const NAliceProtocol::TRequestContext& requestContext, TString& lang, NAlice::NCuttlefish::TLogContext* logContext) {
        // TODO: check experimental flags from ab, not request payload. using cgi 'test-id=12345' not working now
        constexpr TStringBuf forceLanguageAsrPrefix = "force_language_asr=";
        for (const auto& [key, value] : requestContext.GetExpFlags()) {
            if (key.StartsWith(forceLanguageAsrPrefix)) {
                lang = key.substr(forceLanguageAsrPrefix.size());
                if (logContext) {
                    logContext->LogEvent(NEvClass::ApplyAbExperiment(key));
                }
            }
        }
    }

    // inherit voiceproxy+python_proxy hack/logic for replace client selected topic
    void LegacyHackTopic(TString& topic, const TString& lang) {
        if (topic == mapsyari || topic == maps) {
            if (lang == langEn || lang == langFr || lang == langDe || lang == langEs || lang == langIt) {
                topic = maps;
            } else {
                topic = mapsyari;
            }
            return;
        }

        Singleton<TTopicsMap>()->MapTopic(topic);
    }
}

void NAlice::NCuttlefish::NAppHostServices::MessageToAsrInitRequest(
    const TMessage& message,
    NProtobuf::TInitRequest& initRequest,
    const NAliceProtocol::TSessionContext& sessionContext,
    const NAliceProtocol::TRequestContext& requestContext,
    TLogContext* logContext
) {
    static const TString defaultTopic = "dialogeneral";
    static const TString defaultLang = "ru-RU";
    static const TString defaultDevice = "uniproxy";
    static const TString defaultHostname = "unknown";
    TString appId;
    TString uuid;
    TString device;
    if (sessionContext.HasAppId()) {
        appId = sessionContext.GetAppId();
    }
    if (sessionContext.HasUserInfo()) {
        auto& userInfo = sessionContext.GetUserInfo();
        if (userInfo.HasUuid()) {
            uuid = userInfo.GetUuid();
        }
    }
    if (sessionContext.HasDeviceInfo()) {
        auto& deviceInfo = sessionContext.GetDeviceInfo();
        if (deviceInfo.HasDevice()) {
            device = deviceInfo.GetDevice();
        }
    }

    const TMessage::THeader& header = NSupport::GetHeaderOrThrow(message);
    const TJsonValue& payload = NSupport::GetJsonValueByPathOrThrow(message.Json, TStringBuf("event.payload"));

    const TJsonValue* advancedOptions = nullptr;
    if (!payload.GetValuePointer(TStringBuf("advancedASROptions"), &advancedOptions)) {
        if (!payload.GetValuePointer(TStringBuf("advanced_options"), &advancedOptions)) {
            payload.GetValuePointer(TStringBuf("advancedOptions"), &advancedOptions);
        }
    }
    if (advancedOptions && !advancedOptions->IsMap()) {
        ythrow yexception() << "advanced asr options MUST be dict type";
    }

    const TJsonValue* normalizerOptions = nullptr;
    if (!payload.GetValuePointer(TStringBuf("normalizer_options"), &normalizerOptions)) {
        payload.GetValuePointer(TStringBuf("normalizerOptions"), &normalizerOptions);
    }
    if (normalizerOptions && !normalizerOptions->IsMap()) {
        ythrow yexception() << "normalizer options MUST be dict type";
    }

    initRequest.SetAppId(appId);
    {
        TString hostname = defaultHostname;
        try {
            hostname = FQDNHostName();
        } catch (...) {
        }
        initRequest.SetClientHostname(hostname);
        GetString(payload, TStringBuf("hostName"), &hostname);
        initRequest.SetHostName(hostname);
    }
    initRequest.SetRequestId(header.MessageId);
    initRequest.SetUuid(uuid);
    initRequest.SetDevice(device ? device : defaultDevice);
    {
        // here we use 'KILLMEPLS' logic from python uniproxy
        TString lang = defaultLang;
        TString topic = defaultTopic;
        if (sessionContext.HasVoiceOptions()) {
            auto& voiceOptions = sessionContext.GetVoiceOptions();
            if (voiceOptions.HasLang() && voiceOptions.GetLang()) {
                lang = voiceOptions.GetLang();
            }
        }
        GetString(payload, TStringBuf("lang"), &lang);
        ApplyForceLanguageAsrExperiment(requestContext, lang, logContext);
        LegacyHackLang(lang);  // <<<<<<<<<<<<<<<<<<<<<<<<<<<<< hack with replace lang used here
        initRequest.SetLang(lang);
        GetString(payload, TStringBuf("topic"), &topic);
        LegacyHackTopic(topic, lang);  // <<<<<<<<<<<<<<<<<<<<<<<<<<<<< hack with replace topic used here (replace deprecated topics to active & etc)
        initRequest.SetTopic(topic);
    }
    {
        // allow_multi_utt -> MultiUtterance
        // allow_multi_utt + enable_e2e_eou -> SingleUtterance
        // !allow_multi_utt -> NoEOU
        initRequest.SetEouMode(NProtobuf::MultiUtterance);  // disabled eou
        if (advancedOptions) {
            bool allowMultiUtt = true;
            if (!GetBoolean(*advancedOptions, TStringBuf("allow_multi_utt"), &allowMultiUtt)) {
                GetBoolean(*advancedOptions, TStringBuf("allowMultiUtt"), &allowMultiUtt);
            }
            if (allowMultiUtt) {
                bool enableE2EEou = false;
                if (GetBoolean(*advancedOptions, TStringBuf("enable_e2e_eou"), &enableE2EEou) && enableE2EEou) {
                    initRequest.SetEouMode(NProtobuf::SingleUtterance);
                }
            } else {
                initRequest.SetEouMode(NProtobuf::NoEOU);
            }
        }
    }
    {
        TString mime;
        GetString(payload, TStringBuf("mime"), &mime);
        if (!mime) {
            GetString(payload, TStringBuf("format"), &mime);
        }
        initRequest.SetMime(mime);
    }
    {
        bool hasSpotter = false;
        if (!GetBoolean(payload, TStringBuf("enable_spotter_validation"), &hasSpotter)) {
            if (advancedOptions) {
                GetBoolean(*advancedOptions, TStringBuf("spotter_validation"), &hasSpotter);
            }
        }
        initRequest.SetHasSpotterPart(hasSpotter);
    }
    //v2 has not this data: advanced_cloud_options
    try {
        // get AB config for ASR
        // see /uniproxy/library/uaas/__init__.py get_ab_config('ASR') usage
        using namespace NVoice::NExperiments;
        if (auto flagsInfoRef = MakeFlagsConstRefFromSessionContextProto(sessionContext)) {
            TMaybe<TString> asrFlags = flagsInfoRef->GetAsrAbFlagsSerializedJson();
            if (asrFlags.Defined()) {
                initRequest.SetExperimentsAB(std::move(asrFlags.GetRef()));
            }
        }
    } catch (...) {
        // invalid UAAS response format can cause exception, - log this (if can)
        DLOG("fail get AB experiment" << CurrentExceptionMessage());
        if (logContext) {
            logContext->LogEvent(NEvClass::WarningMessage(
                TStringBuilder() << "fail get UaaS experiments for ASR: " << CurrentExceptionMessage()
            ));
        }
    }
    {
        auto& recognitionOptions = *initRequest.MutableRecognitionOptions();
        if (advancedOptions) {
            {
                TString spotterPhrase;
                if (GetString(*advancedOptions, TStringBuf("spotter_phrase"), &spotterPhrase)) {
                    recognitionOptions.SetSpotterPhrase(spotterPhrase);
                }
            }
            {
                bool capitalization = false;
                GetBoolean(*advancedOptions, TStringBuf("capitalize"), &capitalization);
                if (capitalization) {
                    recognitionOptions.SetCapitalization(true);
                }
            }
            {
                bool manualPunctuation = false;
                GetBoolean(*advancedOptions, TStringBuf("manual_punctuation"), &manualPunctuation);
                if (manualPunctuation) {
                    recognitionOptions.SetManualPunctuation(true);
                }
            }
            {
                TString embeddedSpotterInfo;
                if (GetString(*advancedOptions, TStringBuf("embedded_spotter_info"), &embeddedSpotterInfo) && embeddedSpotterInfo) {
                    recognitionOptions.SetEmbeddedSpotterInfo(embeddedSpotterInfo);
                }
            }
            {
                // support old/dirty protocol bugs :((
                TString embeddedSpotterInfo;
                if (GetString(payload, TStringBuf("embedded_spotter_info"), &embeddedSpotterInfo) && embeddedSpotterInfo) {
                    recognitionOptions.SetEmbeddedSpotterInfo(embeddedSpotterInfo);
                }
            }
        }
        {
            //SUPPORT DIRTY OLD LOGIC: also can have spotter_phrase in payload
            // (and it has more priority, than advanced_options.spotter_phrase)
            TString spotterPhrase;
            if (GetString(payload, TStringBuf("spotter_phrase"), &spotterPhrase)) {
                recognitionOptions.SetSpotterPhrase(spotterPhrase);
            }
        }
        {
            bool punctuation = false;
            if (GetBoolean(payload, TStringBuf("punctuation"), &punctuation)) {
                recognitionOptions.SetPunctuation(punctuation);
            }
        }
        {
            bool disableAntimat = false;
            GetBoolean(payload, TStringBuf("disableAntimatNormalizer"), &disableAntimat);
            recognitionOptions.SetAntimat(!disableAntimat);
        }
        {
            const NJson::TJsonValue::TArray* contextList;
            if (GetArrayPointer(payload, TStringBuf("context"), &contextList)) {
                for (auto& ctx: *contextList) {
                    auto& context = *recognitionOptions.AddContext();
                    context.SetId(ctx[TStringBuf("id")].GetStringSafe({}));
                    {
                        const NJson::TJsonValue::TArray* triggersList;
                        if (GetArrayPointer(ctx, "trigger", &triggersList)) {
                            for (auto& trigger : *triggersList) {
                                context.AddTrigger(trigger.GetStringSafe({}));
                            }
                        }
                    }
                    {
                        const NJson::TJsonValue::TArray* contentsList;
                        if (GetArrayPointer(ctx, "content", &contentsList)) {
                            for (auto& content : *contentsList) {
                                context.AddContent(content.GetStringSafe({}));
                            }
                        }
                    }
                }
            }
        }
        {
            bool normalization = bool(normalizerOptions);
            recognitionOptions.SetNormalization(normalization);
        }
    }
    {
        //TODO auto& yabioOptions = initRequest.MutableYabioOptions();
    }
    {
        auto& ao = *initRequest.MutableAdvancedOptions();
        if (advancedOptions) {
            {
                unsigned long long partialUpdatePeriod = 0;
                if (GetUInteger(*advancedOptions, TStringBuf("partial_update_period"), &partialUpdatePeriod) && partialUpdatePeriod) {
                    ao.SetOverridePartialUpdatePeriod(partialUpdatePeriod);
                }
            }
            {
                bool partialResults = true;
                GetBoolean(*advancedOptions, TStringBuf("partial_results"), &partialResults);
                if (!partialResults) {
                    ao.SetOverridePartialUpdatePeriod(PARTIALS_UPDATE_PERIOD_IS_NO_PARTIALS);
                }
            }
            {
                TString s;
                if (GetString(*advancedOptions, TStringBuf("degradation_mode"), &s)) {
                    if (s == TStringBuf("Disable")) {
                        ao.SetDegradationMode(NProtobuf::DegradationModeDisable);
                    } else if (s == TStringBuf("Auto")) {
                        ao.SetDegradationMode(NProtobuf::DegradationModeAuto);
                    } else if (s == TStringBuf("Enable")) {
                        ao.SetDegradationMode(NProtobuf::DegradationModeEnable);
                    } else {
                        ythrow yexception() << "unsupported degradation mode: " << s;
                    }
                } else {
                    if (sessionContext.GetUserInfo().GetUuidKind() == NAliceProtocol::TUserInfo::ROBOT
                        || sessionContext.GetUserInfo().GetUuidKind() == NAliceProtocol::TUserInfo::TEST
                    ) {
                        ao.SetDegradationMode(NProtobuf::DegradationModeDisable);  // we need best & stable results (for QA)
                    } else {
                        ao.SetDegradationMode(NProtobuf::DegradationModeAuto);
                    }
                }
            }
        }

        if (const auto* isUserSessionWithRetry = payload.GetValueByPath("dirty_hacks.is_client_retry")) {
            ao.SetIsUserSessionWithRetry(isUserSessionWithRetry->IsBoolean() && isUserSessionWithRetry->GetBoolean());
        }
    }
    ApplyLocalExperimentsHacks(initRequest, sessionContext, message, logContext);
    ApplyExperiments(initRequest, requestContext, logContext);
    ApplySettings(initRequest, requestContext, logContext);
}

void NAlice::NCuttlefish::NAppHostServices::ApplyLocalExperimentsHacks(
    NProtobuf::TInitRequest& initRequest,
    const NAliceProtocol::TSessionContext& sessionContext,
    const TMessage& message,
    TLogContext* logContext
) {
    //TODO: increment separate metric for each experiment
    const TString authKey = sessionContext.GetAppToken();
    const TString appId = sessionContext.GetAppId();
    bool vinsVoiceInput = false;
    if (auto header = message.Header.Get()) {
        vinsVoiceInput = header->FullName == TMessage::VINS_VOICE_INPUT;
    }

    // quasar-experiments
    if (authKey == "51ae06cc-5c8f-48dc-93ae-7214517679e6") {
        if (initRequest.GetEouMode() == NProtobuf::MultiUtterance) {  // if NoEOU forced we can't redefine it to single utt
            initRequest.SetEouMode(NProtobuf::SingleUtterance);  // enable_e2e_eou
        }
    }

    // place here logic from python_uniproxy local experiments (enforce quasar topic for quasar authKey, etc)
    if (sessionContext.HasUserOptions() && sessionContext.GetUserOptions().GetDisableLocalExperiments()) {
        return;
    }

    ////////////////////// * (ANY_LANG) ////////////////////////
    // capitalize_keyboard KEYBOARD-1548
    if (appId == "ru.yandex.androidkeyboard"
        && !initRequest.GetRecognitionOptions().GetCapitalization()
    ) {
        initRequest.MutableRecognitionOptions()->SetCapitalization(true);
        if (logContext) {
            logContext->LogEvent(NEvClass::ApplyLocalExperimentHack("capitalize_keyboard KEYBOARD-1548"));
        }
    }

    // enforce_antimat VOICESERV-1359
    if (vinsVoiceInput
        && authKey == "cc96633d-59d4-4724-94bd-f5db2f02ad13"
        && !initRequest.GetRecognitionOptions().GetAntimat()
    ) {
        initRequest.MutableRecognitionOptions()->SetAntimat(true);
        if (logContext) {
            logContext->LogEvent(NEvClass::ApplyLocalExperimentHack("enforce_antimat VOICESERV-1359"));
        }
    }

    // NOT_SUPPORT: maps_he2galdi MOBNAVI-12631 (we not has galdy in apphost)

    ////////////////////// RUSSIAN ////////////////////////
    // dialog_general_set_lang_ru_ru overriden by enable-dialog-general-gpu

    // enable-dialog-general-gpu VOICESERV-3080 VOICESERV-2831
    if (vinsVoiceInput
        && initRequest.GetTopic().EndsWith("eneral")
        && (initRequest.GetTopic().StartsWith("dialog") || initRequest.GetTopic().StartsWith("desktop"))
    ) {
        if (logContext) {
            logContext->LogEvent(NEvClass::ApplyLocalExperimentHack("enable-dialog-general-gpu VOICESERV-3080"));
        }
        initRequest.SetLang("ru-RU");
        initRequest.SetTopic("dialog-general-gpu");
        if (initRequest.GetEouMode() == NProtobuf::MultiUtterance) {  // if NoEOU forced we can't redefine it to single utt
            initRequest.SetEouMode(NProtobuf::SingleUtterance);  // enable_e2e_eou
        }
    }

    if (initRequest.GetLang().StartsWith("ru")) {
        // dialogmaps_enable_e2e_prod
        if (initRequest.GetTopic().Contains("maps")) {
            initRequest.SetTopic("dialogmapsgpu");
            if (initRequest.GetEouMode() == NProtobuf::MultiUtterance) {  // if NoEOU forced we can't redefine it to single utt
                initRequest.SetEouMode(NProtobuf::SingleUtterance);  // enable_e2e_eou
            }
            if (logContext) {
                logContext->LogEvent(NEvClass::ApplyLocalExperimentHack("dialogmaps_enable_e2e_prod"));
            }
        }

        // dialogmaps_enable_e2e_prod_autolauncher
        if (vinsVoiceInput && initRequest.GetTopic() == "autolauncher") {
            initRequest.SetTopic("dialogmapsgpu");
            if (initRequest.GetEouMode() == NProtobuf::MultiUtterance) {  // if NoEOU forced we can't redefine it to single utt
                initRequest.SetEouMode(NProtobuf::SingleUtterance);  // enable_e2e_eou
            }
            if (logContext) {
                logContext->LogEvent(NEvClass::ApplyLocalExperimentHack("dialogmaps_enable_e2e_prod_autolauncher"));
            }
        }

        // tv_general_enable_e2e_for_alice
        if (initRequest.GetTopic() == "tv-general-gpu") {
            if (initRequest.GetEouMode() == NProtobuf::MultiUtterance) {  // if NoEOU forced we can't redefine it to single utt
                initRequest.SetEouMode(NProtobuf::SingleUtterance);  // enable_e2e_eou
            }
            if (logContext) {
                logContext->LogEvent(NEvClass::ApplyLocalExperimentHack("tv_general_enable_e2e_for_alice"));
            }
        }
    }

    ////////////////////// TURKISH ////////////////////////
    if (initRequest.GetLang().StartsWith("tr")) {
        // redirect_tr_dialogmaps_to_gpu
        if (initRequest.GetTopic().StartsWith("dialog") && initRequest.GetTopic().EndsWith("maps")) {
            initRequest.SetTopic("dialogmapsgpu");
            if (initRequest.GetEouMode() == NProtobuf::MultiUtterance) {  // if NoEOU forced we can't redefine it to single utt
                initRequest.SetEouMode(NProtobuf::SingleUtterance);  // enable_e2e_eou
            }
            if (logContext) {
                logContext->LogEvent(NEvClass::ApplyLocalExperimentHack("redirect_tr_dialogmaps_to_gpu"));
            }
        }
    }

    ////////////////////// ENGLISH ////////////////////////
    if (initRequest.GetLang().StartsWith("en")) {
        // redirect_en_dialogmaps_to_maps
        if (initRequest.GetTopic().StartsWith("dialog") && initRequest.GetTopic().EndsWith("maps")) {
            initRequest.SetTopic("maps");
            if (logContext) {
                logContext->LogEvent(NEvClass::ApplyLocalExperimentHack("redirect_en_dialogmaps_to_maps"));
            }
        }

        // redirect_en_topics_to_freeform
        if (
            initRequest.GetTopic() == "queries"
            || initRequest.GetTopic() == "general"
            || initRequest.GetTopic() == "notes"
        ) {
            initRequest.SetTopic("freeform");
            if (logContext) {
                logContext->LogEvent(NEvClass::ApplyLocalExperimentHack("redirect_en_topics_to_freeform"));
            }
        }

        // maps_en2ru VOICESERV-935
        if (!vinsVoiceInput
            && initRequest.GetTopic() == "maps"
            && (
                authKey == "27fbd96d-ec5b-4688-a54d-421d81aa8cd2"
                || authKey == "87d9fcbc-602c-43df-becb-772a15340ea2"
                || authKey == "19a8b9d1-bfd3-4243-98f2-7f49db92b379"
                || authKey == "22e2f3a1-a6ee-4dd6-b397-0591f895c37b"
            )
        ) {
            initRequest.SetLang("ru-RU");
            initRequest.SetTopic("dialogmapsgpu");
            if (initRequest.GetEouMode() == NProtobuf::MultiUtterance) {  // if NoEOU forced we can't redefine it to single utt
                initRequest.SetEouMode(NProtobuf::SingleUtterance);  // enable_e2e_eou
            }
            if (logContext) {
                logContext->LogEvent(NEvClass::ApplyLocalExperimentHack("maps_en2ru VOICESERV-935"));
            }
        }
    }
}

void NAlice::NCuttlefish::NAppHostServices::ApplyExperiments(
    NAlice::NAsr::NProtobuf::TInitRequest& initRequest,
    const NAliceProtocol::TRequestContext& requestContext,
    TLogContext* logContext
) {
    // clone python_unuproxy logic with using self.system.ab_asr_topic (get flag from UAAS)
    constexpr TStringBuf setTopic = "set_topic";
    constexpr TStringBuf setTopicPrefix = "set_topic=";
    for (const auto& [key, value] : requestContext.GetExpFlags()) {
        if (key == setTopic) {
            initRequest.SetTopic(value);
            if (logContext) {
                logContext->LogEvent(NEvClass::ApplyAbExperiment(TStringBuilder() << key << ":" << value));
            }
        } else if (key.StartsWith(setTopicPrefix)) {
            TStringBuf abTopic(key);
            abTopic.Skip(setTopicPrefix.size());
            if (abTopic) {
                initRequest.SetTopic(abTopic.Data());
                if (logContext) {
                    logContext->LogEvent(NEvClass::ApplyAbExperiment(key));
                }
            }
        } else if (key == TStringBuf("enable_e2e_eou")) {
            if (initRequest.GetEouMode() == NProtobuf::MultiUtterance) {  // if NoEOU forced we can't redefine it to single utt
                initRequest.SetEouMode(NProtobuf::SingleUtterance);  // enable_e2e_eou
            }
        } else if (key == TStringBuf("asr_enable_suggester")) {
            initRequest.MutableAdvancedOptions()->SetEnableSuggester(true);
        }
    }
}


void NAlice::NCuttlefish::NAppHostServices::ApplySettings(
    NAlice::NAsr::NProtobuf::TInitRequest& initRequest,
    const NAliceProtocol::TRequestContext& request,
    TLogContext* logContext
) {
    Y_UNUSED(logContext);
    if (request.GetSettingsFromManager().GetEnableSuggester()) {
        initRequest.MutableAdvancedOptions()->SetEnableSuggester(true);
    }
}
