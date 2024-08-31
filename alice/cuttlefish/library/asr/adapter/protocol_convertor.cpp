#include "protocol_convertor.h"

#include <library/cpp/svnversion/svnversion.h>
#include <util/system/hostname.h>

using namespace NAlice::NAsr;
using namespace NAlice::NAsrAdapter;

void TProtocolConvertor::Convert(const NProtobuf::TInitRequest& initRequest2, YaldiProtobuf::InitRequest& initRequest1) {
    // v2 protocol not support version, so skip initRequest1.SetprotocolVersion();
    if (initRequest2.HasHostName()) {
        initRequest1.SethostName(initRequest2.GetHostName());
    } else {
        TString hostname;
        try {
            hostname = FQDNHostName();
        } catch (...) {
            hostname = "unknown";
        }
        initRequest1.SethostName(hostname);
    }
    if (initRequest2.HasRequestId()) {
        initRequest1.SetrequestId(initRequest2.GetRequestId());
    } else {
        initRequest1.SetrequestId("");
    }
    if (initRequest2.HasUuid()) {
        initRequest1.Setuuid(initRequest2.GetUuid());
    } else {
        initRequest1.Setuuid("");
    }
    if (initRequest2.HasDevice()) {
        initRequest1.Setdevice(initRequest2.GetDevice());
    } else {
        initRequest1.Setdevice("asr_adapter");
    }
    initRequest1.Setcoords("0,0"); // not has data for this field
    if (initRequest2.HasTopic()) {
        initRequest1.Settopic(initRequest2.GetTopic());
    } else {
        initRequest1.Settopic("");
    }
    TString lang = "ru";
    if (initRequest2.HasLang() && initRequest2.GetLang().size()) {
        lang = initRequest2.GetLang();
    }
    initRequest1.set_lang(lang);
    initRequest1.set_samplerate("16000");
    const auto& recognitionOptions = initRequest2.GetRecognitionOptions();
    if (recognitionOptions.HasPunctuation()) {
        initRequest1.set_punctuation(recognitionOptions.GetPunctuation());
    }
    VoiceProxyProtobuf::AdvancedASROptions& advOpts = *initRequest1.mutable_advanced_options();
    advOpts.set_partial_results(true);  // TODO: https://st.yandex-team.ru/VOICESERV-2927#5fa143bda366102097e45170
    bool allowMultiUtterance = true;
    bool enableE2EEou = false;
    if (initRequest2.HasEouMode()) {
        if (initRequest2.GetEouMode() == NProtobuf::MultiUtterance) {
            allowMultiUtterance = true;
        } else if (initRequest2.GetEouMode() == NProtobuf::SingleUtterance) {
            allowMultiUtterance = true;
            enableE2EEou = true;
        } else if (initRequest2.GetEouMode() == NProtobuf::NoEOU) {
            allowMultiUtterance = false;
        } else {
            Y_ASSERT(false && "unsupported EouMode value");
        }
    }
    advOpts.set_allow_multi_utt(allowMultiUtterance);
    advOpts.set_capitalize(recognitionOptions.HasCapitalization() && recognitionOptions.GetCapitalization());
    advOpts.set_manual_punctuation(recognitionOptions.HasManualPunctuation() && recognitionOptions.GetManualPunctuation());
    if (initRequest2.HasMime()) {
        advOpts.set_mime(initRequest2.GetMime());
    }
    // v2 protocol not support early mode, so skip advOpts.set_early_eou_message()
    advOpts.set_enable_e2e_eou(enableE2EEou);
    if (initRequest2.HasHasSpotterPart()) {
        advOpts.set_spotter_validation(initRequest2.GetHasSpotterPart());
    }
    if (recognitionOptions.HasSpotterPhrase()) {
        advOpts.set_spotter_phrase(recognitionOptions.GetSpotterPhrase());
    }
    if (recognitionOptions.HasEmbeddedSpotterInfo()) {
        advOpts.set_embedded_spotter_info(recognitionOptions.GetEmbeddedSpotterInfo());
    }
    // spotter front/back not supported in v2 protocol version
    //if (recognitionOptions.HasSpotterBack()) {
    //    advOpts.set_spotter_back(recognitionOptions.GetSpotterBack());
    //}
    //if (recognitionOptions.HasRequestFront()) {
    //    advOpts.set_request_front(recognitionOptions.GetRequestFront());
    //}
    if (initRequest2.HasAdvancedOptions()) {
        const auto& advancedOptions2 = initRequest2.GetAdvancedOptions();
        if (advancedOptions2.HasOverridePartialUpdatePeriod()) {
            if (advancedOptions2.GetOverridePartialUpdatePeriod() < 100000) {
                advOpts.set_partial_update_period(advancedOptions2.GetOverridePartialUpdatePeriod());
            } else {
                // asr v2 protocol not has field for disable partials, so use hack with huge value in OverridePartialUpdatePeriod
                // recommended by abezhin@
                advOpts.set_partial_results(false);
            }
        }
        if (advancedOptions2.HasDegradationMode()) {
            auto mode = advancedOptions2.GetDegradationMode();
            switch (mode) {
            case NProtobuf::DegradationModeDisable:
                advOpts.set_degradation_mode(VoiceProxyProtobuf::AdvancedASROptions::Disable);
                break;
            case NProtobuf::DegradationModeAuto:
                advOpts.set_degradation_mode(VoiceProxyProtobuf::AdvancedASROptions::Auto);
                break;
            case NProtobuf::DegradationModeEnable:
                advOpts.set_degradation_mode(VoiceProxyProtobuf::AdvancedASROptions::Enable);
                break;
            }
        }

        if (advancedOptions2.HasEnableSuggester()) {
            advOpts.set_enable_suggester(advancedOptions2.GetEnableSuggester());
        }
        if (advancedOptions2.HasIsUserSessionWithRetry()) {
            advOpts.set_is_user_session_with_retry(advancedOptions2.GetIsUserSessionWithRetry());
        }

        if (advancedOptions2.HasMaxSilenceDurationMS()) {
            advOpts.set_max_silence_duration_ms(advancedOptions2.GetMaxSilenceDurationMS());
        }

        if (advancedOptions2.HasInitialMaxSilenceDurationMS()) {
            advOpts.set_initial_wait_duration_ms(advancedOptions2.GetInitialMaxSilenceDurationMS());
        }

        if (advancedOptions2.HasEnableSidespeechDetector()) {
            advOpts.set_enable_sidespeech_detector(advancedOptions2.GetEnableSidespeechDetector());
        }

        if (advancedOptions2.HasEouThreshold()) {
            advOpts.set_eou_threshold(advancedOptions2.GetEouThreshold());
        }
    }
    // v2 protocol not support force process each chunk, so skip advOpts.set_force_process_each_chunk();

    YaldiProtobuf::NormalizerOptions& normalizerOptions = *initRequest1.Mutablenormalizer_options();
    if (recognitionOptions.HasNormalization() || advOpts.manual_punctuation()) {
        normalizerOptions.set_normalize_partials(true);
        if (advOpts.manual_punctuation()) {
            normalizerOptions.set_name("revnorm_manual_punct");
        } else {
            normalizerOptions.set_name("revnorm");
        }
    }
    normalizerOptions.set_lang(lang);
    if (!(recognitionOptions.HasManualPunctuation() && recognitionOptions.GetManualPunctuation())) {
        normalizerOptions.add_banlist("revnorm_manual_punct");
    }
    if (!(recognitionOptions.HasCapitalization() && recognitionOptions.GetCapitalization())) {
        normalizerOptions.add_banlist("punctuation_cvt.capitalize");
    }
    if (!(recognitionOptions.HasAntimat() && recognitionOptions.GetAntimat())) {
        normalizerOptions.add_banlist("reverse_conversion.profanity");
        normalizerOptions.add_banlist("simple_conversions.profanity");
    }
    for (const auto& context2 : recognitionOptions.GetContext()) {
        YaldiProtobuf::Context& context = *initRequest1.add_context();
        context.set_id(context2.GetId());
        for (const auto& trigger : context2.GetTrigger()) {
            context.add_trigger(trigger);
        }
        for (const auto& content : context2.GetContent()) {
            context.add_content(content);
        }
    }
    if (recognitionOptions.HasEmbeddedSpotterInfo()) {
        advOpts.set_embedded_spotter_info(recognitionOptions.GetEmbeddedSpotterInfo());
    }
    // v2 protocol use separate message for close connection so skip initRequest1.setcloseconnection();
    if (initRequest2.HasClientHostname()) {
        initRequest1.set_clienthostname(initRequest2.GetClientHostname());
    }
    // v2 protocol not support cloud options, so skip initRequest1.setadvanced_cloud_options();
    if (initRequest2.HasExperimentsAB()) {
        initRequest1.set_experiments(initRequest2.GetExperimentsAB());
    }

    // VOICESERV-4028 ASR suggester
    if (initRequest2.GetAdvancedOptions().HasEnableSuggester()) {
        if (initRequest2.GetAdvancedOptions().GetEnableSuggester()) {
            initRequest1.mutable_advanced_options()->set_enable_suggester(true);
        }
    }

    if (initRequest2.HasUserInfo()) {
        *initRequest1.mutable_user_info() = initRequest2.GetUserInfo();
    }

    if (initRequest2.HasAppId()) {
        initRequest1.set_appid(initRequest2.GetAppId());
    }
}

void TProtocolConvertor::Convert(const NProtobuf::TAddData& addData2, YaldiProtobuf::AddData& addData1) {
    if (addData2.HasAudioData()) {
        addData1.SetaudioData(addData2.GetAudioData());
    }
    // if (addData2.HasSpotterChunk()) {
    //     legacy use end/begin ogg streams for detect ending spotter & starting main audio, so ignore this option
    // }
    addData1.SetlastChunk(false);
}

void TProtocolConvertor::Convert(const NProtobuf::TEndOfStream&, YaldiProtobuf::AddData& addData1) {
    addData1.SetlastChunk(true);
}

void TProtocolConvertor::Convert(const NProtobuf::TCloseConnection&, YaldiProtobuf::AddData& addData1) {
    addData1.SetcloseConnection(true);
    addData1.SetlastChunk(true);
}

namespace {
    TString ResponseCodeToErrMsg(YaldiProtobuf::ResponseCode responseCode) {
        static const TString badMessageFormatting = "BadMessageFormatting";
        static const TString invalidParams = "InvalidParams";
        static const TString timeout = "Timeout";
        static const TString internalError = "InternalError";
        static const TString unknown = "Unknown";
        switch (responseCode) {
        case YaldiProtobuf::BadMessageFormatting:  // 400
            return badMessageFormatting;
        case YaldiProtobuf::InvalidParams:  // 404
            return invalidParams;
        case YaldiProtobuf::Timeout:  // 408
            return timeout;
        case YaldiProtobuf::InternalError:  // 500
            return internalError;
        default:
            return unknown;
        }
    }
}

void TProtocolConvertor::Convert(const YaldiProtobuf::InitResponse& initResponse1, NProtobuf::TResponse& response) {
    auto& initResponse2 = *response.MutableInitResponse();
    NProtobuf::FillRequiredDefaults(initResponse2);
    if (initResponse1.GetresponseCode() == YaldiProtobuf::OK) {
        initResponse2.SetIsOk(true);
    } else {
        initResponse2.SetIsOk(false);
        initResponse2.SetErrMsg(ResponseCodeToErrMsg(initResponse1.responsecode()));
    }
    if (initResponse1.has_hostname()) {
        initResponse2.SetHostname(initResponse1.hostname());
    }
    if (initResponse1.has_topic()) {
        initResponse2.SetTopic(initResponse1.topic());
    }
    if (initResponse1.has_topic_version()) {
        initResponse2.SetTopicVersion(initResponse1.topic_version());
    }
    TString svnVersion;
    TStringOutput so(svnVersion);
    so << TStringBuf("asr_adapter: ") << GetArcadiaSourceUrl() << '@' << GetArcadiaLastChange();
    initResponse2.SetServerVersion(svnVersion);
}

void TProtocolConvertor::Convert(const YaldiProtobuf::AddDataResponse& addDataResponse1, NProtobuf::TResponse& response) {
    auto& addDataResponse2 = *response.MutableAddDataResponse();
    NProtobuf::FillRequiredDefaults(addDataResponse2);
    if (addDataResponse1.responsecode() == YaldiProtobuf::OK) {
        addDataResponse2.SetIsOk(true);
        if (addDataResponse1.HasendOfUtt() && addDataResponse1.GetendOfUtt()) {
            addDataResponse2.SetResponseStatus(NProtobuf::EndOfUtterance);
        } else {
            addDataResponse2.SetResponseStatus(NProtobuf::Active);
        }
        if (addDataResponse1.has_cache_key()) {
            addDataResponse2.SetCacheKey(addDataResponse1.cache_key());
        }
        if (addDataResponse1.has_do_not_send_to_client() && addDataResponse1.do_not_send_to_client()) {
            addDataResponse2.SetDoNotSendToClient(true);
        }
        if (addDataResponse1.has_whisper_info() && addDataResponse1.whisper_info().has_is_whisper()) {
            addDataResponse2.SetIsWhisper(addDataResponse1.whisper_info().is_whisper());
        }
        // separate validation result not supported in yaldi protocol, so for setting ValidationFailed we need separate code
        // addDataResponse2.SetResponseStatus(NAsrEngineProtocol::ValidationFailed);
        // current yald protocol not contain info about spotter validation process, so skip ValidationInvoked
        // addDataResponse2.SetValidationInvoked(true); // https://st.yandex-team.ru/VOICESERV-2927#5fa137255cc04463d84b925f
        for (auto& result : addDataResponse1.recognition()) {
            auto& hypo = *addDataResponse2.MutableRecognition()->AddHypos();
            NProtobuf::FillRequiredDefaults(hypo);
            if (result.has_normalized()) {
                hypo.SetNormalized(result.normalized());
            }
            if (result.has_parent_model()) {
                hypo.SetParentModel(result.parent_model());
            }
            if (result.has_confidence()) {
                hypo.SetTotalScore(result.confidence());
            } else {
                hypo.SetTotalScore(1.0);
            }
            // not has analogue(end_of_phrase) option in v2 protocol, so skip it
            // if (result.has_end_of_phrase()) // true if we detected an end of phrase, YandexCloud-specific parameter
            for (auto& word : result.words()) {
                if (word.has_value()) {
                    hypo.AddWords(word.value());
                }
            }
        }
        //TODO: repeated VoiceProxyProtobuf.BiometryResult bioResult = 5;
        //TODO: repeated TBiometryResult BioResult = 6;
        if (addDataResponse1.has_core_debug()) {
            addDataResponse2.SetCoreDebug(addDataResponse1.core_debug());
        }
        if (addDataResponse1.has_messagescount()) {
            addDataResponse2.SetMessagesCount(addDataResponse1.messagescount());
        } else {
            addDataResponse2.SetMessagesCount(1);
        }
        if (addDataResponse1.has_duration_processed_audio()) {
            addDataResponse2.SetDurationProcessedAudio(addDataResponse1.duration_processed_audio());
        } else {
            addDataResponse2.SetDurationProcessedAudio(0);
        }
        if (addDataResponse1.has_thrown_partials_fraction()) {
            addDataResponse2.MutableRecognition()->SetThrownPartialsFraction(addDataResponse1.thrown_partials_fraction());
        }
        // v2 protocol not support trash classifactor related fields, so skip it
        // if (addDataResponse1.has_is_trash()) ? https://st.yandex-team.ru/VOICESERV-2927#5fa138155cc04463d84b955a
        // if (addDataResponse1.has_trash_score()) ? https://st.yandex-team.ru/VOICESERV-2927#5fa138155cc04463d84b955a
        for (auto& contextRef1 : addDataResponse1.context_ref()) {
            auto& contextRef2 = *addDataResponse2.MutableRecognition()->AddContextRef();
            contextRef2.SetIndex(contextRef1.index());
            contextRef2.SetContentIndex(contextRef1.content_index());
            contextRef2.SetConfidence(contextRef1.confidence());
        }
        // v2 protocol not contain meta info values, so skip addDataResponse1.has_metainfo()/addDataResponse1.metainfo()
        if (addDataResponse1.has_metainfo()) {
            auto& metainfo = addDataResponse1.metainfo();
            if (metainfo.has_topic()) {
                addDataResponse2.MutableMetaInfo()->SetTopic(metainfo.topic());
            }
            if (metainfo.has_version()) {
                addDataResponse2.MutableMetaInfo()->SetTopicVersion(metainfo.version());
            }
            if (metainfo.has_server_hostname()) {
                addDataResponse2.MutableMetaInfo()->SetHostname(metainfo.server_hostname());
            }
            if (metainfo.has_server_version()) {
                addDataResponse2.MutableMetaInfo()->SetServerVersion(metainfo.server_version());
            }
        }
        return;
    }

    addDataResponse2.SetIsOk(false);
    addDataResponse2.SetErrMsg(ResponseCodeToErrMsg(addDataResponse1.responsecode()));
}
