#include "uniproxy_prepared.h"

#include "ttls.h"

#include <alice/wonderlogs/library/builders/uniproxy.h>
#include <alice/wonderlogs/library/common/names.h>
#include <alice/wonderlogs/library/common/utils.h>
#include <alice/wonderlogs/library/parsers/uniproxy.h>
#include <alice/wonderlogs/library/yt/utils.h>
#include <alice/wonderlogs/protos/request_stat.pb.h>

#include <alice/library/client/protos/client_info.pb.h>
#include <alice/library/json/json.h>

#include <google/protobuf/util/message_differencer.h>

#include <library/cpp/json/json_writer.h>
#include <library/cpp/threading/future/async.h>
#include <library/cpp/threading/future/future.h>
#include <library/cpp/threading/future/subscription/wait_all_or_exception.h>
#include <library/cpp/yson/node/node_io.h>

#include <mapreduce/interface/log.h>
#include <mapreduce/yt/library/operation_tracker/operation_tracker.h>

#include <util/datetime/parser.h>
#include <util/generic/iterator_range.h>
#include <util/generic/size_literals.h>
#include <util/string/builder.h>

using namespace NAlice::NWonderlogs;

namespace {
using google::protobuf::Message;

/*
 * Создаёт таблицы с событиями и директивами из логов uniproxy на отрезке [from - shift, to):
 * RequestStat.
 * VinsRequest - директива логируется единожды на все паршиалы.
 * VinsResponse - директива логируется при отправке ответа клиенту.
 *
 */
class TUniproxyEventDirectiveMapper : public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<Message>> {
public:
    class TInputOutputTables {
    public:
        TInputOutputTables(const TVector<TString>& uniproxyEventTables, const TString& requestStatTable,
                           const TString& megamindRequestTable, const TString& megamindResponseTable,
                           const TString& spotterValidtionTable, const TString& spotterStreamTable,
                           const TString& streamTable, const TString& logSpotterTable,
                           const TString& messageIdToConnectSessionIdTable, const TString& messageIdToEnvironmentTable,
                           const TString& messageIdToClientIpIdTable, const TString& synchronizeStateTable,
                           const TString& voiceInputTable, const TString& asrRecognizeTable,
                           const TString& asrResultTable, const TString& megamindTimingsTable,
                           const TString& ttsTimingsTable, const TString& ttsGenerateTable,
                           const TString& messageIdToDoNotUseUserLogsTable, const TString& asrDebugTable,
                           const TString& testIdsTable, const TString& errorTable)
            : UniproxyEventTables(uniproxyEventTables)
            , RequestStatTable(requestStatTable)
            , MegamindRequestTable(megamindRequestTable)
            , MegamindResponseTable(megamindResponseTable)
            , SpotterValidationTable(spotterValidtionTable)
            , SpotterStreamTable(spotterStreamTable)
            , StreamTable(streamTable)
            , LogSpotterTable(logSpotterTable)
            , MessageIdToConnectSessionIdTable(messageIdToConnectSessionIdTable)
            , MessageIdToEnvironmentTable(messageIdToEnvironmentTable)
            , MessageIdToClientIpTable(messageIdToClientIpIdTable)
            , SynchronizeStateTable(synchronizeStateTable)
            , VoiceInputTable(voiceInputTable)
            , AsrRecognizeTable(asrRecognizeTable)
            , AsrResultTable(asrResultTable)
            , MegamindTimingsTable(megamindTimingsTable)
            , TtsTimingsTable(ttsTimingsTable)
            , TtsGenerateTable(ttsGenerateTable)
            , MessageIdToDoNotUseUserLogsTable(messageIdToDoNotUseUserLogsTable)
            , AsrDebugTable(asrDebugTable)
            , TestIdsTable(testIdsTable)
            , ErrorTable(errorTable) {
        }

        NYT::TMapOperationSpec AddToOperationSpec(NYT::TMapOperationSpec&& operationSpec) {
            for (const auto& table : UniproxyEventTables) {
                operationSpec = operationSpec.AddInput<NYT::TNode>(table);
            }
            return operationSpec.AddOutput<TUniproxyPrepared::TRequestStatWrapper>(RequestStatTable)
                .AddOutput<TUniproxyPrepared::TMegamindRequest>(MegamindRequestTable)
                .AddOutput<TUniproxyPrepared::TMegamindResponse>(MegamindResponseTable)
                .AddOutput<TUniproxyPrepared::TSpotterValidation>(SpotterValidationTable)
                .AddOutput<TUniproxyPrepared::TSpotterStream>(SpotterStreamTable)
                .AddOutput<TUniproxyPrepared::TStream>(StreamTable)
                .AddOutput<TUniproxyPrepared::TLogSpotter>(LogSpotterTable)
                .AddOutput<TUniproxyPrepared::TMessageIdToConnectSessionId>(MessageIdToConnectSessionIdTable)
                .AddOutput<TUniproxyPrepared::TMessageIdToEnvironment>(MessageIdToEnvironmentTable)
                .AddOutput<TUniproxyPrepared::TMessageIdToClientIp>(MessageIdToClientIpTable)
                .AddOutput<TUniproxyPrepared::TSynchronizeState>(SynchronizeStateTable)
                .AddOutput<TUniproxyPrepared::TVoiceInput>(VoiceInputTable)
                .AddOutput<TUniproxyPrepared::TAsrRecognize>(AsrRecognizeTable)
                .AddOutput<TUniproxyPrepared::TAsrResult>(AsrResultTable)
                .AddOutput<TUniproxyPrepared::TMegamindTimings>(MegamindTimingsTable)
                .AddOutput<TUniproxyPrepared::TTtsTimings>(TtsTimingsTable)
                .AddOutput<TUniproxyPrepared::TTtsGenerate>(TtsGenerateTable)
                .AddOutput<TUniproxyPrepared::TMessageIdToDoNotUseUserLogs>(MessageIdToDoNotUseUserLogsTable)
                .AddOutput<TUniproxyPrepared::TAsrDebug>(AsrDebugTable)
                .AddOutput<TUniproxyPrepared::TTestIds>(TestIdsTable)
                .AddOutput<TUniproxyPrepared::TError>(ErrorTable);
        }

        enum EOutIndices {
            RequestStat = 0,
            MegamindRequest = 1,
            MegamindResponse = 2,
            SpotterValidation = 3,
            SpotterStream = 4,
            Stream = 5,
            LogSpotter = 6,
            MessageIdToConnectSessionId = 7,
            MessageIdToEnvironment = 8,
            MessageIdToClientIp = 9,
            SynchronizeState = 10,
            VoiceInput = 11,
            AsrRecognize = 12,
            AsrResult = 13,
            MegamindTimings = 14,
            TtsTimings = 15,
            TtsGenerate = 16,
            MessageIdToDoNotUseUserLogs = 17,
            AsrDebug = 18,
            TestIds = 19,
            Error = 20,
        };

    private:
        const TVector<TString>& UniproxyEventTables;
        const TString& RequestStatTable;
        const TString& MegamindRequestTable;
        const TString& MegamindResponseTable;
        const TString& SpotterValidationTable;
        const TString& SpotterStreamTable;
        const TString& StreamTable;
        const TString& LogSpotterTable;
        const TString& MessageIdToConnectSessionIdTable;
        const TString& MessageIdToEnvironmentTable;
        const TString& MessageIdToClientIpTable;
        const TString& SynchronizeStateTable;
        const TString& VoiceInputTable;
        const TString& AsrRecognizeTable;
        const TString& AsrResultTable;
        const TString& MegamindTimingsTable;
        const TString& TtsTimingsTable;
        const TString& TtsGenerateTable;
        const TString& MessageIdToDoNotUseUserLogsTable;
        const TString& AsrDebugTable;
        const TString& TestIdsTable;
        const TString& ErrorTable;
    };

    Y_SAVELOAD_JOB(TimestampFrom, TimestampTo, RequestsShift);
    TUniproxyEventDirectiveMapper() = default;
    TUniproxyEventDirectiveMapper(const TInstant& timestampFrom, const TInstant& timestampTo,
                                  const TDuration& requestsShift)
        : TimestampFrom(timestampFrom)
        , TimestampTo(timestampTo)
        , RequestsShift(requestsShift) {
    }

    void Do(TReader* reader, TWriter* writer) override {
        const auto logError = [writer](const TUniproxyPrepared::TError::EReason reason, const TString& message,
                                       const TMaybe<TString>& uuid, const TMaybe<TString>& messageId) {
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
            if (auto setraceUrl =
                    TryGenerateSetraceUrl({error.HasMessageId() ? error.GetMessageId() : TMaybe<TString>{},
                                           error.HasUuid() ? error.GetUuid() : TMaybe<TString>{}})) {
                error.SetSetraceUrl(*setraceUrl);
            }
            writer->AddRow(error, TInputOutputTables::EOutIndices::Error);
        };

        for (auto& cursor : *reader) {
            const auto& row = cursor.GetRow();

            TStringBuf message = row["message"].AsString();
            const auto timestamp = TInstant::Seconds(row["_logfeller_timestamp"].AsUint64());
            NJson::TJsonValue json;

            if (!message.SkipPrefix(SESSIONLOG)) {
                continue;
            }

            if (!NJson::ReadJsonTree(message, &json)) {
                logError(TUniproxyPrepared::TError::R_INVALID_JSON,
                         TStringBuilder{} << "Invalid json: " << message << " timestamp: " << timestamp,
                         /* uuid= */ {}, /* messageId= */ {});
                continue;
            }
            // TODO(ran1s) check correctness

            const auto uuid = ParseUuid(json);
            const auto timestampEvent = ParseTimestampEvent(json);

            const auto connectSessionId = MaybeStringFromJson(json["Session"]["SessionId"]);
            TMaybe<TString> messageId;
            TMaybe<TString> clientIp = MaybeStringFromJson(json["Session"]["IpAddr"]);
            TMaybe<bool> doNotUseUserLogs = MaybeBoolFromJson(json["Session"]["DoNotUseUserLogs"]);

            if (timestampEvent && SkipRequest(*timestampEvent, TimestampFrom, TimestampTo, RequestsShift)) {
                continue;
            }

            TMaybe<TUniproxyPrepared::TEnvironment> environment;
            if (row["qloud_application"].IsString() && row["qloud_project"].IsString()) {
                TUniproxyPrepared::TEnvironment env;
                env.SetQloudApplication(row["qloud_application"].AsString());
                env.SetQloudProject(row["qloud_project"].AsString());
                environment = env;
            }

            const auto generateErrorMessage =
                [](const TVector<std::tuple<const TMaybe<TString>&, const TStringBuf&>>& fields) {
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
                };

            const auto logCommonParts =
                [writer](const TVector<TUniproxyPrepared::TError>& errors,
                         TMaybe<TUniproxyPrepared::TMessageIdToConnectSessionId> messageIdToConnectSessionId,
                         TMaybe<TUniproxyPrepared::TMessageIdToEnvironment> messageIdToEnvironment,
                         TMaybe<TUniproxyPrepared::TMessageIdToClientIp> messageIdToClientIp,
                         TMaybe<TUniproxyPrepared::TMessageIdToDoNotUseUserLogs> messageIdToDoNotUseUserLogs) {
                    for (const auto& error : errors) {
                        writer->AddRow(error, TInputOutputTables::EOutIndices::Error);
                    }
                    if (messageIdToConnectSessionId) {
                        writer->AddRow(*messageIdToConnectSessionId,
                                       TInputOutputTables::EOutIndices::MessageIdToConnectSessionId);
                    }
                    if (messageIdToEnvironment) {
                        writer->AddRow(*messageIdToEnvironment,
                                       TInputOutputTables::EOutIndices::MessageIdToEnvironment);
                    }
                    if (messageIdToClientIp) {
                        writer->AddRow(*messageIdToClientIp, TInputOutputTables::EOutIndices::MessageIdToClientIp);
                    }
                    if (messageIdToDoNotUseUserLogs) {
                        writer->AddRow(*messageIdToDoNotUseUserLogs,
                                       TInputOutputTables::EOutIndices::MessageIdToDoNotUseUserLogs);
                    }
                };

            TUniproxyLogsParser logsParser(json, environment, timestamp, message);

            // TODO(ran1s) test parsing events and directives
            if (const auto* eventPtr = json.GetMap().FindPtr("Event")) {
                const auto& event = (*eventPtr)["event"];
                const auto& header = event["header"];

                if (EUniproxyEventType::RequestStat == logsParser.GetType()) {
                    const auto parsedLog = logsParser.ParseRequestStat();

                    if (parsedLog.ParsedEvent) {
                        writer->AddRow(*parsedLog.ParsedEvent, TInputOutputTables::EOutIndices::RequestStat);
                    }

                    logCommonParts(parsedLog.Errors, parsedLog.MessageIdToConnectSessionId,
                                   parsedLog.MessageIdToEnvironment, parsedLog.MessageIdToClientIp,
                                   parsedLog.MessageIdToDoNotUseUserLogs);

                    // TODO(ran1s) delete
                    continue;

                } else if (header["namespace"] == "Log" && header["name"] == "Spotter") {
                    const auto& payload = event["payload"];
                    messageId = MaybeStringFromJson(payload["vinsMessageId"]);
                    const auto errorLogMessageId = messageId;
                    bool realMessageId = true;
                    // In case it does not have message_id we need to generate in order to save the voice
                    if (!messageId) {
                        messageId = CreateGuidAsString();
                        realMessageId = false;
                    }
                    if (uuid && !uuid->empty() && messageId && !messageId->empty()) {
                        TUniproxyPrepared::TLogSpotter logSpotter;
                        logSpotter.SetUuid(*uuid);
                        logSpotter.SetMessageId(*messageId);
                        logSpotter.SetRealMessageId(realMessageId);
                        if (timestampEvent) {
                            logSpotter.SetTimestampLogMs(timestampEvent->MilliSeconds());
                        }
                        if (header["messageId"].IsString() && !header["messageId"].GetString().empty()) {
                            logSpotter.MutableValue()->SetMessageId(header["messageId"].GetString());
                            logSpotter.SetSpotterStreamId(header["messageId"].GetString());
                        } else {
                            logSpotter.SetSpotterStreamId(CreateGuidAsString());
                            logError(TUniproxyPrepared::TError::R_INVALID_VALUE,
                                     TStringBuilder{} << "Invalid spotter_message_id from LogSpotter event", uuid,
                                     errorLogMessageId);
                        }
                        if (payload["transcript"].IsString()) {
                            logSpotter.MutableValue()->SetTranscript(payload["transcript"].GetString());
                        } else {
                            logError(TUniproxyPrepared::TError::R_INVALID_VALUE,
                                     TStringBuilder{} << "Invalid transcript from LogSpotter event", uuid,
                                     errorLogMessageId);
                        }
                        if (payload.IsMap()) {
                            if (const auto* topic = payload.GetMap().FindPtr("topic"); topic && topic->IsString()) {
                                logSpotter.MutableValue()->SetTopic(topic->GetString());
                            }

                            if (const auto* source = payload.GetMap().FindPtr("source");
                                source && source->IsString()) {
                                logSpotter.MutableValue()->SetSource(source->GetString());
                            }

                            if (const auto* firmware = payload.GetMap().FindPtr("firmware");
                                firmware && firmware->IsString()) {
                                logSpotter.MutableValue()->SetFirmware(firmware->GetString());
                            }
                        }
                        TLogSpotter::TSpotterActivationInfo spotterActivationInfo;
                        if (!ParseSpotterActivationInfo(payload["extra"], spotterActivationInfo)) {
                            const auto errorMessage = TStringBuilder{}
                                                      << "Could not parse SpotterActivationInfo from LogSpotter: "
                                                      << message << " timestamp: " << timestamp;
                            logError(TUniproxyPrepared::TError::R_INVALID_JSON, errorMessage, uuid, messageId);
                        } else {
                            *logSpotter.MutableValue()->MutableSpotterActivationInfo() = spotterActivationInfo;
                        }

                        writer->AddRow(logSpotter, TInputOutputTables::EOutIndices::LogSpotter);
                    } else {
                        auto errorMessage =
                            generateErrorMessage({std::tie(uuid, UUID), std::tie(messageId, MESSAGE_ID)});
                        errorMessage << "from Log Spotter event: " << message << " timestamp: " << timestamp;
                        logError(TUniproxyPrepared::TError::R_INVALID_VALUE, errorMessage, uuid, errorLogMessageId);
                    }
                } else if (header["namespace"] == "System" && header["name"] == "SynchronizeState") {
                    const auto& payload = event["payload"];
                    if (uuid && !uuid->empty() && connectSessionId && !connectSessionId->empty()) {
                        TUniproxyPrepared::TSynchronizeState synchronizeState;
                        if (timestampEvent) {
                            synchronizeState.SetTimestampLogMs(timestampEvent->MilliSeconds());
                        }
                        if (connectSessionId) {
                            synchronizeState.SetConnectSessionId(*connectSessionId);
                        }
                        synchronizeState.SetUuid(*uuid);
                        if (payload.Has("auth_token") && payload["auth_token"].IsString()) {
                            synchronizeState.MutableValue()->SetAuthToken(payload["auth_token"].GetString());
                        }
                        if (const auto* appPtr = payload.GetValueByPath("vins.application");
                            appPtr && appPtr->IsMap()) {
                            const auto& app = *appPtr;
                            auto& appProto = *synchronizeState.MutableValue()->MutableApplication();
                            if (const auto appId = app.GetMap().FindPtr("app_id"); appId && appId->IsString()) {
                                appProto.SetAppId(appId->GetString());
                            }
                            if (const auto deviceId = app.GetMap().FindPtr("device_id");
                                deviceId && deviceId->IsString()) {
                                appProto.SetDeviceId(deviceId->GetString());
                            }
                            if (const auto deviceManufacturer = app.GetMap().FindPtr("device_manufacturer");
                                deviceManufacturer && deviceManufacturer->IsString()) {
                                appProto.SetDeviceManufacturer(deviceManufacturer->GetString());
                            }
                            if (const auto deviceModel = app.GetMap().FindPtr("device_model");
                                deviceModel && deviceModel->IsString()) {
                                appProto.SetDeviceModel(deviceModel->GetString());
                            }
                            if (const auto osVersion = app.GetMap().FindPtr("os_version");
                                osVersion && osVersion->IsString()) {
                                appProto.SetOsVersion(osVersion->GetString());
                            }
                            if (const auto platform = app.GetMap().FindPtr("platform");
                                platform && platform->IsString()) {
                                appProto.SetPlatform(platform->GetString());
                            }
                            if (const auto appVersion = app.GetMap().FindPtr("app_version");
                                appVersion && appVersion->IsString()) {
                                appProto.SetAppVersion(appVersion->GetString());
                            }
                        }

                        writer->AddRow(synchronizeState, TInputOutputTables::EOutIndices::SynchronizeState);
                    } else {
                        auto errorMessage = generateErrorMessage(
                            {std::tie(uuid, UUID), std::tie(connectSessionId, CONNECT_SESSION_ID)});
                        errorMessage << "from SynchronizeState event: " << message << " timestamp: " << timestamp;
                        logError(TUniproxyPrepared::TError::R_INVALID_VALUE, errorMessage, uuid, /* messageId= */ {});
                    }
                } else if (header["namespace"] == "Vins" && header["name"] == "VoiceInput") {
                    const auto& payload = event["payload"];
                    TUniproxyPrepared::TVoiceInput voiceInput;
                    messageId = MaybeStringFromJson(header["messageId"]);

                    if (uuid && !uuid->empty() && messageId && !messageId->empty() &&
                        ParseVoiceInput(payload, *voiceInput.MutableValue())) {
                        if (timestampEvent) {
                            voiceInput.SetTimestampLogMs(timestampEvent->MilliSeconds());
                        }
                        voiceInput.SetUuid(*uuid);
                        voiceInput.SetMessageId(*messageId);
                        writer->AddRow(voiceInput, TInputOutputTables::EOutIndices::VoiceInput);
                    } else {
                        auto errorMessage =
                            generateErrorMessage({std::tie(uuid, UUID), std::tie(messageId, MESSAGE_ID)});
                        errorMessage << "from VoiceInput event: " << message << " timestamp: " << timestamp;
                        logError(TUniproxyPrepared::TError::R_INVALID_VALUE, errorMessage, uuid, messageId);
                    }
                } else if (header["namespace"] == "ASR" && header["name"] == "Recognize") {
                    const auto& payload = event["payload"];
                    messageId = MaybeStringFromJson(header["messageId"]);
                    if (uuid && !uuid->empty() && messageId && !messageId->empty()) {
                        TUniproxyPrepared::TAsrRecognize asrRecognize;
                        if (timestampEvent) {
                            asrRecognize.SetTimestampLogMs(timestampEvent->MilliSeconds());
                        }
                        asrRecognize.SetUuid(*uuid);
                        asrRecognize.SetMessageId(*messageId);
                        if (payload.IsMap()) {
                            if (const auto topic = payload.GetMap().FindPtr("topic"); topic && topic->IsString()) {
                                asrRecognize.MutableValue()->SetTopic(topic->GetString());
                            }
                        }
                        writer->AddRow(asrRecognize, TInputOutputTables::EOutIndices::AsrRecognize);
                    } else {
                        auto errorMessage =
                            generateErrorMessage({std::tie(uuid, UUID), std::tie(messageId, MESSAGE_ID)});
                        errorMessage << "from ASR Recognize event: " << message << " timestamp: " << timestamp;
                        logError(TUniproxyPrepared::TError::R_INVALID_VALUE, errorMessage, uuid, messageId);
                    }
                } else if (header["namespace"] == "TTS" && header["name"] == "Generate") {
                    const auto& payload = event["payload"];
                    messageId = MaybeStringFromJson(header["messageId"]);
                    if (uuid && !uuid->empty() && messageId && !messageId->empty()) {
                        TUniproxyPrepared::TTtsGenerate ttsGenerate;

                        if (timestampEvent) {
                            ttsGenerate.SetTimestampLogMs(timestampEvent->MilliSeconds());
                        }
                        ttsGenerate.SetUuid(*uuid);
                        ttsGenerate.SetMessageId(*messageId);

                        if (ParseTtsGenerate(payload, *ttsGenerate.MutableValue())) {
                            writer->AddRow(ttsGenerate, TInputOutputTables::EOutIndices::TtsGenerate);
                        } else {
                            const auto errorMessage = TStringBuilder{}
                                                      << "Could not parse TTS Generate event payload: " << message
                                                      << " timestamp: " << timestamp;
                            logError(TUniproxyPrepared::TError::R_INVALID_JSON, errorMessage, uuid, *messageId);
                        }

                    } else {
                        auto errorMessage =
                            generateErrorMessage({std::tie(uuid, UUID), std::tie(messageId, MESSAGE_ID)});
                        errorMessage << "from TTS Generate event: " << message << " timestamp: " << timestamp;
                        logError(TUniproxyPrepared::TError::R_INVALID_VALUE, errorMessage, uuid, messageId);
                    }
                }
            } else if (const auto* directivePtr = json.GetMap().FindPtr("Directive")) {
                const auto& directive = *directivePtr;

                if (EUniproxyEventType::MegamindRequest == logsParser.GetType()) {
                    const auto parsedLog = logsParser.ParseMegamindRequest();

                    if (parsedLog.ParsedEvent) {
                        writer->AddRow(*parsedLog.ParsedEvent, TInputOutputTables::EOutIndices::MegamindRequest);
                    }

                    logCommonParts(parsedLog.Errors, parsedLog.MessageIdToConnectSessionId,
                                   parsedLog.MessageIdToEnvironment, parsedLog.MessageIdToClientIp,
                                   parsedLog.MessageIdToDoNotUseUserLogs);

                    // TODO(ran1s) delete
                    continue;
                } else if (directive["type"] == "AsrCoreDebug" && directive["backend"] == "spotter") {
                    messageId = MaybeStringFromJson(directive["ForEvent"]);

                    if (uuid && !uuid->empty() && messageId && !messageId->empty()) {
                        TUniproxyPrepared::TAsrDebug asrDebug;
                        asrDebug.SetMessageId(*messageId);
                        asrDebug.SetUuid(*uuid);
                        if (timestampEvent) {
                            asrDebug.SetTimestampLogMs(timestampEvent->MilliSeconds());
                        }

                        if (ParseAsrDebug(directive, *asrDebug.MutableValue())) {
                            writer->AddRow(asrDebug, TInputOutputTables::EOutIndices::AsrDebug);
                        } else {
                            const auto errorMessage = TStringBuilder{}
                                                      << "Could not parse json from AsrCoreDebug: " << message
                                                      << " timestamp: " << timestamp;
                            logError(TUniproxyPrepared::TError::R_INVALID_JSON, errorMessage, uuid, messageId);
                        }
                    } else {
                        auto errorMessage =
                            generateErrorMessage({std::tie(uuid, UUID), std::tie(messageId, MESSAGE_ID)});
                        errorMessage << "from AsrCoreDebug directive: " << message << " timestamp: " << timestamp;
                        logError(TUniproxyPrepared::TError::R_INVALID_VALUE, errorMessage, uuid, messageId);
                    }
                } else if (const auto& header = directive["directive"]["header"];
                           EUniproxyEventType::MegamindResponse == logsParser.GetType()) {
                    const auto parsedLog = logsParser.ParseMegamindResponse();

                    if (parsedLog.ParsedEvent) {
                        writer->AddRow(*parsedLog.ParsedEvent, TInputOutputTables::EOutIndices::MegamindResponse);
                    }

                    logCommonParts(parsedLog.Errors, parsedLog.MessageIdToConnectSessionId,
                                   parsedLog.MessageIdToEnvironment, parsedLog.MessageIdToClientIp,
                                   parsedLog.MessageIdToDoNotUseUserLogs);

                    // TODO(ran1s) delete
                    continue;

                } else if (header["namespace"] == "Spotter" && header["name"] == "Validation") {
                    messageId = MaybeStringFromJson(header["refMessageId"]);

                    if (uuid && messageId && !uuid->empty() && !messageId->empty()) {
                        TUniproxyPrepared::TSpotterValidation spotterValidation;
                        spotterValidation.SetUuid(*uuid);
                        spotterValidation.SetMessageId(*messageId);
                        if (timestampEvent) {
                            spotterValidation.SetTimestampLogMs(timestampEvent->MilliSeconds());
                        }
                        const auto& payload = directive["directive"]["payload"];
                        TMaybe<bool> modelResult;
                        TMaybe<bool> finalResult;
                        if (const auto& valid = payload["valid"]; valid.IsInteger()) {
                            modelResult = static_cast<bool>(valid.GetInteger());
                        }
                        if (const auto& result = payload["result"]; result.IsInteger()) {
                            finalResult = static_cast<bool>(result.GetInteger());
                        }

                        if (modelResult) {
                            spotterValidation.MutableValue()->SetValid(*modelResult);
                            spotterValidation.MutableValue()->SetModelResult(*modelResult);
                        }

                        if (finalResult) {
                            spotterValidation.MutableValue()->SetResult(*finalResult);
                            spotterValidation.MutableValue()->SetFinalResult(*finalResult);
                        }

                        writer->AddRow(spotterValidation, TInputOutputTables::EOutIndices::SpotterValidation);
                    } else {
                        auto errorMessage =
                            generateErrorMessage({std::tie(uuid, UUID), std::tie(messageId, MESSAGE_ID)});
                        errorMessage << "from SpotterValidation directive: " << message << " timestamp: " << timestamp;
                        logError(TUniproxyPrepared::TError::R_INVALID_VALUE, errorMessage, uuid, messageId);
                    }
                } else if (header["namespace"] == "ASR" && header["name"] == "Result") {
                    messageId = MaybeStringFromJson(header["refMessageId"]);
                    if (uuid && messageId && !uuid->empty() && !messageId->empty()) {
                        TUniproxyPrepared::TAsrResult asrResult;
                        asrResult.SetUuid(*uuid);
                        asrResult.SetMessageId(*messageId);
                        if (timestampEvent) {
                            asrResult.SetTimestampLogMs(timestampEvent->MilliSeconds());
                        }
                        const auto& payload = directive["directive"]["payload"];

                        if (payload.IsMap()) {
                            if (const auto* topic = payload.GetValueByPath("metainfo.topic");
                                topic && topic->IsString()) {
                                asrResult.MutableValue()->SetTopic(topic->GetString());
                            }
                            if (const auto* lingwareVersion = payload.GetValueByPath("metainfo.version");
                                lingwareVersion && lingwareVersion->IsString()) {
                                asrResult.MutableValue()->SetLingwareVersion(lingwareVersion->GetString());
                            }
                        }
                        writer->AddRow(asrResult, TInputOutputTables::EOutIndices::AsrResult);
                    } else {
                        auto errorMessage =
                            generateErrorMessage({std::tie(uuid, UUID), std::tie(messageId, MESSAGE_ID)});
                        errorMessage << "from ASR Result directive: " << message << " timestamp: " << timestamp;
                        logError(TUniproxyPrepared::TError::R_INVALID_VALUE, errorMessage, uuid, messageId);
                    }
                } else if (const auto& rootHeader = directive["header"];
                           rootHeader["namespace"] == "System" && rootHeader["name"] == "UniproxyVinsTimings") {
                    messageId = MaybeStringFromJson(rootHeader["refMessageId"]);
                    if (uuid && messageId && !uuid->empty() && !messageId->empty()) {
                        TUniproxyPrepared::TMegamindTimings megamindTimings;
                        megamindTimings.SetUuid(*uuid);
                        megamindTimings.SetMessageId(*messageId);
                        if (timestampEvent) {
                            megamindTimings.SetTimestampLogMs(timestampEvent->MilliSeconds());
                        }
                        if (ParseMegamindTimings(json, *megamindTimings.MutableValue())) {
                            writer->AddRow(megamindTimings, TInputOutputTables::EOutIndices::MegamindTimings);
                        } else {
                            const auto errorMessage = TStringBuilder{}
                                                      << "Could not convert json to proto from UniproxyVinsTimings: "
                                                      << message << " timestamp: " << timestamp;
                            logError(TUniproxyPrepared::TError::R_FAILED_CONVERT_JSON_TO_PROTO, errorMessage, uuid,
                                     *messageId);
                        }
                    } else {
                        auto errorMessage =
                            generateErrorMessage({std::tie(uuid, UUID), std::tie(messageId, MESSAGE_ID)});
                        errorMessage << "from UniproxyVinsTimings directive: " << message
                                     << " timestamp: " << timestamp;
                        logError(TUniproxyPrepared::TError::R_INVALID_VALUE, errorMessage, uuid, messageId);
                    }
                } else if (rootHeader["namespace"] == "System" && rootHeader["name"] == "UniproxyTTSTimings") {
                    messageId = MaybeStringFromJson(rootHeader["refMessageId"]);
                    if (uuid && messageId && !uuid->empty() && !messageId->empty()) {
                        TUniproxyPrepared::TTtsTimings ttsTimings;
                        ttsTimings.SetUuid(*uuid);
                        ttsTimings.SetMessageId(*messageId);
                        if (timestampEvent) {
                            ttsTimings.SetTimestampLogMs(timestampEvent->MilliSeconds());
                        }
                        if (ParseTtsTimings(json, *ttsTimings.MutableValue())) {
                            writer->AddRow(ttsTimings, TInputOutputTables::EOutIndices::TtsTimings);
                        } else {
                            const auto errorMessage = TStringBuilder{}
                                                      << "Could not convert json to proto from UniproxyTTSTimings: "
                                                      << message << " timestamp: " << timestamp;
                            logError(TUniproxyPrepared::TError::R_FAILED_CONVERT_JSON_TO_PROTO, errorMessage, uuid,
                                     *messageId);
                        }
                    } else {
                        auto errorMessage =
                            generateErrorMessage({std::tie(uuid, UUID), std::tie(messageId, MESSAGE_ID)});
                        errorMessage << "from UniproxyTTSTimings directive: " << message
                                     << " timestamp: " << timestamp;
                        logError(TUniproxyPrepared::TError::R_INVALID_VALUE, errorMessage, uuid, messageId);
                    }
                } else if (EUniproxyEventType::TestIds == logsParser.GetType()) {
                    const auto parsedLog = logsParser.ParseTestIds();

                    if (parsedLog.ParsedEvent) {
                        writer->AddRow(*parsedLog.ParsedEvent, TInputOutputTables::EOutIndices::TestIds);
                    }

                    logCommonParts(parsedLog.Errors, parsedLog.MessageIdToConnectSessionId,
                                   parsedLog.MessageIdToEnvironment, parsedLog.MessageIdToClientIp,
                                   parsedLog.MessageIdToDoNotUseUserLogs);

                    continue;
                }
            } else if (const auto* streamPtr = json.GetMap().FindPtr("Stream")) {
                const auto& stream = *streamPtr;
                messageId = MaybeStringFromJson(stream["vins_message_id"]);
                if (!messageId) {
                    messageId = MaybeStringFromJson(stream["messageId"]);
                }
                const auto errorLogMessageId = messageId;
                bool realMessageId = true;
                // In case it does not have message_id we need to generate in order to save the voice
                if (!messageId) {
                    messageId = CreateGuidAsString();
                    realMessageId = false;
                }
                const auto isSpotter = stream["isSpotter"].GetBoolean();

                if (uuid && messageId && !uuid->empty() && !messageId->empty()) {
                    if (isSpotter) {
                        TUniproxyPrepared::TSpotterStream spotterStream;
                        spotterStream.SetUuid(*uuid);
                        spotterStream.SetMessageId(*messageId);
                        spotterStream.SetRealMessageId(realMessageId);
                        if (timestampEvent) {
                            spotterStream.SetTimestampLogMs(timestampEvent->MilliSeconds());
                        }
                        if (stream["MDS"].IsString() && stream["format"].IsString()) {
                            spotterStream.MutableValue()->SetMds(stream["MDS"].GetString());
                            spotterStream.MutableValue()->SetFormat(stream["format"].GetString());
                            writer->AddRow(spotterStream, TInputOutputTables::EOutIndices::SpotterStream);
                        } else {
                            const auto errorMessage =
                                TStringBuilder{} << "Could not get MDS and format from Spotter Stream: " << message
                                                 << " timestamp: " << timestamp;
                            logError(TUniproxyPrepared::TError::R_INVALID_VALUE, errorMessage, uuid,
                                     errorLogMessageId);
                        }
                    } else {
                        TUniproxyPrepared::TStream streamProto;
                        streamProto.SetUuid(*uuid);
                        streamProto.SetMessageId(*messageId);
                        streamProto.SetRealMessageId(realMessageId);
                        if (stream["messageId"].IsString() && !stream["messageId"].GetString().empty()) {
                            streamProto.SetSpotterStreamId(stream["messageId"].GetString());
                        } else {
                            streamProto.SetSpotterStreamId(CreateGuidAsString());
                        }
                        if (timestampEvent) {
                            streamProto.SetTimestampLogMs(timestampEvent->MilliSeconds());
                        }
                        if (stream["MDS"].IsString() && stream["format"].IsString()) {
                            streamProto.MutableValue()->SetMds(stream["MDS"].GetString());
                            streamProto.MutableValue()->SetFormat(stream["format"].GetString());
                            writer->AddRow(streamProto, TInputOutputTables::EOutIndices::Stream);
                        } else {
                            const auto errorMessage = TStringBuilder{}
                                                      << "Could not get MDS and format from Stream: " << message
                                                      << " timestamp: " << timestamp;
                            logError(TUniproxyPrepared::TError::R_INVALID_VALUE, errorMessage, uuid,
                                     errorLogMessageId);
                        }
                    }
                } else {
                    auto errorMessage =
                        generateErrorMessage({std::tie(uuid, UUID), std::tie(errorLogMessageId, MESSAGE_ID)});
                    errorMessage << "from Stream: " << message << " timestamp: " << timestamp;
                    logError(TUniproxyPrepared::TError::R_INVALID_VALUE, errorMessage, uuid, errorLogMessageId);
                }
            }
            {
                TUniproxyPrepared::TMessageIdToConnectSessionId messageIdToConnectSessionId;
                if (uuid && !uuid->empty() && messageId && !messageId->empty() && connectSessionId &&
                    !connectSessionId->empty()) {
                    messageIdToConnectSessionId.SetUuid(*uuid);
                    messageIdToConnectSessionId.SetMessageId(*messageId);
                    messageIdToConnectSessionId.SetConnectSessionId(*connectSessionId);
                    if (timestampEvent) {
                        messageIdToConnectSessionId.SetTimestampLogMs(timestampEvent->MilliSeconds());
                    }

                    writer->AddRow(messageIdToConnectSessionId,
                                   TInputOutputTables::EOutIndices::MessageIdToConnectSessionId);
                }
            }
            {
                TUniproxyPrepared::TMessageIdToEnvironment messageIdToEnvironment;
                if (uuid && !uuid->empty() && messageId && !messageId->empty() && environment) {
                    messageIdToEnvironment.SetUuid(*uuid);
                    messageIdToEnvironment.SetMessageId(*messageId);
                    *messageIdToEnvironment.MutableEnvironment() = *environment;
                    if (timestampEvent) {
                        messageIdToEnvironment.SetTimestampLogMs(timestampEvent->MilliSeconds());
                    }

                    writer->AddRow(messageIdToEnvironment, TInputOutputTables::EOutIndices::MessageIdToEnvironment);
                }
            }

            {
                TUniproxyPrepared::TMessageIdToClientIp messageIdToClientIp;
                if (uuid && !uuid->empty() && messageId && !messageId->empty() && clientIp && !clientIp->empty()) {
                    messageIdToClientIp.SetUuid(*uuid);
                    messageIdToClientIp.SetMessageId(*messageId);
                    messageIdToClientIp.SetClientIp(*clientIp);
                    if (timestampEvent) {
                        messageIdToClientIp.SetTimestampLogMs(timestampEvent->MilliSeconds());
                    }

                    writer->AddRow(messageIdToClientIp, TInputOutputTables::EOutIndices::MessageIdToClientIp);
                }
            }

            {
                TUniproxyPrepared::TMessageIdToDoNotUseUserLogs messageIdToDoNotUseUserLogs;
                if (uuid && !uuid->empty() && messageId && !messageId->empty() && doNotUseUserLogs) {
                    messageIdToDoNotUseUserLogs.SetUuid(*uuid);
                    messageIdToDoNotUseUserLogs.SetMessageId(*messageId);
                    messageIdToDoNotUseUserLogs.SetDoNotUseUserLogs(*doNotUseUserLogs);
                    if (timestampEvent) {
                        messageIdToDoNotUseUserLogs.SetTimestampLogMs(timestampEvent->MilliSeconds());
                    }

                    writer->AddRow(messageIdToDoNotUseUserLogs,
                                   TInputOutputTables::EOutIndices::MessageIdToDoNotUseUserLogs);
                }
            }
        }
    }

private:
    TInstant TimestampFrom;
    TInstant TimestampTo;
    TDuration RequestsShift;
};

/*
 * Редьюсит RequestStat, VinsRequest, VinsResponse, эвристику client_retry_number по message_id
 *
 * Если запрос без ответа находится на отрезка [to - shift, to), то мы его убираем. Если он останется без ответа,
 * то попадёт в следующее окно.
 *
 */
class TUniproxyPreparedReducer : public NYT::IReducer<NYT::TTableReader<Message>, NYT::TTableWriter<Message>> {
public:
    class TInputOutputTables {
    public:
        TInputOutputTables(const TString& requestStatTable, const TString& megamindRequestTable,
                           const TString& megamindResponseTable, const TString& clientRetryTable,
                           const TString& spotterValidationTable, const TString& spotterStreamTable,
                           const TString& streamTable, const TString& messageIdToConnectSessionIdTable,
                           const TString& messageIdToEnvironmentTable, const TString& messageIdToClientIpTable,
                           const TString& voiceInputTable, const TString& asrRecognizeTable,
                           const TString& asrResultTable, const TString& synchronizeStateWithMessageIdTable,
                           const TString& megamindTimingsTable, const TString& ttsTimingsTable,
                           const TString& ttsGenerateTable, const TString& messageIdToDoNotUseUserLogsTable,
                           const TString& asrDebugTable, const TString& logSpotterWithStreamsTable,
                           const TString& testIdsTable, const TString& uniproxyPreparedTable,
                           const TString& errorTable)
            : RequestStatTable(requestStatTable)
            , MegamindRequestTable(megamindRequestTable)
            , MegamindResponseTable(megamindResponseTable)
            , ClientRetryTable(clientRetryTable)
            , SpotterValidationTable(spotterValidationTable)
            , SpotterStreamTable(spotterStreamTable)
            , StreamTable(streamTable)
            , MessageIdToConnectSessionIdTable(messageIdToConnectSessionIdTable)
            , MessageIdToEnvironmentTable(messageIdToEnvironmentTable)
            , MessageIdToClientIpTable(messageIdToClientIpTable)
            , VoiceInputTable(voiceInputTable)
            , AsrRecognizeTable(asrRecognizeTable)
            , AsrResultTable(asrResultTable)
            , SynchronizeStateWithMessageIdTable(synchronizeStateWithMessageIdTable)
            , MegamindTimingsTable(megamindTimingsTable)
            , TtsTimingsTable(ttsTimingsTable)
            , TtsGenerateTable(ttsGenerateTable)
            , MessageIdToDoNotUseUserLogsTable(messageIdToDoNotUseUserLogsTable)
            , AsrDebugTable(asrDebugTable)
            , LogSpotterWithStreamsTable(logSpotterWithStreamsTable)
            , TestIdsTable(testIdsTable)
            , UniproxyPreparedTable(uniproxyPreparedTable)
            , ErrorTable(errorTable) {
        }

        NYT::TReduceOperationSpec AddToOperationSpec(NYT::TReduceOperationSpec&& operationSpec) {
            return operationSpec.AddInput<TUniproxyPrepared::TMegamindRequest>(MegamindRequestTable)
                .AddInput<TUniproxyPrepared::TMegamindResponse>(MegamindResponseTable)
                .AddInput<TUniproxyPrepared::TRequestStatWrapper>(RequestStatTable)
                .AddInput<TUniproxyPrepared::TClientRetry>(ClientRetryTable)
                .AddInput<TUniproxyPrepared::TSpotterValidation>(SpotterValidationTable)
                .AddInput<TUniproxyPrepared::TSpotterStream>(SpotterStreamTable)
                .AddInput<TUniproxyPrepared::TStream>(StreamTable)
                .AddInput<TUniproxyPrepared::TMessageIdToConnectSessionId>(MessageIdToConnectSessionIdTable)
                .AddInput<TUniproxyPrepared::TMessageIdToEnvironment>(MessageIdToEnvironmentTable)
                .AddInput<TUniproxyPrepared::TMessageIdToClientIp>(MessageIdToClientIpTable)
                .AddInput<TUniproxyPrepared::TVoiceInput>(VoiceInputTable)
                .AddInput<TUniproxyPrepared::TAsrRecognize>(AsrRecognizeTable)
                .AddInput<TUniproxyPrepared::TAsrResult>(AsrResultTable)
                .AddInput<TUniproxyPrepared::TSynchronizeStateWithMessageId>(SynchronizeStateWithMessageIdTable)
                .AddInput<TUniproxyPrepared::TMegamindTimings>(MegamindTimingsTable)
                .AddInput<TUniproxyPrepared::TTtsTimings>(TtsTimingsTable)
                .AddInput<TUniproxyPrepared::TTtsGenerate>(TtsGenerateTable)
                .AddInput<TUniproxyPrepared::TMessageIdToDoNotUseUserLogs>(MessageIdToDoNotUseUserLogsTable)
                .AddInput<TUniproxyPrepared::TAsrDebug>(AsrDebugTable)
                .AddInput<TUniproxyPrepared::TLogSpotterWithStreams>(LogSpotterWithStreamsTable)
                .AddInput<TUniproxyPrepared::TTestIds>(TestIdsTable)
                .AddOutput<TUniproxyPrepared>(
                    NYT::TRichYPath(UniproxyPreparedTable)
                        .Schema(NYT::CreateTableSchema<TUniproxyPrepared>({"uuid", "message_id"})))
                .AddOutput<TUniproxyPrepared::TError>(ErrorTable);
        }

        enum EInIndices {
            MegamindRequest = 0,
            MegamindResponse = 1,
            RequestStat = 2,
            ClientRetry = 3,
            SpotterValidation = 4,
            SpotterStream = 5,
            Stream = 6,
            MessageIdToConnectSessionId = 7,
            MessageIdToEnvironment = 8,
            MessageIdToClientIp = 9,
            VoiceInput = 10,
            AsrRecognize = 11,
            AsrResult = 12,
            SynchronizeStateWithMessageId = 13,
            MegamindTimings = 14,
            TtsTimings = 15,
            TtsGenerate = 16,
            MessageIdToDoNotUseUserLogs = 17,
            AsrDebug = 18,
            LogSpotterWithStreams = 19,
            TestIds = 20,
        };

        enum EOutIndices {
            UniproxyPrepared = 0,
            Error = 1,
        };

    private:
        const TString& RequestStatTable;
        const TString& MegamindRequestTable;
        const TString& MegamindResponseTable;
        const TString& ClientRetryTable;
        const TString& SpotterValidationTable;
        const TString& SpotterStreamTable;
        const TString& StreamTable;
        const TString& MessageIdToConnectSessionIdTable;
        const TString& MessageIdToEnvironmentTable;
        const TString& MessageIdToClientIpTable;
        const TString& VoiceInputTable;
        const TString& AsrRecognizeTable;
        const TString& AsrResultTable;
        const TString& SynchronizeStateWithMessageIdTable;
        const TString& MegamindTimingsTable;
        const TString& TtsTimingsTable;
        const TString& TtsGenerateTable;
        const TString& MessageIdToDoNotUseUserLogsTable;
        const TString& AsrDebugTable;
        const TString& LogSpotterWithStreamsTable;
        const TString& TestIdsTable;
        const TString& UniproxyPreparedTable;
        const TString& ErrorTable;
    };

    void Do(TReader* reader, TWriter* writer) override {
        // TODO(ran1s) add hypothesis_number
        TUniproxyPrepared uniproxyPrepared;

        const auto logErrors = [writer](const TUniproxyPreparedBuilder::TErrors& errors) {
            for (const auto& error : errors) {
                writer->AddRow(error, TInputOutputTables::EOutIndices::Error);
            }
        };

        TUniproxyPreparedBuilder builder;

        for (auto& cursor : *reader) {
            switch (cursor.GetTableIndex()) {
                case TInputOutputTables::EInIndices::MegamindRequest: {
                    logErrors(builder.AddMegamindRequest(cursor.GetRow<TUniproxyPrepared::TMegamindRequest>()));
                    break;
                }
                case TInputOutputTables::EInIndices::MegamindResponse: {
                    logErrors(builder.AddMegamindResponse(cursor.GetRow<TUniproxyPrepared::TMegamindResponse>()));
                    break;
                }
                case TInputOutputTables::EInIndices::RequestStat: {
                    logErrors(builder.AddRequestStat(cursor.GetRow<TUniproxyPrepared::TRequestStatWrapper>()));
                    break;
                }
                case TInputOutputTables::EInIndices::ClientRetry: {
                    logErrors(builder.AddClientRetry(cursor.GetRow<TUniproxyPrepared::TClientRetry>()));
                    break;
                }
                case TInputOutputTables::EInIndices::SpotterValidation: {
                    logErrors(builder.AddSpotterValidation(cursor.GetRow<TUniproxyPrepared::TSpotterValidation>()));
                    break;
                }
                case TInputOutputTables::EInIndices::SpotterStream: {
                    logErrors(builder.AddSpotterStream(cursor.GetRow<TUniproxyPrepared::TSpotterStream>()));
                    break;
                }
                case TInputOutputTables::EInIndices::Stream: {
                    logErrors(builder.AddStream(cursor.GetRow<TUniproxyPrepared::TStream>()));
                    break;
                }
                case TInputOutputTables::EInIndices::LogSpotterWithStreams: {
                    logErrors(
                        builder.AddLogSpotterWithStreams(cursor.GetRow<TUniproxyPrepared::TLogSpotterWithStreams>()));
                    break;
                }
                case TInputOutputTables::EInIndices::MessageIdToConnectSessionId: {
                    logErrors(builder.AddMessageIdToConnectSessionId(
                        cursor.GetRow<TUniproxyPrepared::TMessageIdToConnectSessionId>()));
                    break;
                }
                case TInputOutputTables::EInIndices::MessageIdToEnvironment: {
                    logErrors(builder.AddMessageIdToEnvironment(
                        cursor.GetRow<TUniproxyPrepared::TMessageIdToEnvironment>()));
                    break;
                }
                case TInputOutputTables::EInIndices::MessageIdToClientIp: {
                    logErrors(
                        builder.AddMessageIdToClientIp(cursor.GetRow<TUniproxyPrepared::TMessageIdToClientIp>()));
                    break;
                }
                case TInputOutputTables::EInIndices::VoiceInput: {
                    logErrors(builder.AddVoiceInput(cursor.GetRow<TUniproxyPrepared::TVoiceInput>()));
                    break;
                }
                case TInputOutputTables::EInIndices::AsrRecognize: {
                    logErrors(builder.AddAsrRecognize(cursor.GetRow<TUniproxyPrepared::TAsrRecognize>()));
                    break;
                }
                case TInputOutputTables::EInIndices::AsrResult: {
                    logErrors(builder.AddAsrResult(cursor.GetRow<TUniproxyPrepared::TAsrResult>()));
                    break;
                }
                case TInputOutputTables::EInIndices::SynchronizeStateWithMessageId: {
                    logErrors(builder.AddSynchronizeStateWithMessageId(
                        cursor.GetRow<TUniproxyPrepared::TSynchronizeStateWithMessageId>()));
                    break;
                }
                case TInputOutputTables::EInIndices::MegamindTimings: {
                    logErrors(builder.AddMegamindTimings(cursor.GetRow<TUniproxyPrepared::TMegamindTimings>()));
                    break;
                }
                case TInputOutputTables::EInIndices::TtsTimings: {
                    logErrors(builder.AddTtsTimings(cursor.GetRow<TUniproxyPrepared::TTtsTimings>()));
                    break;
                }
                case TInputOutputTables::EInIndices::TtsGenerate: {
                    logErrors(builder.AddTtsGenerate(cursor.GetRow<TUniproxyPrepared::TTtsGenerate>()));
                    break;
                }
                case TInputOutputTables::EInIndices::MessageIdToDoNotUseUserLogs: {
                    logErrors(builder.AddMessageIdToDoNotUseUserLogs(
                        cursor.GetRow<TUniproxyPrepared::TMessageIdToDoNotUseUserLogs>()));
                    break;
                }
                case TInputOutputTables::EInIndices::AsrDebug: {
                    logErrors(builder.AddAsrDebug(cursor.GetRow<TUniproxyPrepared::TAsrDebug>()));
                    break;
                }
                case TInputOutputTables::EInIndices::TestIds: {
                    logErrors(builder.AddTestIds(cursor.GetRow<TUniproxyPrepared::TTestIds>()));
                    break;
                }
                default:
                    Y_FAIL("Unexpected table index in TUniproxyPreparedReducer");
            }
        }

        writer->AddRow(std::move(builder).Build(), TInputOutputTables::EOutIndices::UniproxyPrepared);
    }
};

/*
 * Создаёт эвристику client_retry_number, означающую ретрай клиента.
 * Не работает для синтетических запросов, потому что используется один request_id.
 *
 */
class TClientRetryReducer
    : public NYT::IReducer<NYT::TTableReader<TUniproxyPrepared::TMegamindRequestResponseRequestStat>,
                           NYT::TTableWriter<TUniproxyPrepared::TClientRetry>> {
public:
    // TODO TInputOutputTables
    void Do(TReader* reader, TWriter* writer) override {
        TVector<TUniproxyPrepared::TMegamindRequestResponseRequestStat> megamindRequestResponseRequestStats;
        for (auto& cursor : *reader) {
            megamindRequestResponseRequestStats.push_back(cursor.GetRow());
        }
        Sort(megamindRequestResponseRequestStats,
             [](const TUniproxyPrepared::TMegamindRequestResponseRequestStat& lhs,
                const TUniproxyPrepared::TMegamindRequestResponseRequestStat& rhs) {
                 return lhs.GetTimestampLogMs() < rhs.GetTimestampLogMs();
             });
        TVector<std::pair<TUniproxyPrepared::TClientRetry, bool>> clientRetries;
        bool oneHasRequestStat = false;
        for (const auto& megamindResponseRequestStat : megamindRequestResponseRequestStats) {
            TUniproxyPrepared::TClientRetry clientRetry;
            clientRetry.SetMessageId(megamindResponseRequestStat.GetMessageId());
            clientRetry.SetUuid(megamindResponseRequestStat.GetUuid());
            if (megamindResponseRequestStat.HasRequestStat()) {
                oneHasRequestStat = true;
            }
            clientRetry.SetTimestampLogMs(megamindResponseRequestStat.GetTimestampLogMs());
            clientRetry.SetSuccessful(false);
            clientRetries.push_back({clientRetry, megamindResponseRequestStat.HasRequestStat()});
        }

        if (oneHasRequestStat) {
            for (auto& [clientRetry, hasSpotter] : TIteratorRange(clientRetries.rbegin(), clientRetries.rend())) {
                if (hasSpotter) {
                    clientRetry.SetSuccessful(true);
                    break;
                }
            }
        } else if (!clientRetries.empty()) {
            clientRetries.back().first.SetSuccessful(true);
        }

        for (const auto& [clientRetry, hasSpotter] : clientRetries) {
            writer->AddRow(clientRetry);
        }
    }
};

/*
 * Редьюсит по message_id для эвристики client_retry
 *
 */
class TMegamindRequestResponseRequestStatReducer
    : public NYT::IReducer<NYT::TTableReader<Message>, NYT::TTableWriter<Message>> {
public:
    class TInputOutputTables {
    public:
        TInputOutputTables(const TString& requestStatTable, const TString& megamindRequestTable,
                           const TString& megamindResponseTable, const TString& megamindRequestRequestStatTable,
                           const TString& errorTable)
            : RequestStatTable(requestStatTable)
            , MegamindRequestTable(megamindRequestTable)
            , MegamindResponseTable(megamindResponseTable)
            , MegamindRequestResponseRequestStatTable(megamindRequestRequestStatTable)
            , ErrorTable(errorTable) {
        }

        NYT::TReduceOperationSpec AddToOperationSpec(NYT::TReduceOperationSpec&& operationSpec) {
            return operationSpec.AddInput<TUniproxyPrepared::TRequestStatWrapper>(RequestStatTable)
                .AddInput<TUniproxyPrepared::TMegamindRequest>(MegamindRequestTable)
                .AddInput<TUniproxyPrepared::TMegamindResponse>(MegamindResponseTable)
                .AddOutput<TUniproxyPrepared::TMegamindRequestResponseRequestStat>(
                    MegamindRequestResponseRequestStatTable)
                .AddOutput<TUniproxyPrepared::TError>(ErrorTable);
        }

        enum EInIndices {
            RequestStat = 0,
            MegamindRequest = 1,
            MegamindResponse = 2,
        };

        enum EOutIndices {
            MegamindRequestResponseRequestStat = 0,
            Error = 1,
        };

    private:
        const TString& RequestStatTable;
        const TString& MegamindRequestTable;
        const TString& MegamindResponseTable;
        const TString& MegamindRequestResponseRequestStatTable;
        const TString& ErrorTable;
    };

    void Do(TReader* reader, TWriter* writer) override {
        TUniproxyPrepared::TMegamindRequestResponseRequestStat megamindRequestResponseRequestStat;

        const auto logError = [&megamindRequestResponseRequestStat,
                               writer](const TUniproxyPrepared::TError::EReason reason, const TString& message) {
            TUniproxyPrepared::TError error;
            error.SetProcess(TUniproxyPrepared::TError::P_MEGAMIND_REQUEST_RESPONSE_REQUEST_STAT_REDUCER);
            error.SetReason(reason);
            error.SetMessage(message);
            if (megamindRequestResponseRequestStat.HasUuid()) {
                error.SetUuid(megamindRequestResponseRequestStat.GetUuid());
            }
            if (megamindRequestResponseRequestStat.HasMessageId()) {
                error.SetMessageId(megamindRequestResponseRequestStat.GetMessageId());
            }
            if (auto setraceUrl =
                    TryGenerateSetraceUrl({error.HasMessageId() ? error.GetMessageId() : TMaybe<TString>{},
                                           error.HasUuid() ? error.GetUuid() : TMaybe<TString>{}})) {
                error.SetSetraceUrl(*setraceUrl);
            }
            writer->AddRow(error, TInputOutputTables::EOutIndices::Error);
        };

        const auto setUuid = [&megamindRequestResponseRequestStat, logError](const TString& uuid) {
            if (!megamindRequestResponseRequestStat.HasUuid()) {
                megamindRequestResponseRequestStat.SetUuid(uuid);
            } else if (megamindRequestResponseRequestStat.GetUuid() != uuid) {
                logError(TUniproxyPrepared::TError::R_DIFFERENT_VALUES,
                         TStringBuilder{} << "Got different uuid" << megamindRequestResponseRequestStat.GetUuid()
                                          << " " << uuid);
            }
        };

        auto setMessageId = [&megamindRequestResponseRequestStat, logError](const TString& messageId) {
            if (!megamindRequestResponseRequestStat.HasMessageId()) {
                megamindRequestResponseRequestStat.SetMessageId(messageId);
            } else if (megamindRequestResponseRequestStat.GetMessageId() != messageId) {
                logError(TUniproxyPrepared::TError::R_DIFFERENT_VALUES,
                         TStringBuilder{} << "Got different message_id "
                                          << megamindRequestResponseRequestStat.GetMessageId() << " " << messageId);
            }
        };

        auto setMegamindRequestId = [&megamindRequestResponseRequestStat, logError](const TString& megamindRequestId) {
            if (!megamindRequestResponseRequestStat.HasMegamindRequestId()) {
                megamindRequestResponseRequestStat.SetMegamindRequestId(megamindRequestId);
            } else if (megamindRequestResponseRequestStat.GetMegamindRequestId() != megamindRequestId) {
                // https://st.yandex-team.ru/VOICESERV-3341
                logError(TUniproxyPrepared::TError::R_DIFFERENT_VALUES,
                         TStringBuilder{} << "Got different megamind_request_id "
                                          << megamindRequestResponseRequestStat.GetMegamindRequestId() << " "
                                          << megamindRequestId);
            }
        };

        const auto initializeCommonFields = [setMessageId, setUuid](const auto& message) {
            setMessageId(message.GetMessageId());
            setUuid(message.GetUuid());
        };

        bool hasMegamindRequestId = false;
        TMaybe<ui64> TimestampLogMs;
        for (auto& cursor : *reader) {
            switch (cursor.GetTableIndex()) {
                case TInputOutputTables::EInIndices::RequestStat: {
                    const auto& requestStat = cursor.GetRow<TUniproxyPrepared::TRequestStatWrapper>();
                    initializeCommonFields(requestStat);
                    *megamindRequestResponseRequestStat.MutableRequestStat() = requestStat.GetRequestStat();
                    break;
                }
                case TInputOutputTables::EInIndices::MegamindRequest: {
                    const auto& megamindRequest = cursor.GetRow<TUniproxyPrepared::TMegamindRequest>();
                    initializeCommonFields(megamindRequest);
                    setMegamindRequestId(megamindRequest.GetMegamindRequestId());
                    if (TimestampLogMs < megamindRequest.GetTimestampLogMs()) {
                        TimestampLogMs = megamindRequest.GetTimestampLogMs();
                    }
                    hasMegamindRequestId = true;
                    break;
                }
                case TInputOutputTables::EInIndices::MegamindResponse: {
                    const auto& megamindResponse = cursor.GetRow<TUniproxyPrepared::TMegamindResponse>();
                    initializeCommonFields(megamindResponse);
                    setMegamindRequestId(megamindResponse.GetMegamindRequestId());
                    if (TimestampLogMs) {
                        TimestampLogMs = Max(*TimestampLogMs, megamindResponse.GetTimestampLogMs());
                    } else {
                        TimestampLogMs = megamindResponse.GetTimestampLogMs();
                    }
                    hasMegamindRequestId = true;
                    break;
                }
                default:
                    Y_FAIL("Unexpected table index in TMegamindRequestResponseRequestStatReducer");
            }
        }
        if (TimestampLogMs) {
            megamindRequestResponseRequestStat.SetTimestampLogMs(*TimestampLogMs);
        }
        if (hasMegamindRequestId) {
            writer->AddRow(megamindRequestResponseRequestStat,
                           TInputOutputTables::EOutIndices::MegamindRequestResponseRequestStat);
        }
    }
};

/*
 * Для SynchronizeState понимание к каким message_id он может относиться
 *
 */
class TSynchronizeStateWithMessageIdReducer
    : public NYT::IReducer<NYT::TTableReader<Message>, NYT::TTableWriter<Message>> {
public:
    class TInputOutputTables {
    public:
        TInputOutputTables(const TString& synchronizeStateTable, const TString& messageIdToConnectSessionIdTable,
                           const TString& synchronizeStateWithMessageIdTable, const TString& errorTable)
            : SynchronizeStateTable(synchronizeStateTable)
            , MessageIdToConnectSessionIdTable(messageIdToConnectSessionIdTable)
            , SynchronizeStateWithMessageIdTable(synchronizeStateWithMessageIdTable)
            , ErrorTable(errorTable) {
        }
        NYT::TReduceOperationSpec AddToOperationSpec(NYT::TReduceOperationSpec&& operationSpec) {
            return operationSpec.AddInput<TUniproxyPrepared::TSynchronizeState>(SynchronizeStateTable)
                .AddInput<TUniproxyPrepared::TMessageIdToConnectSessionId>(MessageIdToConnectSessionIdTable)
                .AddOutput<TUniproxyPrepared::TSynchronizeStateWithMessageId>(SynchronizeStateWithMessageIdTable)
                .AddOutput<TUniproxyPrepared::TError>(ErrorTable);
        }

        enum EInIndices {
            SynchronizeState = 0,
            MessageIdToConnectSessionId = 1,
        };

        enum EOutIndices {
            SynchronizeStateWithMessageId = 0,
            Error = 1,
        };

    private:
        const TString& SynchronizeStateTable;
        const TString& MessageIdToConnectSessionIdTable;
        const TString& SynchronizeStateWithMessageIdTable;
        const TString& ErrorTable;
    };
    void Do(TReader* reader, TWriter* writer) override {
        TUniproxyPrepared::TSynchronizeStateWithMessageId synchronizeStateWithMessageId;
        const auto logError = [&synchronizeStateWithMessageId, writer](const TUniproxyPrepared::TError::EReason reason,
                                                                       const TString& message) {
            TUniproxyPrepared::TError error;
            error.SetProcess(TUniproxyPrepared::TError::P_SYNCHRONIZE_STATE_WITH_MESSAGE_ID_REDUCER);
            error.SetReason(reason);
            error.SetMessage(message);
            if (synchronizeStateWithMessageId.HasUuid()) {
                error.SetUuid(synchronizeStateWithMessageId.GetUuid());
            }
            if (synchronizeStateWithMessageId.HasMessageId()) {
                error.SetMessageId(synchronizeStateWithMessageId.GetMessageId());
            }
            if (auto setraceUrl =
                    TryGenerateSetraceUrl({error.HasMessageId() ? error.GetMessageId() : TMaybe<TString>{},
                                           error.HasUuid() ? error.GetUuid() : TMaybe<TString>{}})) {
                error.SetSetraceUrl(*setraceUrl);
            }
            writer->AddRow(error, TInputOutputTables::EOutIndices::Error);
        };
        auto setUuid = [&synchronizeStateWithMessageId, logError](const TString& uuid) {
            if (!synchronizeStateWithMessageId.HasUuid()) {
                synchronizeStateWithMessageId.SetUuid(uuid);
            } else if (synchronizeStateWithMessageId.GetUuid() != uuid) {
                logError(TUniproxyPrepared::TError::R_DIFFERENT_VALUES,
                         TStringBuilder{} << "Got different uuid" << synchronizeStateWithMessageId.GetUuid() << " "
                                          << uuid);
            }
        };
        for (auto& cursor : *reader) {
            switch (cursor.GetTableIndex()) {
                case TInputOutputTables::EInIndices::SynchronizeState: {
                    const auto& synchronizeState = cursor.GetRow<TUniproxyPrepared::TSynchronizeState>();
                    setUuid(synchronizeState.GetUuid());
                    if (!synchronizeStateWithMessageId.HasValue() ||
                        synchronizeStateWithMessageId.GetTimestampLogMs() < synchronizeState.GetTimestampLogMs()) {
                        synchronizeStateWithMessageId.SetTimestampLogMs(synchronizeState.GetTimestampLogMs());
                        *synchronizeStateWithMessageId.MutableValue() = synchronizeState.GetValue();
                    }
                    break;
                }
                case TInputOutputTables::EInIndices::MessageIdToConnectSessionId: {
                    const auto& messageIdToConnectSessionId =
                        cursor.GetRow<TUniproxyPrepared::TMessageIdToConnectSessionId>();
                    setUuid(messageIdToConnectSessionId.GetUuid());
                    synchronizeStateWithMessageId.SetMessageId(messageIdToConnectSessionId.GetMessageId());
                    if (synchronizeStateWithMessageId.HasValue()) {
                        writer->AddRow(synchronizeStateWithMessageId,
                                       TInputOutputTables::EOutIndices::SynchronizeStateWithMessageId);
                    }
                    break;
                }
                default:
                    Y_FAIL("Unexpected table index in TSynchronizeStateWithMessageIdReducer");
            }
        }
    }
};

/*
 * Объединение Stream event где isSpotter = false и Log Spotter event по uuid и message_id
 *
 */
class TLogSpotterWithStreamsReducer : public NYT::IReducer<NYT::TTableReader<Message>, NYT::TTableWriter<Message>> {
public:
    class TInputOutputTables {
    public:
        TInputOutputTables(const TString& logSpotterTable, const TString& streamTable,
                           const TString& logSpotterWithStreamTable, const TString& errorTable)
            : LogSpotterTable(logSpotterTable)
            , StreamTable(streamTable)
            , LogSpotterWithStreamTable(logSpotterWithStreamTable)
            , ErrorTable(errorTable) {
        }
        NYT::TReduceOperationSpec AddToOperationSpec(NYT::TReduceOperationSpec&& operationSpec) {
            return operationSpec.AddInput<TUniproxyPrepared::TLogSpotter>(LogSpotterTable)
                .AddInput<TUniproxyPrepared::TStream>(StreamTable)
                .AddOutput<TUniproxyPrepared::TLogSpotterWithStreams>(LogSpotterWithStreamTable)
                .AddOutput<TUniproxyPrepared::TError>(ErrorTable);
        }

        enum EInIndices {
            LogSpotter = 0,
            Stream = 1,
        };

        enum EOutIndices {
            LogSpotterWithStream = 0,
            Error = 1,
        };

    private:
        const TString& LogSpotterTable;
        const TString& StreamTable;
        const TString& LogSpotterWithStreamTable;
        const TString& ErrorTable;
    };
    void Do(TReader* reader, TWriter* writer) override {
        TUniproxyPrepared::TLogSpotterWithStreams logSpotterWithStreams;
        const auto logError = [&logSpotterWithStreams, writer](const TUniproxyPrepared::TError::EReason reason,
                                                               const TString& message) {
            TUniproxyPrepared::TError error;
            error.SetProcess(TUniproxyPrepared::TError::P_LOG_SPOTTER_WITH_STREAMS_REDUCER);
            error.SetReason(reason);
            error.SetMessage(message);
            if (logSpotterWithStreams.HasUuid()) {
                error.SetUuid(logSpotterWithStreams.GetUuid());
            }
            if (logSpotterWithStreams.HasMessageId()) {
                error.SetMessageId(logSpotterWithStreams.GetMessageId());
            }
            if (auto setraceUrl =
                    TryGenerateSetraceUrl({error.HasMessageId() ? error.GetMessageId() : TMaybe<TString>{},
                                           error.HasUuid() ? error.GetUuid() : TMaybe<TString>{}})) {
                error.SetSetraceUrl(*setraceUrl);
            }
            writer->AddRow(error, TInputOutputTables::EOutIndices::Error);
        };
        const auto setUuid = [&logSpotterWithStreams, logError](const TString& uuid) {
            if (!logSpotterWithStreams.HasUuid()) {
                logSpotterWithStreams.SetUuid(uuid);
            } else if (logSpotterWithStreams.GetUuid() != uuid) {
                logError(TUniproxyPrepared::TError::R_DIFFERENT_VALUES,
                         TStringBuilder{} << "Got different uuid " << logSpotterWithStreams.GetUuid() << " " << uuid);
            }
        };

        const auto setMessageId = [&logSpotterWithStreams, logError](const TString& messageId) {
            if (!logSpotterWithStreams.HasMessageId()) {
                logSpotterWithStreams.SetMessageId(messageId);
            } else if (logSpotterWithStreams.GetMessageId() != messageId) {
                logError(TUniproxyPrepared::TError::R_DIFFERENT_VALUES,
                         TStringBuilder{} << "Got different message_id " << logSpotterWithStreams.GetMessageId() << " "
                                          << messageId);
            }
        };
        bool containsLogSpotter = false;
        for (auto& cursor : *reader) {
            switch (cursor.GetTableIndex()) {
                case TInputOutputTables::EInIndices::LogSpotter: {
                    const auto& logSpotter = cursor.GetRow<TUniproxyPrepared::TLogSpotter>();
                    setUuid(logSpotter.GetUuid());
                    setMessageId(logSpotter.GetMessageId());
                    if (logSpotter.HasTimestampLogMs()) {
                        logSpotterWithStreams.SetTimestampLogMs(logSpotter.GetTimestampLogMs());
                    }
                    *logSpotterWithStreams.MutableLogSpotter() = logSpotter.GetValue();
                    containsLogSpotter = true;
                    break;
                }
                case TInputOutputTables::EInIndices::Stream: {
                    const auto& stream = cursor.GetRow<TUniproxyPrepared::TStream>();
                    setUuid(stream.GetUuid());
                    if (stream.HasTimestampLogMs()) {
                        logSpotterWithStreams.SetTimestampLogMs(stream.GetTimestampLogMs());
                    }
                    *logSpotterWithStreams.MutableStreams()->Add() = stream.GetValue();
                    break;
                }
                default:
                    Y_FAIL("Unexpected table index in TLogSpotterWithStreamReducer");
            }
        }
        if (containsLogSpotter) {
            writer->AddRow(logSpotterWithStreams, TInputOutputTables::EOutIndices::LogSpotterWithStream);
        }
    }
};

REGISTER_MAPPER(TUniproxyEventDirectiveMapper)
REGISTER_REDUCER(TUniproxyPreparedReducer)
REGISTER_REDUCER(TClientRetryReducer)
REGISTER_REDUCER(TMegamindRequestResponseRequestStatReducer)
REGISTER_REDUCER(TSynchronizeStateWithMessageIdReducer)
REGISTER_REDUCER(TLogSpotterWithStreamsReducer)

class TTempTable {
public:
    TTempTable(NYT::IClientBasePtr client, const TString& tmpDirectory, const TString& prefix)
        : Table_(CreateRandomTable(client, tmpDirectory, prefix))
        , SortedTable_(CreateRandomTable(client, tmpDirectory, prefix + "-sorted")) {
    }

    const TString& Table() const {
        return Table_;
    }
    const TString& Sorted() const {
        return SortedTable_;
    }

private:
    TString Table_;
    TString SortedTable_;
};

} // namespace

namespace NAlice::NWonderlogs {

void MakeUniproxyPrepared(NYT::IClientPtr client, const TString& tmpDirectory, const TVector<TString>& uniproxyEvents,
                          const TString& outputTable, const TString& errorTable, const TInstant& timestampFrom,
                          const TInstant& timestampTo, const TDuration& requestsShift) {
    auto tx = client->StartTransaction();

    const TTempTable requestStat{tx, tmpDirectory, "request-stat"};
    const TTempTable megamindRequest{tx, tmpDirectory, "megamind-request"};
    const TTempTable megamindRequestResponseRequestStat{tx, tmpDirectory, "megamind-response-request-stat"};
    const TTempTable megamindResponse{tx, tmpDirectory, "megamind-response"};
    const TTempTable clientRetry{tx, tmpDirectory, "client-retry"};
    const TTempTable spotterValidation{tx, tmpDirectory, "spotter-validation"};
    const TTempTable spotterStream{tx, tmpDirectory, "spotter-stream"};
    const TTempTable stream{tx, tmpDirectory, "stream"};
    const TTempTable logSpotter{tx, tmpDirectory, "log-spotter"};
    const TTempTable logSpotterWithStreams{tx, tmpDirectory, "log-spotter-with-streams"};
    const TTempTable synchronizeState{tx, tmpDirectory, "synchronize-state"};
    const TTempTable synchronizeStateWithMessageId{tx, tmpDirectory, "synchronize-state-with-message-id"};
    const TTempTable voiceInput{tx, tmpDirectory, "voice-input"};
    const TTempTable asrRecognize{tx, tmpDirectory, "asr-recognize"};
    const TTempTable asrResult{tx, tmpDirectory, "asr-result"};
    const TTempTable messageIdToConnectSessionId{tx, tmpDirectory, "message-id-to-connect-session-id"};
    const auto messageIdToConnectSessionIdSortedByConnectSessionId =
        CreateRandomTable(tx, tmpDirectory, "message-id-to-connect-session-id-sorted-by-connect-session-id");
    const TTempTable messageIdToEnvironment{tx, tmpDirectory, "message-id-to-environment"};
    const TTempTable messageIdToClientIp{tx, tmpDirectory, "message-id-to-client-ip"};
    const TTempTable megamindTimings{tx, tmpDirectory, "megamind-timings"};
    const TTempTable ttsTimings{tx, tmpDirectory, "tts-timings"};
    const TTempTable ttsGenerate{tx, tmpDirectory, "tts-generate"};
    const TTempTable doNotUseUserLogs{tx, tmpDirectory, "message-id-to-do-not-use-user-logs"};
    const TTempTable asrDebug{tx, tmpDirectory, "asr-debug"};
    const TTempTable testIds{tx, tmpDirectory, "test-ids"};

    CreateTable(tx, outputTable, TInstant::Now() + MONTH_TTL);
    CreateTable(tx, errorTable, TInstant::Now() + MONTH_TTL);

    TVector<TString> errorTables;
    for (int i = 0; i < 6; i++) {
        errorTables.push_back(CreateRandomTable(tx, tmpDirectory, "error-table"));
    }
    NYT::TNode spec;
    spec["job_io"]["table_writer"]["max_row_weight"] = 32_MB;

    tx->Map(TUniproxyEventDirectiveMapper::TInputOutputTables(
                uniproxyEvents, requestStat.Table(), megamindRequest.Table(), megamindResponse.Table(),
                spotterValidation.Table(), spotterStream.Table(), stream.Table(), logSpotter.Table(),
                messageIdToConnectSessionId.Table(), messageIdToEnvironment.Table(), messageIdToClientIp.Table(),
                synchronizeState.Table(), voiceInput.Table(), asrRecognize.Table(), asrResult.Table(),
                megamindTimings.Table(), ttsTimings.Table(), ttsGenerate.Table(), doNotUseUserLogs.Table(),
                asrDebug.Table(), testIds.Table(), errorTables[0])
                .AddToOperationSpec(NYT::TMapOperationSpec{})
                .MapperSpec(NYT::TUserJobSpec{}.MemoryLimit(1_GB).MemoryReserveFactor(15)),
            new TUniproxyEventDirectiveMapper{timestampFrom, timestampTo, requestsShift},
            NYT::TOperationOptions{}.InferOutputSchema(true).Spec(spec));

    auto threads = CreateThreadPool(/* threadCount= */ 2);
    TVector<NThreading::TFuture<void>> futures;

    // Independent pipeline generating synchronize states with message id
    futures.push_back(NThreading::Async(
        [&tx, &messageIdToConnectSessionId, &messageIdToConnectSessionIdSortedByConnectSessionId, &synchronizeState,
         &synchronizeStateWithMessageId, &errorTables, &spec]() {
            {
                NYT::TOperationTracker tracker;
                tracker.AddOperation(tx->Sort(NYT::TSortOperationSpec{}
                                                  .AddInput(messageIdToConnectSessionId.Table())
                                                  .Output(messageIdToConnectSessionIdSortedByConnectSessionId)
                                                  .SortBy({"uuid", "connect_session_id"}),
                                              NYT::TOperationOptions{}.Wait(false)));
                tracker.AddOperation(tx->Sort(NYT::TSortOperationSpec{}
                                                  .AddInput(synchronizeState.Table())
                                                  .Output(synchronizeState.Sorted())
                                                  .SortBy({"uuid", "connect_session_id"}),
                                              NYT::TOperationOptions{}.Wait(false)));
                tracker.WaitAllCompleted();
            }
            tx->Reduce(TSynchronizeStateWithMessageIdReducer::TInputOutputTables(
                           synchronizeState.Sorted(), messageIdToConnectSessionIdSortedByConnectSessionId,
                           synchronizeStateWithMessageId.Table(), errorTables[2])
                           .AddToOperationSpec(NYT::TReduceOperationSpec{})
                           .ReducerSpec(NYT::TUserJobSpec{}.MemoryLimit(1_GB))
                           .ReduceBy({"uuid", "connect_session_id"}),
                       new TSynchronizeStateWithMessageIdReducer, NYT::TOperationOptions{}.InferOutputSchema(true));
            tx->Sort(NYT::TSortOperationSpec{}
                         .AddInput(synchronizeStateWithMessageId.Table())
                         .Output(synchronizeStateWithMessageId.Sorted())
                         .SortBy({"uuid", "message_id", "timestamp_log_ms"}),
                     NYT::TOperationOptions{}.Spec(spec));
        },
        *threads));

    // Independent pipeline preparing logSpotterStream events with micro
    futures.push_back(NThreading::Async(
        [&tx, &logSpotter, &stream, &logSpotterWithStreams, &errorTables, &spec]() {
            {
                NYT::TOperationTracker tracker;
                for (const auto& tmpTable : {logSpotter, stream}) {
                    tracker.AddOperation(tx->Sort(NYT::TSortOperationSpec{}
                                                      .AddInput(tmpTable.Table())
                                                      .Output(tmpTable.Sorted())
                                                      .SortBy({"uuid", "spotter_stream_id", "timestamp_log_ms"}),
                                                  NYT::TOperationOptions{}.Wait(false).Spec(spec)));
                }
                tracker.WaitAllCompleted();
            }

            tx->Reduce(TLogSpotterWithStreamsReducer::TInputOutputTables(logSpotter.Sorted(), stream.Sorted(),
                                                                         logSpotterWithStreams.Table(), errorTables[4])
                           .AddToOperationSpec(NYT::TReduceOperationSpec{})
                           .ReducerSpec(NYT::TUserJobSpec{}.MemoryLimit(1_GB))
                           .ReduceBy({"uuid", "spotter_stream_id"}),
                       new TLogSpotterWithStreamsReducer, NYT::TOperationOptions{}.InferOutputSchema(true).Spec(spec));

            {
                NYT::TOperationTracker tracker;
                for (const auto& tmpTable : {logSpotterWithStreams, stream}) {
                    tracker.AddOperation(tx->Sort(NYT::TSortOperationSpec{}
                                                      .AddInput(tmpTable.Table())
                                                      .Output(tmpTable.Sorted())
                                                      .SortBy({"uuid", "message_id", "timestamp_log_ms"}),
                                                  NYT::TOperationOptions{}.Wait(false).Spec(spec)));
                }
                tracker.WaitAllCompleted();
            }
        },
        *threads));

    {
        NYT::TOperationTracker tracker;
        for (auto& tmpTable : {spotterValidation, spotterStream, messageIdToConnectSessionId, messageIdToEnvironment,
                               messageIdToClientIp, voiceInput, asrRecognize, asrResult, megamindTimings, ttsTimings,
                               ttsGenerate, doNotUseUserLogs, asrDebug, testIds}) {
            tracker.AddOperation(tx->Sort(NYT::TSortOperationSpec{}
                                              .AddInput(tmpTable.Table())
                                              .Output(tmpTable.Sorted())
                                              .SortBy({"uuid", "message_id", "timestamp_log_ms"}),
                                          NYT::TOperationOptions{}.Wait(false).Spec(spec)));
        }
        {
            NYT::TOperationTracker tracker2;
            for (const auto& tmpTable : {megamindRequest, megamindResponse, requestStat}) {
                tracker2.AddOperation(tx->Sort(NYT::TSortOperationSpec{}
                                                   .AddInput(tmpTable.Table())
                                                   .Output(tmpTable.Sorted())
                                                   .SortBy({"uuid", "message_id", "timestamp_log_ms"}),
                                               NYT::TOperationOptions{}.Wait(false).Spec(spec)));
            }
            tracker2.WaitAllCompleted();
        }

        tx->Reduce(TMegamindRequestResponseRequestStatReducer::TInputOutputTables(
                       requestStat.Sorted(), megamindRequest.Sorted(), megamindResponse.Sorted(),
                       megamindRequestResponseRequestStat.Table(), errorTables[1])
                       .AddToOperationSpec(NYT::TReduceOperationSpec{})
                       .ReducerSpec(NYT::TUserJobSpec{}.MemoryLimit(1_GB).MemoryReserveFactor(17))
                       .ReduceBy({"uuid", "message_id"}),
                   new TMegamindRequestResponseRequestStatReducer,
                   NYT::TOperationOptions{}.InferOutputSchema(true).Spec(spec));

        tx->Sort(NYT::TSortOperationSpec()
                     .AddInput(megamindRequestResponseRequestStat.Table())
                     .Output(megamindRequestResponseRequestStat.Sorted())
                     .SortBy({"uuid", "megamind_request_id"}));

        tx->Reduce(NYT::TReduceOperationSpec{}
                       .ReducerSpec(NYT::TUserJobSpec{}.MemoryLimit(1_GB).MemoryReserveFactor(19))
                       .ReduceBy({"uuid", "megamind_request_id"})
                       .AddInput<TUniproxyPrepared::TMegamindRequestResponseRequestStat>(
                           megamindRequestResponseRequestStat.Sorted())
                       .AddOutput<TUniproxyPrepared::TClientRetry>(clientRetry.Table()),
                   new TClientRetryReducer, NYT::TOperationOptions{}.InferOutputSchema(true).Spec(spec));

        tracker.AddOperation(tx->Sort(NYT::TSortOperationSpec()
                                          .AddInput(clientRetry.Table())
                                          .Output(clientRetry.Sorted())
                                          .SortBy({"uuid", "message_id", "timestamp_log_ms"}),
                                      NYT::TOperationOptions{}.Wait(false)));

        tracker.WaitAllCompleted();
    }
    NThreading::NWait::WaitAllOrException(futures).Wait();

    tx->Reduce(TUniproxyPreparedReducer::TInputOutputTables(
                   requestStat.Sorted(), megamindRequest.Sorted(), megamindResponse.Sorted(), clientRetry.Sorted(),
                   spotterValidation.Sorted(), spotterStream.Sorted(), stream.Sorted(),
                   messageIdToConnectSessionId.Sorted(), messageIdToEnvironment.Sorted(), messageIdToClientIp.Sorted(),
                   voiceInput.Sorted(), asrRecognize.Sorted(), asrResult.Sorted(),
                   synchronizeStateWithMessageId.Sorted(), megamindTimings.Sorted(), ttsTimings.Sorted(),
                   ttsGenerate.Sorted(), doNotUseUserLogs.Sorted(), asrDebug.Sorted(), logSpotterWithStreams.Sorted(),
                   testIds.Sorted(), outputTable, errorTables[3])
                   .AddToOperationSpec(NYT::TReduceOperationSpec{})
                   .ReducerSpec(NYT::TUserJobSpec{}.MemoryLimit(1_GB).MemoryReserveFactor(17))
                   .ReduceBy({"uuid", "message_id"})
                   .SortBy({"uuid", "message_id", "timestamp_log_ms"}),
               new TUniproxyPreparedReducer, NYT::TOperationOptions{}.Spec(spec));

    tx->Concatenate(errorTables, errorTable);
    tx->Commit();
}

} // namespace NAlice::NWonderlogs
