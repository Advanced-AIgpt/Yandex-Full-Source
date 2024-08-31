#include "bass_adapter.h"

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/request/request.h>

#include <alice/library/client/client_features.h>
#include <alice/library/client/client_info.h>
#include <alice/library/json/json.h>
#include <alice/library/music/defs.h>
#include <alice/library/network/common.h>
#include <alice/library/proto/proto.h>

#include <alice/megamind/protos/common/device_state.pb.h>
#include <alice/megamind/protos/common/events.pb.h>
#include <alice/megamind/protos/common/location.pb.h>

#include <alice/protos/data/language/language.pb.h>

#include <util/string/builder.h>

namespace NAlice::NHollywood {

namespace NImpl {

namespace {

using TLocationScheme = NBASSRequest::TMeta<TSchemeTraits>::TLocation;
using TDeviceStateScheme = NBASSRequest::TMeta<TSchemeTraits>::TDeviceState;
using TBiometricsScoresScheme = NBASSRequest::TMeta<TSchemeTraits>::TBiometricsScores;
using TBiometryClassificationScheme = NBASSRequest::TMeta<TSchemeTraits>::TBiometryClassification;

const TString VINS_PATH = "/vins";
const TString PREPARE_PATH = "/megamind/prepare";
const TString APPLY_PATH = "/megamind/apply";

const TString BASS_SRCRWR_PREFIX = "BASS_SRCRWR_";

// It is highly advisable to avoid using this
// Only allowed for turkish navi scenarios due to their low-profit (atm) nature
// and Weather scenario (because "nowcast" can only show weather for today, so we switch often)
const THashSet<TString> ALLOWED_FORM_CHANGE_INTENTS = {
    "personal_assistant.scenarios.get_weather",
    "personal_assistant.scenarios.get_weather__details",
    "personal_assistant.scenarios.get_weather__ellipsis",
    "personal_assistant.scenarios.get_weather_nowcast",
    "personal_assistant.scenarios.get_weather_nowcast__ellipsis",
    "personal_assistant.scenarios.player_continue",
    "personal_assistant.scenarios.show_route",
    "personal_assistant.scenarios.voiceprint_enroll",
    "personal_assistant.scenarios.voiceprint_enroll__collect_voice",
};

TString GetLegacyUtterance(const NScenarios::TInput& input) {
    if (input.HasVoice()) {
        const auto& voice = input.GetVoice();

        if (const auto& asrData = voice.GetAsrData(); !asrData.empty()) {
            if (const auto& words = asrData[0].GetWords(); !words.empty()) {
                // recover the original utterance the same way VINS does it
                TStringBuilder result;
                bool first = true;
                for (const auto& word : words) {
                    if (first) {
                        first = false;
                    } else {
                        result << ' ';
                    }
                    result << word.GetValue();
                }
                return result;
            }
        }
        return voice.GetUtterance();
    }

    if (input.HasText()) {
        return input.GetText().GetUtterance();
    }

    return {};
}

TString GetAsrUtterance(const NScenarios::TInput& input) {
    if (input.HasVoice()) {
        if (const auto& asrData = input.GetVoice().GetAsrData(); !asrData.empty()) {
            return asrData[0].GetUtterance();
        }
    }

    return {}; // ASR is only relevant for the voice input
}

template <typename TScheme, typename TProto>
TSchemeHolder<TScheme> ConvertProtobuf(const TProto& proto) {
    // TODO(a-square): find a less hacky conversion
    return TSchemeHolder<TScheme>(NSc::TValue::FromJsonValue(JsonFromProto(proto)));
}

TSchemeHolder<TLocationScheme> ConvertLocation(const TLocation& location) {
    TSchemeHolder<TLocationScheme> scheme;
    scheme->Lon() = location.GetLon();
    scheme->Lat() = location.GetLat();
    if (location.GetAccuracy()) {
        scheme->Accuracy() = location.GetAccuracy();
    }
    if (location.GetRecency()) {
        scheme->Recency() = location.GetRecency(); // double-to-int truncation, both in milliseconds
    }
    return scheme;
}

TSchemeHolder<TBiometricsScoresScheme> ConvertBiometricsScores(const TBiometryScoring& proto) {
    auto json = JsonFromProto(proto);

    // patch scores_with_modes:
    // - protobuf doesn't distinguish absent repeated field from an empty repeated field
    // - scheme does, and meta.biometrics_scores.scores_with_modes items have a required scores array
    //   - so we put it in if it's not already there to avoid scheme validation errors
    if (auto* scoresWithModes = json.GetMapSafe().FindPtr(TStringBuf("scores_with_mode"))) {
        for (auto& item : scoresWithModes->GetArraySafe()) {
            item.GetMapSafe()[TStringBuf("scores")].SetType(NJson::JSON_ARRAY);
        }
    } else {
        json["scores_with_mode"].SetType(NJson::JSON_ARRAY);
    }

    return TSchemeHolder<TBiometricsScoresScheme>(NSc::TValue::FromJsonValue(json));
}

TEvent CreatePartialEvent(const TScenarioInputWrapper& input) {
    TEvent event;
    const auto& inputProto = input.Proto();

    switch (inputProto.GetEventCase()) {
        case NScenarios::TInput::EventCase::kText: {
            event.SetType(EEventType::text_input);
            event.SetText(inputProto.GetText().GetRawUtterance());
            break;
        }
        case NScenarios::TInput::EventCase::kVoice: {
            event.SetType(EEventType::voice_input);

            const auto& voice = inputProto.GetVoice();
            *event.MutableAsrResult() = voice.GetAsrData();

            // XXX(a-square): add a HasBiometryScoring check if necessary,
            // not doing it right now to preserve the tests
            *event.MutableBiometryScoring() = voice.GetBiometryScoring();

            if (voice.HasBiometryClassification()) {
                *event.MutableBiometryClassification() = voice.GetBiometryClassification();
            }

            break;
        }
        case NScenarios::TInput::EventCase::kImage: {
            event.SetType(EEventType::image_input);
            break;
        }
        case NScenarios::TInput::EventCase::kCallback: {
            event.SetType(EEventType::server_action);
            *event.MutablePayload() = inputProto.GetCallback().GetPayload();
            event.SetName(inputProto.GetCallback().GetName());
            event.SetIgnoreAnswer(inputProto.GetCallback().GetIgnoreAnswer());
            break;
        }
        default: {
            break;
        }
    }
    return event;
}

TString CreateBassPathWithCgi(const TString& path, const NJson::TJsonValue& appHostParams) {
    TStringBuilder fullPath = TStringBuilder() << path;

    bool firstCgi = true;
    for (const auto& [source, rewrite]: appHostParams["srcrwr"].GetMap()) {
        if (source.StartsWith(BASS_SRCRWR_PREFIX)) {
            if (firstCgi) {
                fullPath << '?';
                firstCgi = false;
            } else {
                fullPath << '&';
            }
            fullPath << "srcrwr=BASS_" << TStringBuf{source}.substr(BASS_SRCRWR_PREFIX.length())
                     << ':' << rewrite.GetString();
        }
    }

    return fullPath;
}

} // namespace

TBassForm CreateBassLikeForm(const TFrame& frame, const TSourceTextProvider* sourceTextProvider) {
    TBassForm bassForm;
    bassForm->Name() = frame.Name();

    auto slots = bassForm->Slots();
    for (const auto& formSlot : frame.Slots()) {
        auto slot = slots.Add();
        slot->Name() = formSlot.Name;
        const auto type = formSlot.Type;
        slot->Type() = type;
        if (type == "int") {
            slot->Value() = FromString<long>(formSlot.Value.AsString());
        } else if (type == "list") {
            const auto value = NSc::TValue::FromJsonThrow(formSlot.Value.AsString());
            const auto& array = value.GetArray();
            slot->Value()->AppendAll(array.begin(), array.end());
        } else {
            slot->Value() = formSlot.Value.AsString();
        }
        slot->Optional() = true;

        if (sourceTextProvider) {
            if (auto sourceText = (*sourceTextProvider)(formSlot.Name, formSlot.Value.AsString())) {
                slot->SourceText() = std::move(sourceText);
            }
        }
    }

    return bassForm;
}

// Copypasted (+ modified) from alice/megamind/library/context/speechkit_utils.cpp
TBassMeta CreateBassLikeMeta(const TScenarioBaseRequestWrapper& request,
                             const TScenarioInputWrapper& input,
                             const bool imageSearch,
                             const bool forbidWebSearch,
                             const bool splitUuid,
                             const bool suppressFormChanges,
                             const TString& from) {
    TBassMeta bassLikeMeta;

    // TODO(a-square): consider supporting config_patch

    // TODO(a-square): permissions
    // - present in VINS-like request
    // - used by alice/bass/forms/context/context.cpp (GetPermissionInfo)
    // - only EClientPermission::PushNotifications is queried by the reminder BASS form

    // TODO(a-square): utterance_data
    // - used by computer_vision, shazam forms
    // - VINS sets the following payload:
    //   - Music: data, result, error_text
    //   - Image: data

    // TODO(a-square): figure out misc fields
    // - (not set by VINS?) yandex_uid (different from uid? for computer_vision, search forms)
    //   - used by computer_vision, search forms
    //   - normally lives in additional_options, **not** bass_options
    //   - YaBro puts yandexuid=... cookie in the cookies
    //     - https://bitbucket.browser.yandex-team.ru/projects/STARDUST/repos/browser/browse/src/services/speechkit/uniproxy_protocol.cc
    //   - nobody appears to put them in bass_options, and VINS doesn't set yandex_uid directly
    //   - maybe clients other than YaBro put it in?
    // - (not set by VINS?) tld (https://a.yandex-team.ru/review/1026704)

    // NOTE(a-square): fields intentionally left out:
    // - end_of_utterance (BASS shouldn't care if we're in a partial)
    // - sequence_number (BASS logs it if it's present but doesn't use it otherwise)
    // - hypothesis_number (not used by BASS)
    // - laas_region
    //   - MM should merge location data before querying protocol scenarios
    //   - https://st.yandex-team.ru/MEGAMIND-596
    // - uid
    //   - judging by comments in BASS, it is only ever set by tests,
    //     and normally UID is retrieved from the BlackBox by BASS itself
    //   - actual usage in BASS supports this
    //   - not set by VINS, Uniproxy or YaBro, unlikely to be set by other clients as well
    // - cookies, megamind_cgi_string, process_id
    //   - these are only used by BASS in the search form, but MM does the web search by itself
    //     (and puts the result in the WEB_SEARCH data source), so no need for those anymore

    const auto& inputProto = input.Proto();

    if (request.ExpFlag(EXP_HW_ENABLE_BASS_ADAPTER_LEGACY_UTTERANCE)) {
        bassLikeMeta->Utterance() = GetLegacyUtterance(inputProto);
        if (const auto asrUtterance = GetAsrUtterance(inputProto)) {
            bassLikeMeta->AsrUtterance() = asrUtterance;
        }
    } else {
        bassLikeMeta->Utterance() = input.Utterance();
    }

    if (inputProto.HasVoice() && inputProto.GetVoice().HasBiometryScoring()) {
        bassLikeMeta->BiometricsScores() = ConvertBiometricsScores(inputProto.GetVoice().GetBiometryScoring()).Scheme();
    }

    if (inputProto.HasVoice() && inputProto.GetVoice().HasBiometryClassification()) {
        bassLikeMeta->BiometryClassification() = ConvertProtobuf<TBiometryClassificationScheme>(
            inputProto.GetVoice().GetBiometryClassification()
        ).Scheme();
    }

    const auto& baseRequest = request.BaseRequestProto();
    bassLikeMeta->DialogId() = baseRequest.GetDialogId();
    bassLikeMeta->RequestId() = baseRequest.GetRequestId();
    if (baseRequest.HasLocation()) {
        // protobuf's location has optional Lat and Lon, but in the BASS request schema they are required
        const auto& location = baseRequest.GetLocation();
        if (location.GetLon() || location.GetLat()) {
            bassLikeMeta->Location() = ConvertLocation(location).Scheme();
        }
    }
    if (baseRequest.HasDeviceState()) {
        bassLikeMeta->DeviceState() = ConvertProtobuf<TDeviceStateScheme>(baseRequest.GetDeviceState()).Scheme();
    }
    NSc::TValue::FromJsonValue(*bassLikeMeta->Experiments().GetMutable(), JsonFromProto(baseRequest.GetExperiments()));

    // TODO(a-square): is the seed the same as when the query goes through VINS?
    // TODO(a-square): can we modify the seed so that it's not the same for all BASS requests?
    bassLikeMeta->RngSeed() = ToString(baseRequest.GetRandomSeed());

    if (baseRequest.GetServerTimeMs()) {
        bassLikeMeta->RequestStartTime() = 1000 * baseRequest.GetServerTimeMs();
    }

    const auto& interfacesProto = baseRequest.GetInterfaces();
    bassLikeMeta->VoiceSession() = interfacesProto.GetVoiceSession();

    auto supported = bassLikeMeta->ClientFeatures()->Supported();
    if (!interfacesProto.GetHasReliableSpeakers()) {
        supported.Add() = "no_reliable_speakers";
    }
    if (!interfacesProto.GetHasBluetooth()) {
        supported.Add() = "no_bluetooth";
    }
    if (interfacesProto.GetHasAccessToBatteryPowerState()) {
        supported.Add() = "battery_power_state";
    }
    if (interfacesProto.GetHasCEC()) {
        supported.Add() = "cec_available";
    }
    if (interfacesProto.GetCanChangeAlarmSound()) {
        supported.Add() = "change_alarm_sound";
    }
    if (interfacesProto.GetCanOpenYandexAuth()) {
        supported.Add() = "open_yandex_auth";
    }
    if (interfacesProto.GetCanRecognizeImage()) {
        supported.Add() = "image_recognizer";
    }
    if (interfacesProto.GetSupportsDeviceLocalReminders()) {
        supported.Add() = "supports_device_local_reminders";
    }
    if (!interfacesProto.GetHasMicrophone()) {
        supported.Add() = "no_microphone";
    }
    if (interfacesProto.GetHasMusicPlayerShots()) {
        supported.Add() = "music_player_allow_shots";
    }
    if (interfacesProto.GetHasMusicSdkClient()) {
        supported.Add() = "music_sdk_client";
    }
    if (interfacesProto.GetMultiroom()) {
        supported.Add() = "multiroom";
    }
    if (interfacesProto.GetMultiroomCluster()) {
        supported.Add() = "multiroom_cluster";
    }
    if (interfacesProto.GetTtsPlayPlaceholder()) {
        supported.Add() = "tts_play_placeholder";
    }
    if (interfacesProto.GetHasBluetoothPlayer()) {
        supported.Add() = "bluetooth_player";
    }
    if (interfacesProto.GetCanRenderDiv2Cards()) {
        supported.Add() = "div2_cards";
    }
    if (interfacesProto.GetCanRenderDivCards()) {
        supported.Add() = "div_cards";
    }
    if (interfacesProto.GetHasMusicQuasarClient()) {
        supported.Add() = "music_quasar_client";
    }
    if (interfacesProto.GetCanSetAlarmSemanticFrame()) {
        supported.Add() = SET_ALARM_SEMANTIC_FRAME;
    }
    if (interfacesProto.GetSupportsShowPromo()) {
        supported.Add() = "show_promo";
    }

    if (baseRequest.HasOptions()) {
        const auto& optionsProto = baseRequest.GetOptions();
        bassLikeMeta->RawUserAgent() = optionsProto.GetUserAgent();
        bassLikeMeta->FiltrationLevel() = optionsProto.GetFiltrationLevel();
        bassLikeMeta->ClientIP() = optionsProto.GetClientIP();
        bassLikeMeta->ScreenScaleFactor() = optionsProto.GetScreenScaleFactor();
        bassLikeMeta->VideoGalleryLimit() = optionsProto.GetVideoGalleryLimit();
        if (const auto regionId = optionsProto.GetUserDefinedRegionId()) {
            bassLikeMeta->RegionId() = regionId;
        }
        // XXX(a-square): currently BASS doesn't scrub personal data from its logs,
        // we can resume sending personal data when it's no longer an issue, see MEGAMIND-816
        // *bassLikeMeta->PersonalData().GetMutable() = NSc::TValue::FromJson(optionsProto.GetRawPersonalData());
    }

    const auto& clientInfoProto = baseRequest.GetClientInfo();
    bassLikeMeta->UUID() = splitUuid
        ? NAlice::SplitID(clientInfoProto.GetUuid())
        : clientInfoProto.GetUuid(); // required field
    bassLikeMeta->DeviceId() = NAlice::SplitID(clientInfoProto.GetDeviceId());
    bassLikeMeta->TimeZone() = clientInfoProto.GetTimezone(); // required field
    bassLikeMeta->Epoch() = FromStringWithDefault<ui64>(clientInfoProto.GetEpoch(), 0UL); // required field
    bassLikeMeta->ClientId() = ConstructClientId(clientInfoProto);
    bassLikeMeta->Lang() = clientInfoProto.GetLang();
    if (bassLikeMeta->Lang() == "tr") {
        bassLikeMeta->Lang() = "tr-TR";
    }
    if (baseRequest.GetUserLanguage() != ::NAlice::ELang::L_UNK) {
        bassLikeMeta->UserLang() = IsoNameByLanguage(static_cast<ELanguage>(baseRequest.GetUserLanguage()));
    }

    auto clientInfo = bassLikeMeta->ClientInfo();
    clientInfo->AppId() = clientInfoProto.GetAppId();
    clientInfo->AppVersion() = clientInfoProto.GetAppVersion();
    clientInfo->DeviceManufacturer() = clientInfoProto.GetDeviceManufacturer();
    clientInfo->DeviceModel() = clientInfoProto.GetDeviceModel();
    clientInfo->Platform() = clientInfoProto.GetPlatform();
    clientInfo->OsVersion() = clientInfoProto.GetOsVersion();

    // TODO(a-square): VINS fills this from the banlist, BASS only uses it
    // to change form from search to general_conversation.
    // It's likely we'll never need it.
    bassLikeMeta->IsBanned() = false;

    // TODO(a-square): VINS checks session.get('pure_general_conversation'),
    // we probably don't need to until we have to port GC to HW
    bassLikeMeta->PureGC() = false;

    // NOTE(a-square): MM checks if "personal_assistant.scenarios.search.images" was parsed
    bassLikeMeta->HasImageSearchGranet() = imageSearch;

    // NOTE(a-square): We reconstruct the SpeechKit input event because BASS wants it.
    // Ideally we'd like to replace it with a better assortment of fields, but it's not critical,
    // and BASS is not long for this world anyway, so that's not happening any time soon.
    NSc::TValue::FromJsonValue(*bassLikeMeta->Event().GetMutable(), JsonFromProto(CreatePartialEvent(input)));

    bassLikeMeta->Validate(/* path= */ {}, /* strict= */ false, /* onError= */ [](const auto& path, const auto& error) {
        ythrow TBassMetaValidationError() << "BASS Meta validation failed at path " << path << ": " << error;
    });

    // When invoking BASS from Hollywood, it is highly advisable to disable any form changes,
    // even if we're using the legacy VINS protocol.
    // Only allowed for turkish navi scenarios due to their low-profit nature
    bassLikeMeta->SuppressFormChanges() = suppressFormChanges;

    if (forbidWebSearch) {
        // False by default. True value means BASS shouldn't generate web search request.
        // DIALOG-6569: Actual for news scenario at the moment.
        bassLikeMeta->ForbidWebSearch() = forbidWebSearch;
    }

    if (from) {
        bassLikeMeta->MusicFrom() = from;
    }

    return bassLikeMeta;
}

TBassAction CreateMusicPlayObjectAction(const TFrame& frame) {
    TBassAction action;
    action->Name() = "quasar.music_play_object";

    NJson::TJsonValue dataJson;
    for (const auto& formSlot : frame.Slots()) {
        if (formSlot.Name == NAlice::NMusic::SLOT_OBJECT_ID) {
            dataJson["object"]["id"] = formSlot.Value.AsString();
        } else if (formSlot.Name == NAlice::NMusic::SLOT_OBJECT_TYPE) {
            dataJson["object"]["type"] = formSlot.Value.AsString();
            if (formSlot.Value.AsString() == TMusicPlayObjectTypeSlot_EValue_Name(TMusicPlayObjectTypeSlot_EValue_Artist)) {
                // Need for correct tracks sequence
                dataJson["isPopular"] = true;
            }
        } else if (formSlot.Name == NAlice::NMusic::SLOT_START_FROM_TRACK_ID) {
            dataJson["object"]["startFromId"] = formSlot.Value.AsString();
        } else if (formSlot.Name == NAlice::NMusic::SLOT_TRACK_OFFSET_INDEX) {
            dataJson["object"]["startFromPosition"] = formSlot.Value.As<ui32>().GetRef();
        } else if (formSlot.Name == NAlice::NMusic::SLOT_OFFSET_SEC) {
            dataJson["offset"] = formSlot.Value.As<double>().GetRef();
        } else if (formSlot.Name == NAlice::NMusic::SLOT_ALARM_ID) {
            dataJson["alarm_id"] = formSlot.Value.AsString();
        } else if (formSlot.Name == NAlice::NMusic::SLOT_ORDER) {
            dataJson["shuffle"] = formSlot.Value.AsString() == "shuffle";
        } else if (formSlot.Name == NAlice::NMusic::SLOT_REPEAT) {
            dataJson["repeat"] = formSlot.Value.AsString() == "All";
        }
    }

    NSc::TValue::FromJsonValue(*action->Data().GetMutable(), dataJson);

    return action;
}

TBassRequest PrepareBassRequestBody(TRTLogger& logger,
                                    const TScenarioBaseRequestWrapper& request,
                                    const TScenarioInputWrapper& input,
                                    const TFrame& frame,
                                    const TSourceTextProvider* sourceTextProvider,
                                    const bool imageSearch,
                                    const bool forbidWebSearch,
                                    const THashMap<EDataSourceType, const NScenarios::TDataSource*>& dataSources,
                                    const bool splitUuid) {
    TBassRequest bassRequest;

    const bool hasSlotForMusicPlayObject = HasSlotForMusicPlayObject(frame);

    try {
        if (hasSlotForMusicPlayObject) {
            bassRequest->Action() = NImpl::CreateMusicPlayObjectAction(frame).Scheme();
        } else {
            bassRequest->Form() = NImpl::CreateBassLikeForm(frame, sourceTextProvider).Scheme();
        }
    } catch (...) {
        LOG_ERROR(logger) << "Failed to create BASS form: " << CurrentExceptionMessage();
        throw;
    }

    try {
        TString from;
        if (hasSlotForMusicPlayObject) {
            if (const auto slot = frame.FindSlot("from")) {
                from = slot->Value.AsString();
            }
        }
        const bool canChangeForm = ALLOWED_FORM_CHANGE_INTENTS.contains(frame.Name()) || hasSlotForMusicPlayObject;
        bassRequest->Meta() = NImpl::CreateBassLikeMeta(
            request,
            input,
            imageSearch,
            forbidWebSearch,
            splitUuid,
            !canChangeForm,
            from
        ).Scheme();
    } catch (...) {
        LOG_ERROR(logger) << "Failed to create BASS meta: " << CurrentExceptionMessage();
        throw;
    }

    {
        for (const auto& [type, dataSource] : dataSources) {
            Y_ENSURE(dataSource, "Expect valid pointer");
            NSc::TValue::FromJsonValue(*bassRequest->DataSources()[type].GetMutable(), JsonFromProto(*dataSource));
        }
    }
    // NOTE(a-square): session_state is the legacy way of adding user name to Alice response,
    // the proper way of doing it in protocol scenarios is with a combiner scenario.
    // Do **not** support session_state unless you absolutely have to
    //
    // TODO(a-square): fill action?
    return bassRequest;
}

THttpProxyRequest PrepareHttpBassRequest(TRTLogger& logger,
                                        const TString& path,
                                        const TString& body,
                                        const TString& formName,
                                        const NScenarios::TRequestMeta& meta) {
    LOG_DEBUG(logger) << "Bass request body: " << body;

    return PrepareHttpRequest(path, meta, logger, formName, body);
}

NJson::TJsonValue CreateSlot(TStringBuf name, TStringBuf type, TStringBuf value) {
    NJson::TJsonValue result{NJson::JSON_MAP};
    result["name"] = TString{name};
    result["type"] = TString{type};
    result["value"] = TString{value};
    result["optional"] = true;
    return result;
}

} // namespace NImpl

bool HasSlotForMusicPlayObject(const TFrame& frame) {
    return AnyOf(
        frame.Slots().begin(),
        frame.Slots().end(),
        [](const auto& formSlot) { return formSlot.Name == NAlice::NMusic::SLOT_OBJECT_ID; }
    );
}

THttpProxyRequest PrepareBassVinsRequest(TRTLogger& logger,
                                        const TScenarioRunRequestWrapper& request,
                                        const TFrame& frame,
                                        const TSourceTextProvider* sourceTextProvider,
                                        const NScenarios::TRequestMeta& meta,
                                        const bool imageSearch,
                                        const NJson::TJsonValue& appHostParams,
                                        const bool forbidWebSearch,
                                        const TVector<EDataSourceType>& dataSourceTypes) {
    THashMap<EDataSourceType, const NScenarios::TDataSource*> dataSources;
    for (const auto type : dataSourceTypes) {
        if (const auto* dataSource = request.GetDataSource(type)) {
            dataSources[type] = dataSource;
        }
    }
    return PrepareBassVinsRequest(logger, request, request.Input(), frame, sourceTextProvider, meta, imageSearch,
                                  appHostParams, forbidWebSearch, dataSources);
}

THttpProxyRequest PrepareBassVinsRequest(TRTLogger& logger,
                                        const TScenarioBaseRequestWrapper& request,
                                        const TScenarioInputWrapper& input,
                                        const TFrame& frame,
                                        const TSourceTextProvider* sourceTextProvider,
                                        const NScenarios::TRequestMeta& meta,
                                        bool imageSearch,
                                        const NJson::TJsonValue& appHostParams,
                                        const bool forbidWebSearch,
                                        const THashMap<EDataSourceType, const NScenarios::TDataSource*>& dataSources,
                                        const bool splitUuid) {
    LOG_INFO(logger) << "Building BASS request body";
    const auto bassRequestBody = NImpl::PrepareBassRequestBody(logger, request, input,
                                                               frame, sourceTextProvider, imageSearch,
                                                               forbidWebSearch, dataSources,
                                                               splitUuid);

    const auto path = NImpl::CreateBassPathWithCgi(NImpl::VINS_PATH, appHostParams);
    return NImpl::PrepareHttpBassRequest(logger, path,
                                         bassRequestBody.Value().ToJson(), frame.Name(), meta);
}

THttpProxyRequest PrepareBassRunRequest(TRTLogger& logger,
                                       const TScenarioRunRequestWrapper& request,
                                       const TFrame& frame,
                                       const TSourceTextProvider* sourceTextProvider,
                                       const NScenarios::TRequestMeta& meta,
                                       const bool imageSearch,
                                       const NJson::TJsonValue& appHostParams) {
    LOG_INFO(logger) << "Building BASS request body";
    const auto bassRequestBody = NImpl::PrepareBassRequestBody(logger, request, request.Input(),
                                                               frame, sourceTextProvider, imageSearch);

    const auto path = NImpl::CreateBassPathWithCgi(NImpl::PREPARE_PATH, appHostParams);
    return NImpl::PrepareHttpBassRequest(logger, path,
                                         bassRequestBody.Value().ToJson(), frame.Name(), meta);
}

THttpProxyRequest PrepareBassApplyRequest(TRTLogger& logger,
                                         const TScenarioApplyRequestWrapper& request,
                                         const NJson::TJsonValue& state,
                                         const NScenarios::TRequestMeta& meta,
                                         const TString& continuationName,
                                         const bool imageSearch,
                                         const NJson::TJsonValue& appHostParams) {
    NJson::TJsonValue bassRequest;
    bassRequest[TStringBuf("IsFinished")] = false;
    bassRequest[TStringBuf("ObjectTypeName")] = continuationName;
    bassRequest[TStringBuf("State")] = state;
    bassRequest[TStringBuf("Meta")]
        = NImpl::CreateBassLikeMeta(request, request.Input(), imageSearch,
                                    /* forbidWebSearch */ false, /* splitUuid */ true,
                                    /* suppressFormChanges */ true).Scheme().ToJson();

    const auto path = NImpl::CreateBassPathWithCgi(NImpl::APPLY_PATH, appHostParams);
    return NImpl::PrepareHttpBassRequest(logger, path, JsonToString(bassRequest),
                                         /* formName= */ {}, meta);
}

THttpProxyRequest PrepareBassRadioSimilarToObjContinueRequest(
    TRTLogger& logger,
    const TScenarioApplyRequestWrapper& request,
    const TString& contentType, const TString& contentId,
    bool isMuspult, const TString& radioStationId, const TString& startFromTrackId,
    const NScenarios::TRequestMeta& meta, const TString& continuationName,
    const NJson::TJsonValue& appHostParams)
{
    NJson::TJsonValue bassRequest;
    bassRequest[TStringBuf("IsFinished")] = false;
    bassRequest[TStringBuf("ObjectTypeName")] = continuationName;

    NJson::TJsonValue state;
    auto& webAnswer = state["apply_arguments"]["web_answer"];
    webAnswer["id"] = contentId;
    webAnswer["type"] = contentType;
    state["context"]["blocks"].SetType(NJson::JSON_ARRAY);
    auto& form = state["context"]["form"];
    form["name"] = "personal_assistant.scenarios.music_play";
    auto& slots = form["slots"];
    slots.SetType(NJson::JSON_ARRAY);
    auto& slotsArray = slots.GetArraySafe();
    if (isMuspult) {
        if (!startFromTrackId.empty()) {
            slotsArray.push_back(NImpl::CreateSlot(/* name = */ "track_to_start_radio_from", /* type = */ "string",
                    /* value = */ startFromTrackId));
        }
        slotsArray.push_back(NImpl::CreateSlot(/* name = */ "remote_type", /* type = */ "string", /* value = */ "pult"));
        slotsArray.push_back(NImpl::CreateSlot(/* name = */ "radio_seeds", /* type = */ "string", /* value = */ radioStationId));
    } else {
        slotsArray.push_back(NImpl::CreateSlot(/* name = */ "need_similar", /* type = */ "string",
                /* value = */ "need_similar"));
        TString objectSlotName = TStringBuilder{} << contentType << "_id";
        slotsArray.push_back(NImpl::CreateSlot(/* name = */ objectSlotName, /* type = */ "string",
                /* value = */ contentId));
    }

    bassRequest[TStringBuf("State")] = JsonToString(state);
    bassRequest[TStringBuf("Meta")]
        = NImpl::CreateBassLikeMeta(request, request.Input(), /* imageSearch = */ false).Scheme().ToJson();

    const auto path = NImpl::CreateBassPathWithCgi(NImpl::APPLY_PATH, appHostParams);
    return NImpl::PrepareHttpBassRequest(logger, path, JsonToString(bassRequest),
                                         /* formName= */ {}, meta);
}

THttpProxyRequest PrepareBassRadioStationIdContinueRequest(
    TRTLogger& logger,
    const TScenarioApplyRequestWrapper& request,
    const TString& radioStationId, const TString& from,
    bool isMuspult, const TString& startFromTrackId,
    const NScenarios::TRequestMeta& meta, const TString& continuationName,
    const NJson::TJsonValue& appHostParams)
{
    NJson::TJsonValue bassRequest;
    bassRequest[TStringBuf("IsFinished")] = false;
    bassRequest[TStringBuf("ObjectTypeName")] = continuationName;

    NJson::TJsonValue state;

    auto& form = state["context"]["form"];
    form["name"] = "personal_assistant.scenarios.music_play";

    if (!radioStationId.empty()) {
        auto& slots = form["slots"];
        slots.SetType(NJson::JSON_ARRAY);
        auto& slotsArray = slots.GetArraySafe();

        if (isMuspult) {
            if (!startFromTrackId.empty()) {
                slotsArray.push_back(NImpl::CreateSlot(/* name = */ "track_to_start_radio_from", /* type = */ "string",
                    /* value = */ startFromTrackId));
            }
            slotsArray.push_back(NImpl::CreateSlot(/* name = */ "remote_type", /* type = */ "string", /* value = */ "pult"));
            slotsArray.push_back(NImpl::CreateSlot(/* name = */ "radio_seeds", /* type = */ "string", /* value = */ radioStationId));
        } else {
            const auto radioStationIdItems = StringSplitter(radioStationId).Split(
                    ':').ToList<TStringBuf>();
            Y_ENSURE(radioStationIdItems.size() == 2);
            const auto type = radioStationIdItems.at(0);
            const auto tag = radioStationIdItems.at(1);

            TVector <TString> similarTypes = {"track", "album", "artist", "playlist", "specialPlaylist"};
            if (Find(similarTypes.begin(), similarTypes.end(), type) != similarTypes.end()) {
                slotsArray.push_back(NImpl::CreateSlot(/* name = */ "need_similar", /* type = */ "string",
                        /* value = */ "need_similar"));
                TString objectSlotName = TStringBuilder{} << type << "_id";
                slotsArray.push_back(NImpl::CreateSlot(/* name = */ objectSlotName, /* type = */ "string",
                        /* value = */ tag));
            } else {
                slotsArray.push_back(NImpl::CreateSlot(/* name = */ type, /* type = */ type,
                        /* value = */ tag));
            }
        }
    }

    bassRequest[TStringBuf("State")] = JsonToString(state);
    bassRequest[TStringBuf("Meta")]
        = NImpl::CreateBassLikeMeta(request, request.Input(), /* imageSearch = */ false,
                                    /* forbidWebSearch = */ false, /* splitUuid = */ true,
                                    /* suppressFormChanges = */ true, from).Scheme().ToJson();

    const auto path = NImpl::CreateBassPathWithCgi(NImpl::APPLY_PATH, appHostParams);
    return NImpl::PrepareHttpBassRequest(logger, path, JsonToString(bassRequest),
                                         /* formName= */ {}, meta);
}

void AddBassRequestItems(TScenarioHandleContext& ctx, const THttpProxyRequest& bassRequest) {
    AddHttpRequestItems(ctx, bassRequest, BASS_REQUEST_ITEM, BASS_REQUEST_RTLOG_TOKEN_ITEM);
}

NJson::TJsonValue RetireBassRequest(const TScenarioHandleContext& ctx) {
    return RetireHttpResponseJson(ctx, BASS_RESPONSE_ITEM, BASS_REQUEST_RTLOG_TOKEN_ITEM);
}

} // namespace NAlice::NHollywood
