#pragma once

#include <alice/wonderlogs/protos/uniproxy_prepared.pb.h>

#include <alice/library/json/json.h>

#include <util/datetime/base.h>
#include <util/generic/maybe.h>
#include <util/string/builder.h>

namespace NAlice::NWonderlogs {

namespace NImpl {

const NJson::TJsonValue& GetSession(const NJson::TJsonValue& json);
TMaybe<TString> ParseConnectSessionId(const NJson::TJsonValue& json);
TMaybe<TString> ParseClientIp(const NJson::TJsonValue& json);
TMaybe<bool> ParseDoNotUseUserLogs(const NJson::TJsonValue& json);

const NJson::TJsonValue& GetEvent(const NJson::TJsonValue& json);
const NJson::TJsonValue& GetPayload(const NJson::TJsonValue& eventOrDirective);
const NJson::TJsonValue& GetDirective(const NJson::TJsonValue& json);
const NJson::TJsonValue& GetInternalDirective(const NJson::TJsonValue& json);
const NJson::TJsonValue& GetHeader(const NJson::TJsonValue& eventOrDirectiveOrPayload);

} // namespace NImpl

inline constexpr TStringBuf SESSIONLOG = "SESSIONLOG: ";

enum class EUniproxyEventType {
    Unknown,
    RequestStat,
    MegamindRequest,
    MegamindResponse,
    SpotterValidation,
    SpotterStream,
    Stream,
    LogSpotter,
    MessageIdToConnectSessionId,
    MessageIdToEnvironment,
    MessageIdToClientIp,
    SynchronizeState,
    VoiceInput,
    AsrRecognize,
    AsrResult,
    MegamindTimings,
    TtsTimings,
    TtsGenerate,
    MessageIdToDoNotUseUserLogs,
    AsrDebug,
    TestIds,
};

template <typename TEvent>
struct TUniproxyParsedLog {
    TMaybe<TEvent> ParsedEvent;
    TMaybe<TUniproxyPrepared::TMessageIdToConnectSessionId> MessageIdToConnectSessionId;
    TMaybe<TUniproxyPrepared::TMessageIdToEnvironment> MessageIdToEnvironment;
    TMaybe<TUniproxyPrepared::TMessageIdToClientIp> MessageIdToClientIp;
    TMaybe<TUniproxyPrepared::TMessageIdToDoNotUseUserLogs> MessageIdToDoNotUseUserLogs;
    TVector<TUniproxyPrepared::TError> Errors;
};

class TUniproxyLogsParser {
public:
    // TODO(ran1s) think about deleting environment and logfellerTimestamp, delete messageDebugOnly
    TUniproxyLogsParser(const NJson::TJsonValue& json, const TMaybe<TUniproxyPrepared::TEnvironment>& environment,
                        TInstant logfellerTimestamp, const TStringBuf messageDebugOnly);

    EUniproxyEventType GetType() const;

    TUniproxyParsedLog<TUniproxyPrepared::TRequestStatWrapper> ParseRequestStat() const;
    TUniproxyParsedLog<TUniproxyPrepared::TMegamindRequest> ParseMegamindRequest() const;
    TUniproxyParsedLog<TUniproxyPrepared::TMegamindResponse> ParseMegamindResponse() const;
    TUniproxyParsedLog<TUniproxyPrepared::TTestIds> ParseTestIds() const;

private:
    static EUniproxyEventType DetermineType(const NJson::TJsonValue& json);

    struct TValueName {
        const TMaybe<TString>& Value;
        const TStringBuf Name;
    };

    static TStringBuilder GenerateErrorMessage(const TVector<TValueName>& fields);
    static TUniproxyPrepared::TError GenerateError(const TUniproxyPrepared::TError::EReason reason,
                                                   const TString& message, const TMaybe<TString>& uuid,
                                                   const TMaybe<TString>& messageId);

    void FillCommonParts(const TString& uuid, const TString& messageId, TMaybe<TInstant> timestampEvent,
                         TMaybe<TUniproxyPrepared::TMessageIdToConnectSessionId>& messageIdToConnectSessionId,
                         TMaybe<TUniproxyPrepared::TMessageIdToEnvironment>& messageIdToEnvironment,
                         TMaybe<TUniproxyPrepared::TMessageIdToClientIp>& messageIdToClientIp,
                         TMaybe<TUniproxyPrepared::TMessageIdToDoNotUseUserLogs>& messageIdToDoNotUseUserLogs) const;

    const NJson::TJsonValue& Json;
    const TMaybe<TUniproxyPrepared::TEnvironment>& Environment;
    TInstant LogfellerTimestamp;
    EUniproxyEventType EventType;
    const TStringBuf MessageDebugOnly;
};

TMaybe<TString> ParseUuid(const NJson::TJsonValue& json);
TMaybe<TInstant> ParseTimestampEvent(const NJson::TJsonValue& json);

bool ParseMegamindTimings(const NJson::TJsonValue& json, TMegamindTimings& megamindTimings);
bool ParseTtsTimings(const NJson::TJsonValue& json, TTtsTimings& ttsTimings);
bool ParseTtsGenerate(const NJson::TJsonValue& payload, TTtsGenerate& ttsGenerate);
bool ParseVoiceInput(const NJson::TJsonValue& payload, TVoiceInput& voiceInput);
bool ParseSpotterActivationInfo(const NJson::TJsonValue& payload, TLogSpotter::TSpotterActivationInfo& spotterExtra);
bool ParseSpotterStats(const NJson::TJsonValue& extra,
                       TLogSpotter::TSpotterActivationInfo::TSpotterStats& spotterStats);
bool ParseAsrDebug(const NJson::TJsonValue& directive, TAsrDebug& asrDebug);

} // namespace NAlice::NWonderlogs
