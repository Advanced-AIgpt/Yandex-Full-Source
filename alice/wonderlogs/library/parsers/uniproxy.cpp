#include "uniproxy.h"

#include <alice/wonderlogs/library/parsers/internal/utils.h>

#include <alice/wonderlogs/library/common/names.h>
#include <alice/wonderlogs/library/common/utils.h>

#include <alice/wonderlogs/protos/request_stat.pb.h>

#include <alice/megamind/api/request/constructor.h>
#include <alice/megamind/protos/speechkit/request.pb.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>

namespace {

using namespace NAlice::NWonderlogs;

constexpr TStringBuf NAME = "name";
constexpr TStringBuf NAMESPACE = "namespace";
constexpr TStringBuf SESSION = "Session";
constexpr TStringBuf DIRECTIVE = "Directive";

} // namespace

namespace NAlice::NWonderlogs {

namespace NImpl {

const NJson::TJsonValue& GetSession(const NJson::TJsonValue& json) {
    return json[SESSION];
}

TMaybe<TString> ParseConnectSessionId(const NJson::TJsonValue& json) {
    return MaybeStringFromJson(GetSession(json)["SessionId"]);
}

TMaybe<TString> ParseClientIp(const NJson::TJsonValue& json) {
    return MaybeStringFromJson(GetSession(json)["IpAddr"]);
}

TMaybe<bool> ParseDoNotUseUserLogs(const NJson::TJsonValue& json) {
    return MaybeBoolFromJson(GetSession(json)["DoNotUseUserLogs"]);
}

const NJson::TJsonValue& GetEvent(const NJson::TJsonValue& json) {
    return json["Event"]["event"];
}

const NJson::TJsonValue& GetPayload(const NJson::TJsonValue& eventOrDirective) {
    return eventOrDirective["payload"];
}

const NJson::TJsonValue& GetDirective(const NJson::TJsonValue& json) {
    return json[DIRECTIVE];
}

const NJson::TJsonValue& GetInternalDirective(const NJson::TJsonValue& json) {
    return json[DIRECTIVE]["directive"];
}

const NJson::TJsonValue& GetHeader(const NJson::TJsonValue& eventOrDirectiveOrPayload) {
    return eventOrDirectiveOrPayload["header"];
}

} // namespace NImpl

TUniproxyLogsParser::TUniproxyLogsParser(const NJson::TJsonValue& json,
                                         const TMaybe<TUniproxyPrepared::TEnvironment>& environment,
                                         TInstant logfellerTimestamp, const TStringBuf messageDebugOnly)
    : Json(json)
    , Environment(environment)
    , LogfellerTimestamp(logfellerTimestamp)
    , EventType(DetermineType(Json))
    , MessageDebugOnly(messageDebugOnly) {
}

EUniproxyEventType TUniproxyLogsParser::GetType() const {
    return EventType;
}

TUniproxyParsedLog<TUniproxyPrepared::TRequestStatWrapper> TUniproxyLogsParser::ParseRequestStat() const {
    TUniproxyParsedLog<TUniproxyPrepared::TRequestStatWrapper> parsedLog;
    const auto& payload = NImpl::GetPayload(NImpl::GetEvent(Json));

    const auto uuid = ParseUuid(Json);
    const auto timestamp = ParseTimestampEvent(Json);
    const auto messageId = MaybeStringFromJson(payload["refMessageId"]);

    if (NotEmpty(uuid) && NotEmpty(messageId)) {
        FillCommonParts(*uuid, *messageId, timestamp, parsedLog.MessageIdToConnectSessionId,
                        parsedLog.MessageIdToEnvironment, parsedLog.MessageIdToClientIp,
                        parsedLog.MessageIdToDoNotUseUserLogs);
        TUniproxyPrepared::TRequestStatWrapper requestStatWrapper;
        requestStatWrapper.SetMessageId(*messageId);
        bool created = false;
        const auto& timestamps = payload["timestamps"];
        auto& mutableTimestamps = *requestStatWrapper.MutableRequestStat()->MutableTimestamps();
        if (timestamps.IsMap()) {
            if (const auto* field = timestamps.GetMap().FindPtr("onSoundPlayerEndTime"); field && field->IsString()) {
                mutableTimestamps.SetOnSoundPlayerEndTime(field->GetString());
                created = true;
            }
            if (const auto* field = timestamps.GetMap().FindPtr("onVinsResponseTime"); field && field->IsString()) {
                mutableTimestamps.SetOnVinsResponseTime(field->GetString());
                created = true;
            }
            if (const auto* field = timestamps.GetMap().FindPtr("onInterruptionPhraseSpottedTime");
                field && field->IsString()) {
                mutableTimestamps.SetOnInterruptionPhraseSpottedTime(field->GetString());
                created = true;
            }
            if (const auto* field = timestamps.GetMap().FindPtr("onFirstSynthesisChunkTime");
                field && field->IsString()) {
                mutableTimestamps.SetOnFirstSynthesisChunkTime(field->GetString());
                created = true;
            }
            if (const auto* field = timestamps.GetMap().FindPtr("onRecognitionBeginTime");
                field && field->IsString()) {
                mutableTimestamps.SetOnRecognitionBeginTime(field->GetString());
                created = true;
            }
            if (const auto* field = timestamps.GetMap().FindPtr("onRecognitionEndTime"); field && field->IsString()) {
                mutableTimestamps.SetOnRecognitionEndTime(field->GetString());
                created = true;
            }
            if (const auto* field = timestamps.GetMap().FindPtr("requestDurationTime"); field && field->IsString()) {
                mutableTimestamps.SetRequestDurationTime(field->GetString());
                created = true;
            }
            if (const auto* field = timestamps.GetMap().FindPtr("spotterConfirmationTime");
                field && field->IsString()) {
                mutableTimestamps.SetSpotterConfirmationTime(field->GetString());
                created = true;
            }
            if (const auto* field = timestamps.GetMap().FindPtr("onPhraseSpottedTime"); field && field->IsString()) {
                mutableTimestamps.SetOnPhraseSpottedTime(field->GetString());
                created = true;
            }
            if (const auto* field = timestamps.GetMap().FindPtr("prevSoundPlayerEndTime");
                field && field->IsString()) {
                mutableTimestamps.SetPrevSoundPlayerEndTime(field->GetString());
                created = true;
            }
            if (const auto* field = timestamps.GetMap().FindPtr("onStartVoiceInputTime"); field && field->IsString()) {
                mutableTimestamps.SetOnStartVoiceInputTime(field->GetString());
                created = true;
            }
            if (const auto* field = timestamps.GetMap().FindPtr("onStartVinsRequestTime");
                field && field->IsString()) {
                mutableTimestamps.SetOnStartVinsRequestTime(field->GetString());
                created = true;
            }
            if (const auto* field = timestamps.GetMap().FindPtr("onSoundPlayerBeginTime");
                field && field->IsString()) {
                mutableTimestamps.SetOnSoundPlayerBeginTime(field->GetString());
                created = true;
            }
            if (const auto* field = timestamps.GetMap().FindPtr("onFirstSocketActivityTime");
                field && field->IsString()) {
                mutableTimestamps.SetOnFirstSocketActivityTime(field->GetString());
                created = true;
            }
            if (const auto* field = timestamps.GetMap().FindPtr("onLastSynthesisChunkTime");
                field && field->IsString()) {
                mutableTimestamps.SetOnLastSynthesisChunkTime(field->GetString());
                created = true;
            }
            if (const auto* field = timestamps.GetMap().FindPtr("StartPlayer"); field && field->IsString()) {
                mutableTimestamps.SetStartPlayer(field->GetString());
                created = true;
            }
            if (const auto* field = timestamps.GetMap().FindPtr("OnPlayerBegin"); field && field->IsString()) {
                mutableTimestamps.SetOnPlayerBegin(field->GetString());
                created = true;
            }
            if (const auto* field = timestamps.GetMap().FindPtr("OnPlayerEnd"); field && field->IsString()) {
                mutableTimestamps.SetOnPlayerEnd(field->GetString());
                created = true;
            }
        }
        requestStatWrapper.SetUuid(*uuid);
        if (timestamp) {
            requestStatWrapper.SetTimestampLogMs(timestamp->MilliSeconds());
        }
        if (created) {
            parsedLog.ParsedEvent = requestStatWrapper;
        }
    } else {
        auto errorMessage = GenerateErrorMessage({{uuid, UUID}, {messageId, MESSAGE_ID}});
        errorMessage << "from RequestStat event: " << MessageDebugOnly << " timestamp: " << LogfellerTimestamp;
        parsedLog.Errors.emplace_back(
            GenerateError(TUniproxyPrepared::TError::R_INVALID_VALUE, errorMessage, uuid, messageId));
    }
    return parsedLog;
}

TUniproxyParsedLog<TUniproxyPrepared::TMegamindRequest> TUniproxyLogsParser::ParseMegamindRequest() const {
    TUniproxyParsedLog<TUniproxyPrepared::TMegamindRequest> parsedLog;

    const auto& directive = NImpl::GetDirective(Json);

    const auto timestamp = ParseTimestampEvent(Json);

    const auto uuid = ParseUuid(Json);
    const auto messageId = MaybeStringFromJson(directive["ForEvent"]);
    const auto requestId = MaybeStringFromJson(directive["Body"]["header"]["request_id"]);

    if (NotEmpty(uuid) && NotEmpty(requestId) && NotEmpty(messageId)) {
        FillCommonParts(*uuid, *messageId, timestamp, parsedLog.MessageIdToConnectSessionId,
                        parsedLog.MessageIdToEnvironment, parsedLog.MessageIdToClientIp,
                        parsedLog.MessageIdToDoNotUseUserLogs);
        TUniproxyPrepared::TMegamindRequest megamindRequest;
        megamindRequest.SetMegamindRequestId(*requestId);
        megamindRequest.SetMessageId(*messageId);
        megamindRequest.SetUuid(*uuid);
        if (timestamp) {
            megamindRequest.SetTimestampLogMs(timestamp->MilliSeconds());
        }
        parsedLog.ParsedEvent = megamindRequest;
    } else {
        auto errorMessage = GenerateErrorMessage({{uuid, UUID}, {messageId, MESSAGE_ID}, {requestId, REQUEST_ID}});
        errorMessage << "from VinsRequest directive: " << MessageDebugOnly << " timestamp: " << LogfellerTimestamp;
        parsedLog.Errors.emplace_back(
            GenerateError(TUniproxyPrepared::TError::R_INVALID_VALUE, errorMessage, uuid, messageId));
    }
    return parsedLog;
}

TUniproxyParsedLog<TUniproxyPrepared::TMegamindResponse> TUniproxyLogsParser::ParseMegamindResponse() const {
    TUniproxyParsedLog<TUniproxyPrepared::TMegamindResponse> parsedLog;

    const auto& directive = NImpl::GetInternalDirective(Json);
    const auto& header = NImpl::GetHeader(directive);
    const auto& payload = NImpl::GetPayload(directive);
    const auto& payloadHeader = NImpl::GetHeader(payload);

    const auto timestamp = ParseTimestampEvent(Json);

    const auto uuid = ParseUuid(Json);
    const auto messageId = MaybeStringFromJson(header["refMessageId"]);
    const auto responseId = MaybeStringFromJson(payloadHeader["response_id"]);
    const auto requestId = MaybeStringFromJson(payloadHeader["request_id"]);

    if (NotEmpty(uuid) && NotEmpty(requestId) && NotEmpty(messageId) && NotEmpty(responseId)) {
        FillCommonParts(*uuid, *messageId, timestamp, parsedLog.MessageIdToConnectSessionId,
                        parsedLog.MessageIdToEnvironment, parsedLog.MessageIdToClientIp,
                        parsedLog.MessageIdToDoNotUseUserLogs);
        TUniproxyPrepared::TMegamindResponse megamindResponse;
        megamindResponse.SetMegamindResponseId(*responseId);
        megamindResponse.SetMegamindRequestId(*requestId);
        megamindResponse.SetMessageId(*messageId);
        megamindResponse.SetUuid(*uuid);
        if (timestamp) {
            megamindResponse.SetTimestampLogMs(timestamp->MilliSeconds());
        }
        parsedLog.ParsedEvent = megamindResponse;
    } else {
        auto errorMessage = GenerateErrorMessage(
            {{uuid, UUID}, {messageId, MESSAGE_ID}, {requestId, REQUEST_ID}, {responseId, RESPONSE_ID}});
        errorMessage << "from VinsResponse directive: " << MessageDebugOnly << " timestamp: " << LogfellerTimestamp;
        parsedLog.Errors.emplace_back(
            GenerateError(TUniproxyPrepared::TError::R_INVALID_VALUE, errorMessage, uuid, messageId));
    }
    return parsedLog;
}

TUniproxyParsedLog<TUniproxyPrepared::TTestIds> TUniproxyLogsParser::ParseTestIds() const {
    TUniproxyParsedLog<TUniproxyPrepared::TTestIds> parsedLog;

    const auto& directive = NImpl::GetDirective(Json);

    const auto timestamp = ParseTimestampEvent(Json);

    const auto uuid = ParseUuid(Json);
    const auto messageId = MaybeStringFromJson(directive["ForEvent"]);

    if (NotEmpty(uuid) && NotEmpty(messageId)) {
        FillCommonParts(*uuid, *messageId, timestamp, parsedLog.MessageIdToConnectSessionId,
                        parsedLog.MessageIdToEnvironment, parsedLog.MessageIdToClientIp,
                        parsedLog.MessageIdToDoNotUseUserLogs);
        TUniproxyPrepared::TTestIds testIds;
        testIds.SetMessageId(*messageId);
        testIds.SetUuid(*uuid);
        if (timestamp) {
            testIds.SetTimestampLogMs(timestamp->MilliSeconds());
        }

        if (const auto* jsonTestIds = directive.GetValueByPath("Body.test_ids");
            jsonTestIds && jsonTestIds->IsArray()) {
            for (const auto& testId : jsonTestIds->GetArray()) {
                ui64 castedTestId;
                if (testId.IsString() && TryFromString<ui64>(testId.GetString(), castedTestId)) {
                    testIds.MutableTestIds()->Add(castedTestId);
                } else {
                    auto errorMessage = TStringBuilder{} << "Can't parse testId from json: " << MessageDebugOnly
                                                         << " timestamp: " << LogfellerTimestamp;
                    parsedLog.Errors.emplace_back(
                        GenerateError(TUniproxyPrepared::TError::R_INVALID_VALUE, errorMessage, uuid, messageId));

                    return parsedLog;
                }
            }
        }

        parsedLog.ParsedEvent = testIds;
    } else {
        auto errorMessage = GenerateErrorMessage({{uuid, UUID}, {messageId, MESSAGE_ID}});
        errorMessage << "from containing TestIds directive: " << MessageDebugOnly
                     << " timestamp: " << LogfellerTimestamp;
        parsedLog.Errors.emplace_back(
            GenerateError(TUniproxyPrepared::TError::R_INVALID_VALUE, errorMessage, uuid, messageId));
    }

    return parsedLog;
}

EUniproxyEventType TUniproxyLogsParser::DetermineType(const NJson::TJsonValue& json) {
    if (const auto* eventPtr = json.GetMap().FindPtr("Event")) {
        const auto& event = (*eventPtr)["event"];
        const auto& header = event["header"];

        if ("Log" == header[NAMESPACE] && "RequestStat" == header[NAME]) {
            return EUniproxyEventType::RequestStat;
        } else if ("Log" == header[NAMESPACE] && "Spotter" == header[NAME]) {
            return EUniproxyEventType::LogSpotter;
        } else if ("System" == header[NAMESPACE] && "SynchronizeState" == header[NAME]) {
            return EUniproxyEventType::SynchronizeState;
        } else if ("Vins" == header[NAMESPACE] && "VoiceInput" == header[NAME]) {
            return EUniproxyEventType::VoiceInput;
        } else if ("ASR" == header[NAMESPACE] && "Recognize" == header[NAME]) {
            return EUniproxyEventType::AsrRecognize;
        } else if ("TTS" == header[NAMESPACE] && "Generate" == header[NAME]) {
            return EUniproxyEventType::TtsGenerate;
        }
    } else if (const auto* directivePtr = json.GetMap().FindPtr("Directive")) {
        const auto& directive = *directivePtr;

        if ("VinsRequest" == directive["type"]) {
            return EUniproxyEventType::MegamindRequest;
        } else if (directive["type"] == "AsrCoreDebug" && directive["backend"] == "spotter") {
            return EUniproxyEventType::AsrDebug;
        } else if (const auto& header = directive["directive"]["header"];
                   "Vins" == header[NAMESPACE] && "VinsResponse" == header[NAME]) {
            return EUniproxyEventType::MegamindResponse;
        } else if ("Spotter" == header[NAMESPACE] && "Validation" == header[NAME]) {
            return EUniproxyEventType::SpotterValidation;
        } else if ("ASR" == header[NAMESPACE] && "Result" == header[NAME]) {
            return EUniproxyEventType::AsrResult;
        } else if (const auto& rootHeader = directive["header"];
                   "System" == rootHeader[NAMESPACE] && "UniproxyVinsTimings" == rootHeader[NAME]) {
            return EUniproxyEventType::MegamindTimings;
        } else if ("System" == rootHeader[NAMESPACE] && "UniproxyTTSTimings" == rootHeader[NAME]) {
            return EUniproxyEventType::TtsTimings;
        } else if (const auto action = MaybeStringFromJson(json["Session"]["Action"]);
                   action && *action == "log_flags_json") {
            return EUniproxyEventType::TestIds;
        }
    } else if (const auto* streamPtr = json.GetMap().FindPtr("Stream")) {
        const auto& stream = *streamPtr;
        const auto isSpotter = stream["isSpotter"].GetBoolean();
        if (isSpotter) {
            return EUniproxyEventType::SpotterStream;
        } else {
            return EUniproxyEventType::Stream;
        }
    }
    return EUniproxyEventType::Unknown;
}

TStringBuilder TUniproxyLogsParser::GenerateErrorMessage(const TVector<TValueName>& fields) {
    auto errorMessage = TStringBuilder{} << "Invalid ";
    bool first = true;
    for (const auto& [field, name] : fields) {
        if (!field || field->empty()) {
            if (!first) {
                errorMessage << ", ";
            }
            errorMessage << name << " ";
            first = false;
        }
    }
    return errorMessage;
}

TUniproxyPrepared::TError TUniproxyLogsParser::GenerateError(const TUniproxyPrepared::TError::EReason reason,
                                                             const TString& message, const TMaybe<TString>& uuid,
                                                             const TMaybe<TString>& messageId) {
    TUniproxyPrepared::TError error;
    error.SetProcess(TUniproxyPrepared::TError::P_UNIPROXY_EVENT_DIRECTIVE_MAPPER);
    error.SetReason(reason);
    error.SetMessage(message);
    if (uuid) {
        error.SetUuid(*uuid);
    }
    if (messageId) {
        error.SetMessageId(*messageId);
    }
    if (auto setraceUrl = TryGenerateSetraceUrl({error.HasMessageId() ? error.GetMessageId() : TMaybe<TString>{},
                                                 error.HasUuid() ? error.GetUuid() : TMaybe<TString>{}})) {
        error.SetSetraceUrl(*setraceUrl);
    }
    return error;
}

void TUniproxyLogsParser::FillCommonParts(
    const TString& uuid, const TString& messageId, TMaybe<TInstant> timestampEvent,
    TMaybe<TUniproxyPrepared::TMessageIdToConnectSessionId>& messageIdToConnectSessionId,
    TMaybe<TUniproxyPrepared::TMessageIdToEnvironment>& messageIdToEnvironment,
    TMaybe<TUniproxyPrepared::TMessageIdToClientIp>& messageIdToClientIp,
    TMaybe<TUniproxyPrepared::TMessageIdToDoNotUseUserLogs>& messageIdToDoNotUseUserLogs) const {
    const auto connectSessionId = NImpl::ParseConnectSessionId(Json);
    if (connectSessionId) {
        TUniproxyPrepared::TMessageIdToConnectSessionId value;
        value.SetUuid(uuid);
        value.SetMessageId(messageId);
        value.SetConnectSessionId(*connectSessionId);
        if (timestampEvent) {
            value.SetTimestampLogMs(timestampEvent->MilliSeconds());
        }
        messageIdToConnectSessionId = value;
    }
    const auto clientIp = NImpl::ParseClientIp(Json);
    if (clientIp) {
        TUniproxyPrepared::TMessageIdToClientIp value;
        value.SetUuid(uuid);
        value.SetMessageId(messageId);
        value.SetClientIp(*clientIp);
        if (timestampEvent) {
            value.SetTimestampLogMs(timestampEvent->MilliSeconds());
        }
        messageIdToClientIp = value;
    }
    const auto doNotUseUserLogs = NImpl::ParseDoNotUseUserLogs(Json);
    if (doNotUseUserLogs) {
        TUniproxyPrepared::TMessageIdToDoNotUseUserLogs value;
        value.SetUuid(uuid);
        value.SetMessageId(messageId);
        value.SetDoNotUseUserLogs(*doNotUseUserLogs);
        if (timestampEvent) {
            value.SetTimestampLogMs(timestampEvent->MilliSeconds());
        }
        messageIdToDoNotUseUserLogs = value;
    }
    if (Environment) {
        TUniproxyPrepared::TMessageIdToEnvironment value;
        value.SetUuid(uuid);
        value.SetMessageId(messageId);
        *value.MutableEnvironment() = *Environment;
        if (timestampEvent) {
            value.SetTimestampLogMs(timestampEvent->MilliSeconds());
        }
        messageIdToEnvironment = value;
    }
}

TMaybe<TString> ParseUuid(const NJson::TJsonValue& json) {
    auto uuid = MaybeStringFromJson(json[SESSION]["Uuid"]);
    if (uuid) {
        return NormalizeUuid(*uuid);
    }
    return uuid;
}

bool ParseVoiceInput(const NJson::TJsonValue& payload, TVoiceInput& voiceInput) {
    if (payload.IsMap()) {
        if (const auto* activationType = payload.GetValueByPath("request.activation_type");
            activationType && activationType->IsString()) {
            voiceInput.SetActivationType(activationType->GetString());
        }
        if (const auto topic = payload.GetMap().FindPtr("topic"); topic && topic->IsString()) {
            voiceInput.SetTopic(topic->GetString());
        }
    }
    NMegamindApi::TRequestConstructor constructor;
    const auto status = constructor.PushSpeechKitJson(payload);
    if (!status.Ok()) {
        return false;
    }
    *voiceInput.MutableSpeechKitRequest() = std::move(constructor).MakeRequest();
    return true;
}

TMaybe<TInstant> ParseTimestampEvent(const NJson::TJsonValue& json) {
    auto time = MaybeStringFromJson(json[SESSION]["Timestamp"]);
    if (time) {
        return ParseDatetime(*time);
    }
    return {};
}

bool ParseMegamindTimings(const NJson::TJsonValue& json, TMegamindTimings& megamindTimings) {
    google::protobuf::util::JsonParseOptions options;
    options.ignore_unknown_fields = true;
    const auto* payload = json.GetValueByPath("Directive.payload");
    if (!payload) {
        return false;
    }
    return google::protobuf::util::JsonStringToMessage(NJson::WriteJson(payload), &megamindTimings, options).ok();
}

bool ParseTtsTimings(const NJson::TJsonValue& json, TTtsTimings& ttsTimings) {
    google::protobuf::util::JsonParseOptions options;
    options.ignore_unknown_fields = true;
    const auto* payload = json.GetValueByPath("Directive.payload");
    if (!payload) {
        return false;
    }
    return google::protobuf::util::JsonStringToMessage(NJson::WriteJson(payload), &ttsTimings, options).ok();
}

bool ParseTtsGenerate(const NJson::TJsonValue& payload, TTtsGenerate& ttsGenerate) {
    if (payload.IsMap()) {
        const auto& paylodMap = payload.GetMap();

        if (const auto* text = paylodMap.FindPtr("text"); text && text->IsString()) {
            ttsGenerate.SetText(text->GetString());
        }

        if (const auto* voice = paylodMap.FindPtr("voice"); voice && voice->IsString()) {
            ttsGenerate.SetVoice(voice->GetString());
        }

        if (const auto* emotion = paylodMap.FindPtr("emotion"); emotion && emotion->IsString()) {
            ttsGenerate.SetEmotion(emotion->GetString());
        }

        if (const auto* format = paylodMap.FindPtr("format"); format && format->IsString()) {
            ttsGenerate.SetFormat(format->GetString());
        }

        if (const auto* quality = paylodMap.FindPtr("quality"); quality && quality->IsString()) {
            ttsGenerate.SetQuality(quality->GetString());
        }

        if (const auto* lang = paylodMap.FindPtr("lang"); lang && lang->IsString()) {
            ttsGenerate.SetLang(lang->GetString());
        }

        return true;
    }

    return false;
}

bool ParseSpotterActivationInfo(const NJson::TJsonValue& extra, TLogSpotter::TSpotterActivationInfo& spotterExtra) {
    if (!extra.IsMap()) {
        return false;
    }

    google::protobuf::util::JsonParseOptions options;
    options.ignore_unknown_fields = true;
    google::protobuf::util::JsonStringToMessage(NJson::WriteJson(extra), &spotterExtra, options);

    const auto& extraMap = extra.GetMap();

    if (const auto* streamType = extraMap.FindPtr("streamType"); streamType && streamType->IsString()) {
        spotterExtra.SetStreamType(streamType->GetString());
    }

    if (const auto* globalStreamId = extraMap.FindPtr("globalStreamId");
        globalStreamId && globalStreamId->IsString()) {
        spotterExtra.SetGlobalStreamId(globalStreamId->GetString());
    }

    if (const auto* actualSoundAfterTriggerMs = extraMap.FindPtr("actualSoundAfterTriggerMs");
        actualSoundAfterTriggerMs && actualSoundAfterTriggerMs->IsInteger()) {
        spotterExtra.SetActualSoundAfterTriggerMs(actualSoundAfterTriggerMs->GetInteger());
    }

    if (const auto* actualSoundBeforeTriggerMs = extraMap.FindPtr("actualSoundBeforeTriggerMs");
        actualSoundBeforeTriggerMs && actualSoundBeforeTriggerMs->IsInteger()) {
        spotterExtra.SetActualSoundBeforeTriggerMs(actualSoundBeforeTriggerMs->GetInteger());
    }

    if (const auto* requestSoundAfterTriggerMs = extraMap.FindPtr("requestSoundAfterTriggerMs");
        requestSoundAfterTriggerMs && requestSoundAfterTriggerMs->IsInteger()) {
        spotterExtra.SetRequestSoundAfterTriggerMs(requestSoundAfterTriggerMs->GetInteger());
    }

    if (const auto* requestSoundBeforeTriggerMs = extraMap.FindPtr("requestSoundBeforeTriggerMs");
        requestSoundBeforeTriggerMs && requestSoundBeforeTriggerMs->IsInteger()) {
        spotterExtra.SetRequestSoundBeforeTriggerMs(requestSoundBeforeTriggerMs->GetInteger());
    }

    if (const auto* isSpotterSound = extraMap.FindPtr("isSpotterSound");
        isSpotterSound && isSpotterSound->IsBoolean()) {
        spotterExtra.SetIsSpotterSound(isSpotterSound->GetBoolean());
    }

    if (const auto* unhandledBytes = extraMap.FindPtr("unhandledBytes");
        unhandledBytes && unhandledBytes->IsString()) {
        ui64 unhandledDataBytes;
        if (TryFromString<ui64>(unhandledBytes->GetString(), unhandledDataBytes)) {
            spotterExtra.SetUnhandledDataBytes(unhandledDataBytes);
        }
    }

    if (const auto* durationSubmitted = extraMap.FindPtr("durationSubmitted");
        durationSubmitted && durationSubmitted->IsString()) {
        ui64 durationDataSubmitted;
        if (TryFromString<ui64>(durationSubmitted->GetString(), durationDataSubmitted)) {
            spotterExtra.SetDurationDataSubmitted(durationDataSubmitted);
        }
    }

    return extra["metainfo"].IsString() && NJson::ValidateJson(extra["metainfo"].GetString()) &&
           ParseSpotterStats(NJson::ReadJsonFastTree(extra["metainfo"].GetString()),
                             *spotterExtra.MutableSpotterStats());
}

bool ParseSpotterStats(const NJson::TJsonValue& metaInfo,
                       TLogSpotter::TSpotterActivationInfo::TSpotterStats& spotterStats) {
    if (!metaInfo.IsMap()) {
        return false;
    }

    const auto& metaInfoMap = metaInfo.GetMap();

    if (const auto* confidences = metaInfoMap.FindPtr("confidences"); confidences && confidences->IsArray()) {
        for (const auto& confidence : confidences->GetArray()) {
            if (confidence.IsDouble()) {
                spotterStats.MutableConfidences()->Add(confidence.GetDouble());
            }
        }
    }

    if (const auto* freqFilterState = metaInfoMap.FindPtr("freq_filter_state");
        freqFilterState && freqFilterState->IsString()) {
        spotterStats.SetFreqFilterState(freqFilterState->GetString());
    }

    if (const auto* freqFilterConfidence = metaInfoMap.FindPtr("freq_filter_confidence");
        freqFilterConfidence && freqFilterConfidence->IsDouble()) {
        spotterStats.SetFreqFilterConfidence(freqFilterConfidence->GetDouble());
    }

    return true;
}

bool ParseAsrDebug(const NJson::TJsonValue& directive, TAsrDebug& asrDebug) {
    NJson::TJsonValue debug;

    if (!directive["debug"].IsString() || !NJson::ReadJsonTree(directive["debug"].GetString(), &debug) ||
        !debug.IsMap()) {
        return false;
    }

    google::protobuf::util::JsonParseOptions options;
    options.ignore_unknown_fields = true;

    auto& burstDetector = *asrDebug.MutableBurstDetector();
    google::protobuf::util::JsonStringToMessage(NJson::WriteJson(debug["StreamValidation"]["burst_detector"]),
                                                &burstDetector, options);

    auto& onlineValidation = *asrDebug.MutableOnlineValidation();
    google::protobuf::util::JsonStringToMessage(NJson::WriteJson(debug["StreamValidation"]["online_validation"]),
                                                &onlineValidation, options);

    {
        auto& streamValidationContext = *asrDebug.MutableStreamValidationContext();
        const auto& contextMap = debug["StreamValidation"]["context"].GetMap();

        if (const auto* submittedAsrFrontMs = contextMap.FindPtr("submitted_asr_front_ms");
            submittedAsrFrontMs && submittedAsrFrontMs->IsDouble()) {
            streamValidationContext.SetSubmittedAsrFrontMs(submittedAsrFrontMs->GetDouble());
        }

        if (const auto* maxAsrFrontMs = contextMap.FindPtr("max_asr_front_ms");
            maxAsrFrontMs && maxAsrFrontMs->IsDouble()) {
            streamValidationContext.SetMaxAsrFrontMs(maxAsrFrontMs->GetDouble());
        }

        if (const auto* spotterBackMs = contextMap.FindPtr("spotter_back_ms");
            spotterBackMs && spotterBackMs->IsDouble()) {
            streamValidationContext.SetSpotterBackMs(spotterBackMs->GetDouble());
        }
    }

    return true;
}

} // namespace NAlice::NWonderlogs
