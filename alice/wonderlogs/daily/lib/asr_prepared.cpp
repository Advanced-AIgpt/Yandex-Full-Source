#include "asr_prepared.h"

#include "ttls.h"

#include <alice/wonderlogs/library/common/names.h>
#include <alice/wonderlogs/library/common/utils.h>
#include <alice/wonderlogs/library/yt/utils.h>
#include <alice/wonderlogs/protos/asr_prepared.pb.h>
#include <alice/wonderlogs/protos/uniproxy_prepared.pb.h>

#include <library/cpp/protobuf/yt/yt2proto.h>
#include <library/cpp/yson/node/node_io.h>

#include <mapreduce/yt/library/operation_tracker/operation_tracker.h>

#include <voicetech/asr/logs/lib/session_parsing.h>
#include <voicetech/library/proto_api/analytics_info.pb.h>
#include <voicetech/library/proto_api/yaldi.pb.h>

#include <util/generic/hash_set.h>
#include <util/generic/size_literals.h>
#include <util/string/builder.h>

namespace NAlice::NWonderlogs {

namespace {
using google::protobuf::Message;

/*
 * Создаёт таблицу с последней гипотезой из логов ASR
 *
 */
class TAsrMapper : public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<Message>> {
public:
    class TInputOutputTables {
    public:
        TInputOutputTables(const TVector<TString>& asrLogsTables, const TString& asrTable,
                           const TString& onlineValidation, const TString& errorTable)
            : AsrLogsTables(asrLogsTables)
            , AsrTable(asrTable)
            , OnlineValidationTable(onlineValidation)
            , ErrorTable(errorTable) {
        }

        NYT::TMapOperationSpec AddToOperationSpec(NYT::TMapOperationSpec&& operationSpec) {
            for (const auto& table : AsrLogsTables) {
                operationSpec = operationSpec.AddInput<NYT::TNode>(table);
            }
            return operationSpec.AddOutput<TAsrPrepared::TData>(AsrTable)
                .AddOutput<TAsrPrepared::TOnlineValidation>(OnlineValidationTable)
                .AddOutput<TAsrPrepared::TError>(ErrorTable);
        }

        enum EOutIndices {
            AsrData = 0,
            OnlineValidation = 1,
            Error = 2,
        };

    private:
        const TVector<TString>& AsrLogsTables;
        const TString& AsrTable;
        const TString& OnlineValidationTable;
        const TString& ErrorTable;
    };

    Y_SAVELOAD_JOB(TimestampFrom, TimestampTo, RequestsShift);
    TAsrMapper() = default;
    TAsrMapper(const TInstant& timestampFrom, const TInstant& timestampTo, const TDuration requestsShift)
        : TimestampFrom(timestampFrom)
        , TimestampTo(timestampTo)
        , RequestsShift(requestsShift) {
    }

    void Do(TReader* reader, TWriter* writer) override {
        for (auto& cursor : *reader) {
            const auto& row = cursor.GetRow();

            // TODO move to parsing lib by asr
            if (!row["all_messages_list"].HasValue()) {
                continue;
            }

            if (!row["request_id"].IsString()) {
                Clog << "Row doesn't have request_id" << Endl;
                continue;
            }

            if (!row["audio_mds_key"].IsString() || row["audio_mds_key"].AsString().empty()) {
                Clog << "Row doesn't have audio_mds_key" << Endl;
                continue;
            }

            auto timestamp = ParseDatetime(row["session_start_time"].AsString());
            if (timestamp && SkipRequest(*timestamp, TimestampFrom, TimestampTo, RequestsShift)) {
                continue;
            }
            const auto uuid = [&row]() -> TMaybe<TString> {
                TMaybe<TString> uuid;
                if (row["init_session_info"].IsMap()) {
                    uuid = MaybeStringFromYson(row["init_session_info"]["uuid"]);
                }
                if (uuid) {
                    return NormalizeUuid(*uuid);
                }
                return uuid;
            }();
            const auto messageId = MaybeStringFromYson(row["request_id"]);

            if (!uuid || !messageId) {
                continue;
            }

            {
                // In this case uniproxy sends two requests to asr:
                // 1. One request checks whether the request is a true activation
                // 2. A regular request
                // One filters the activation requests here because both requests have the same message_id

                const auto& spotterValidation = row["init_session_info"]["advanced_options"]["spotter_validation"];
                if (spotterValidation.IsBool() && spotterValidation.AsBool()) {
                    TAsrPrepared::TOnlineValidation onlineValidation;
                    if (timestamp) {
                        onlineValidation.SetTimestampLogMs(timestamp->MilliSeconds());
                    }
                    onlineValidation.SetUuid(*uuid);
                    onlineValidation.SetMessageId(*messageId);
                    onlineValidation.MutableValue()->SetMdsKey(row["audio_mds_key"].AsString());
                    writer->AddRow(onlineValidation, TInputOutputTables::EOutIndices::OnlineValidation);
                    continue;
                }
            }

            TAsrPrepared::TData asr;
            if (timestamp) {
                asr.SetTimestampLogMs(timestamp->MilliSeconds());
            }
            asr.SetUuid(*uuid);
            asr.SetMessageId(*messageId);

            const auto logError = [writer, &uuid, &messageId](const TAsrPrepared::TError::EReason reason,
                                                              const TString& message) {
                TAsrPrepared::TError error;
                error.SetProcess(TAsrPrepared::TError::P_ASR_MAPPER);
                error.SetReason(reason);
                error.SetMessage(message);
                if (uuid) {
                    error.SetUuid(*uuid);
                }
                if (messageId) {
                    error.SetMessageId(*messageId);
                }
                if (auto setraceUrl = TryGenerateSetraceUrl({messageId, uuid})) {
                    error.SetSetraceUrl(*setraceUrl);
                }
                writer->AddRow(error, TInputOutputTables::EOutIndices::Error);
            };

            for (const auto& [field, name, yson] : {std::tie(uuid, UUID, row["init_session_info"]),
                                                    std::tie(messageId, MESSAGE_ID, row["request_id"])}) {
                if (!field || field->empty()) {
                    logError(TAsrPrepared::TError::R_INVALID_VALUE,
                             TStringBuilder{} << "Invalid " << name << ": " << NYT::NodeToYsonString(yson));
                }
            }

            asr.MutableData()->SetMdsKey(row["audio_mds_key"].AsString());
            YaldiProtobuf::AddDataResponse addDataResponse;
            bool found = false;
            for (const auto& message : row["all_messages_list"].AsList()) {
                if (message["endOfUtt"].IsBool() && message["endOfUtt"].AsBool()) {
                    found = true;
                    try {
                        YtNodeToProto(message, addDataResponse);
                    } catch (...) {
                        logError(TAsrPrepared::TError::R_FAILED_CONVERT_YSON_TO_PROTO, NYT::NodeToYsonString(message));
                    }
                    break;
                }
            }
            if (addDataResponse.recognitionSize() > 0) {
                const auto& recognition = addDataResponse.Getrecognition(0);
                asr.MutableData()->MutableRecognition()->SetNormalized(recognition.Getnormalized());
                for (const auto& word : recognition.Getwords()) {
                    asr.MutableData()->MutableRecognition()->AddWords(word.Getvalue());
                }
                for (const auto& hypothesisFrom : addDataResponse.recognition()) {
                    auto& hypothesisTo = *asr.MutableData()->AddHypotheses();
                    for (const auto& word : hypothesisFrom.Getwords()) {
                        hypothesisTo.AddWords(word.Getvalue());
                    }
                    if (hypothesisFrom.Hasnormalized()) {
                        hypothesisTo.SetNormalized(hypothesisFrom.Getnormalized());
                    }
                }
            }

            if (row.IsMap() && row["init_session_info"].IsMap() && row["init_session_info"]["topic"].IsString()) {
                asr.SetTopic(row["init_session_info"]["topic"].AsString());
            }
            if (addDataResponse.Hasis_trash()) {
                asr.MutableData()->SetTrash(addDataResponse.Getis_trash());
            }
            if (addDataResponse.Getwhisper_info().Hasis_whisper()) {
                asr.MutableData()->SetWhisper(addDataResponse.Getwhisper_info().Getis_whisper());
            }
            {
                const NVoicetech::NASRLogs::NParsing::TSessionParser parser(row);
                const auto& analyticsInfo = parser.GetASRAnalyticsInfo();
                if (analyticsInfo) {
                    *asr.MutableAnalyticsInfo() = *analyticsInfo;
                }
            }
            if (found) {
                writer->AddRow(asr, TInputOutputTables::EOutIndices::AsrData);
            }
        }
    }

private:
    TInstant TimestampFrom;
    TInstant TimestampTo;
    TDuration RequestsShift;
};

/*
 * Добавялет поле present_in_uniproxy_logs
 *
 */
class TAsrPreparedReducer : public NYT::IReducer<NYT::TTableReader<Message>, NYT::TTableWriter<Message>> {
public:
    class TInputOutputTables {
    public:
        TInputOutputTables(const TString& uniproxyPreparedTable, const TString& asrTable,
                           const TString& onlineValidationTable, const TString& asrPreparedTable,
                           const TString& errorTable)
            : UniproxyPreparedTable(uniproxyPreparedTable)
            , AsrTable(asrTable)
            , OnlineValidationTable(onlineValidationTable)
            , AsrPreparedTable(asrPreparedTable)
            , ErrorTable(errorTable) {
        }

        NYT::TReduceOperationSpec AddToOperationSpec(NYT::TReduceOperationSpec&& operationSpec) {
            return operationSpec.AddInput<TUniproxyPrepared>(UniproxyPreparedTable)
                .AddInput<TAsrPrepared::TData>(AsrTable)
                .AddInput<TAsrPrepared::TOnlineValidation>(OnlineValidationTable)
                .AddOutput<TAsrPrepared>(NYT::TRichYPath(AsrPreparedTable)
                                             .Schema(NYT::CreateTableSchema<TAsrPrepared>({"uuid", "message_id"})))
                .AddOutput<TAsrPrepared::TError>(ErrorTable);
        }

        enum EInIndices {
            UniproxyPrepared = 0,
            Asr = 1,
            OnlineValidation = 2,
        };

        enum EOutIndices {
            AsrPrepared = 0,
            Error = 1,
        };

    private:
        const TString& UniproxyPreparedTable;
        const TString& AsrTable;
        const TString& OnlineValidationTable;
        const TString& AsrPreparedTable;
        const TString& ErrorTable;
    };

    void Do(TReader* reader, TWriter* writer) override {
        TAsrPrepared asrPrepared;
        const auto logError = [&asrPrepared, writer](const TAsrPrepared::TError::EReason reason,
                                                     const TString& message) {
            TAsrPrepared::TError error;
            error.SetProcess(TAsrPrepared::TError::P_ASR_PREPARED_REDUCER);
            error.SetReason(reason);
            error.SetMessage(message);
            if (asrPrepared.HasUuid()) {
                error.SetUuid(asrPrepared.GetUuid());
            }
            if (asrPrepared.HasMessageId()) {
                error.SetMessageId(asrPrepared.GetMessageId());
            }
            if (auto setraceUrl =
                    TryGenerateSetraceUrl({error.HasMessageId() ? error.GetMessageId() : TMaybe<TString>{},
                                           error.HasUuid() ? error.GetUuid() : TMaybe<TString>{}})) {
                error.SetSetraceUrl(*setraceUrl);
            }
            writer->AddRow(error, TInputOutputTables::EOutIndices::Error);
        };

        auto setUuid = [logError, &asrPrepared](const TString& uuid) {
            if (!asrPrepared.HasUuid()) {
                asrPrepared.SetUuid(uuid);
            } else if (asrPrepared.GetUuid() != uuid) {
                logError(TAsrPrepared::TError::R_DIFFERENT_VALUES,
                         TStringBuilder{} << "Got different uuid " << asrPrepared.GetUuid() << " " << uuid);
            }
        };
        auto setMessageId = [logError, &asrPrepared](const TString& messageId) {
            if (!asrPrepared.HasMessageId()) {
                asrPrepared.SetMessageId(messageId);
            } else if (asrPrepared.GetMessageId() != messageId) {
                logError(TAsrPrepared::TError::R_DIFFERENT_VALUES, TStringBuilder{} << "Got different message_id "
                                                                                    << asrPrepared.GetMessageId()
                                                                                    << " " << messageId);
            }
        };

        const auto initializeCommonFields = [setUuid, setMessageId](const auto& message) {
            setMessageId(message.GetMessageId());
            setUuid(message.GetUuid());
        };

        asrPrepared.SetPresentInUniproxyLogs(false);
        bool presentInAsrLogs = false;
        asrPrepared.MutablePresence()->SetAsr(false);
        asrPrepared.MutablePresence()->SetOnlineValidation(false);
        for (auto& cursor : *reader) {
            switch (cursor.GetTableIndex()) {
                case TInputOutputTables::EInIndices::UniproxyPrepared: {
                    const auto& uniproxyPrepared = cursor.GetRow<TUniproxyPrepared>();
                    initializeCommonFields(uniproxyPrepared);
                    asrPrepared.SetPresentInUniproxyLogs(true);
                    break;
                }
                case TInputOutputTables::EInIndices::Asr: {
                    const auto& asr = cursor.GetRow<TAsrPrepared::TData>();
                    initializeCommonFields(asr);
                    *asrPrepared.MutableData() = asr.GetData();
                    if (asr.HasTimestampLogMs()) {
                        asrPrepared.SetTimestampLogMs(asr.GetTimestampLogMs());
                    }
                    if (asr.HasTopic()) {
                        asrPrepared.SetTopic(asr.GetTopic());
                    }
                    if (asr.HasAnalyticsInfo()) {
                        *asrPrepared.MutableAnalyticsInfo() = asr.GetAnalyticsInfo();
                    }
                    presentInAsrLogs = true;
                    asrPrepared.MutablePresence()->SetAsr(true);
                    break;
                }
                case TInputOutputTables::EInIndices::OnlineValidation: {
                    const auto& onlineValidation = cursor.GetRow<TAsrPrepared::TOnlineValidation>();
                    initializeCommonFields(onlineValidation);
                    *asrPrepared.MutableOnlineValidation() = onlineValidation.GetValue();
                    presentInAsrLogs = true;
                    asrPrepared.MutablePresence()->SetOnlineValidation(true);
                    break;
                }
                default:
                    Y_FAIL("Unexpected table index in TAsrPreparedReducer");
            }
        }
        if (presentInAsrLogs) {
            writer->AddRow(asrPrepared, TInputOutputTables::EOutIndices::AsrPrepared);
        }
    }
};

REGISTER_MAPPER(TAsrMapper)
REGISTER_REDUCER(TAsrPreparedReducer)

} // namespace

void MakeAsrPrepared(NYT::IClientPtr client, const TString& tmpDirectory, const TString& uniproxyPrepared,
                     const TVector<TString>& asrLogs, const TString& outputTable, const TString& errorTable,
                     const TInstant& timestampFrom, const TInstant& timestampTo, const TDuration& requestsShift) {
    auto tx = client->StartTransaction();

    const auto asrTmpTable = CreateRandomTable(tx, tmpDirectory, "asr");
    const auto asrTmpTableSorted = CreateRandomTable(tx, tmpDirectory, "asr-sorted");
    const auto onlineValidationTmpTable = CreateRandomTable(tx, tmpDirectory, "online-validation");
    const auto onlineValidationTmpTableSorted = CreateRandomTable(tx, tmpDirectory, "online-validation-sorted");

    CreateTable(tx, outputTable, TInstant::Now() + MONTH_TTL);
    CreateTable(tx, errorTable, TInstant::Now() + MONTH_TTL);

    TVector<TString> errorTables;
    for (int i = 0; i < 2; i++) {
        errorTables.push_back(CreateRandomTable(tx, tmpDirectory, "error-table"));
    }
    tx->Map(TAsrMapper::TInputOutputTables{asrLogs, asrTmpTable, onlineValidationTmpTable, errorTables[0]}
                .AddToOperationSpec(NYT::TMapOperationSpec{})
                .MapperSpec(NYT::TUserJobSpec{}.MemoryLimit(3_GB).MemoryReserveFactor(26)),
            new TAsrMapper{timestampFrom, timestampTo, requestsShift},
            NYT::TOperationOptions{}.InferOutputSchema(true));

    ;

    NYT::TOperationTracker tracker;
    for (const auto& [table, tableSorted] : {std::tie(asrTmpTable, asrTmpTableSorted),
                                             std::tie(onlineValidationTmpTable, onlineValidationTmpTableSorted)}) {
        tracker.AddOperation(
            tx->Sort(NYT::TSortOperationSpec{}.AddInput(table).Output(tableSorted).SortBy({"uuid", "message_id"}),
                     NYT::TOperationOptions{}.Wait(false)));
    }
    tracker.WaitAllCompleted();

    tx->Reduce(TAsrPreparedReducer::TInputOutputTables{uniproxyPrepared, asrTmpTableSorted,
                                                       onlineValidationTmpTableSorted, outputTable, errorTables[1]}
                   .AddToOperationSpec(NYT::TReduceOperationSpec{})
                   .ReducerSpec(NYT::TUserJobSpec{}.MemoryLimit(1_GB).MemoryReserveFactor(18))
                   .ReduceBy({"uuid", "message_id"}),
               new TAsrPreparedReducer, NYT::TOperationOptions{});

    tx->Concatenate(errorTables, errorTable);
    tx->Commit();
}

} // namespace NAlice::NWonderlogs
