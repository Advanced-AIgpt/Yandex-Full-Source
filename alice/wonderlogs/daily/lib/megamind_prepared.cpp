#include "megamind_prepared.h"

#include "ttls.h"

#include <alice/wonderlogs/library/builders/megamind.h>
#include <alice/wonderlogs/library/common/names.h>
#include <alice/wonderlogs/library/common/utils.h>
#include <alice/wonderlogs/library/parsers/megamind.h>
#include <alice/wonderlogs/library/yt/utils.h>

#include <alice/megamind/protos/common/events.pb.h>
#include <alice/megamind/protos/speechkit/directives.pb.h>
#include <alice/megamind/protos/speechkit/request.pb.h>
#include <alice/megamind/protos/speechkit/response.pb.h>

#include <alice/library/json/json.h>

#include <library/cpp/json/json_writer.h>
#include <library/cpp/json/yson/json2yson.h>
#include <library/cpp/yson/node/node_io.h>

#include <mapreduce/yt/library/operation_tracker/operation_tracker.h>

#include <util/charset/unidata.h>
#include <util/generic/size_literals.h>
#include <util/string/builder.h>

using namespace NAlice::NWonderlogs;

namespace {

using google::protobuf::Message;

/*
 * Создаёт таблицу speechkit request-response из megamind с uuid, megamind_request_id, megamind_response_id для
 * последующего редьюса
 *
 */
class TMegamindRequestResponseMapper : public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<Message>> {
public:
    class TInputOutputTables {
    public:
        TInputOutputTables(const TVector<TString>& megamindAnalyticsLogsTables,
                           const TString& megamindRequestResponseTable, const TString& errorTable)
            : MegamindAnalyticsLogsTables(megamindAnalyticsLogsTables)
            , MegamindRequestResponseTable(megamindRequestResponseTable)
            , ErrorTable(errorTable) {
        }

        NYT::TMapOperationSpec AddToOperationSpec(NYT::TMapOperationSpec&& operationSpec) {
            for (const auto& table : MegamindAnalyticsLogsTables) {
                operationSpec = operationSpec.AddInput<NYT::TNode>(table);
            }
            return operationSpec.AddOutput<TMegamindPrepared::TMegamindRequestResponse>(MegamindRequestResponseTable)
                .AddOutput<TMegamindPrepared::TError>(ErrorTable);
        }

        enum EOutIndices {
            RequestResponse = 0,
            Error = 1,
        };

    private:
        const TVector<TString>& MegamindAnalyticsLogsTables;
        const TString& MegamindRequestResponseTable;
        const TString& ErrorTable;
    };

    Y_SAVELOAD_JOB(TimestampFrom, TimestampTo);
    TMegamindRequestResponseMapper() = default;
    TMegamindRequestResponseMapper(const TInstant& timestampFrom, const TInstant& timestampTo)
        : TimestampFrom(timestampFrom)
        , TimestampTo(timestampTo) {
    }

    void Do(TReader* reader, TWriter* writer) override {
        for (auto& cursor : *reader) {
            const auto& row = cursor.GetRow();
            const auto timestamp = TInstant::Seconds(row["_logfeller_timestamp"].AsUint64());
            if (SkipNotRequest(timestamp, TimestampFrom, TimestampTo)) {
                continue;
            }
            TMegamindLogsParser parser(row);
            const auto parsedLog = parser.Parse();
            if (parsedLog.RequestResponse) {
                writer->AddRow(*parsedLog.RequestResponse, TInputOutputTables::EOutIndices::RequestResponse);
            }
            for (const auto& error : parsedLog.Errors) {
                writer->AddRow(error, TInputOutputTables::EOutIndices::Error);
            }
        }
    }

private:
    TInstant TimestampFrom;
    TInstant TimestampTo;
};

/*
 * Редьюсит uniproxy-prepard с таблицей request-response по uuid, megamind_request_id
 * Возможны 2 принципиальных случая:
 * 1. запись есть в uniproxy, значит можем восстановить message_id
 * 2. записи нет в uniproxy, значит просто сортируем по серверному времени и берём с максимальным
 *
 */
class TMegamindPreparedReducer : public NYT::IReducer<NYT::TTableReader<Message>, NYT::TTableWriter<Message>> {
public:
    class TInputOutputTables {
    public:
        TInputOutputTables(const TString& uniproxyPreparedTable, const TString& megamindRequestResponseTable,
                           const TString& megamindPreparedTable, const TString& errorTable)
            : UniproxyPreparedTable(uniproxyPreparedTable)
            , MegamindRequestResponseTable(megamindRequestResponseTable)
            , MegamindPreparedTable(megamindPreparedTable)
            , ErrorTable(errorTable) {
        }

        NYT::TReduceOperationSpec AddToOperationSpec(NYT::TReduceOperationSpec&& operationSpec) {
            return operationSpec.AddInput<TUniproxyPrepared>(UniproxyPreparedTable)
                .AddInput<TMegamindPrepared::TMegamindRequestResponse>(MegamindRequestResponseTable)
                .AddOutput<TMegamindPrepared>(MegamindPreparedTable)
                .AddOutput<TMegamindPrepared::TError>(ErrorTable);
        }

        enum EInIndices {
            UniproxyPrepared = 0,
            MegamindRequestResponse = 1,
        };

        enum EOutIndices {
            MegamindPrepared = 0,
            Error = 1,
        };

    private:
        const TString& UniproxyPreparedTable;
        const TString& MegamindRequestResponseTable;
        const TString& MegamindPreparedTable;
        const TString& ErrorTable;
    };

    void Do(TReader* reader, TWriter* writer) override {
        TMaybe<TMegamindPrepared::TMegamindRequestResponse> successfulMegamindRequestResponse;
        TMaybe<TUniproxyPrepared> successfulUniproxyPrepared;
        const auto logError = [writer, &successfulMegamindRequestResponse, &successfulUniproxyPrepared](
                                  const TMegamindPrepared::TError::EReason reason, const TString& message) {
            TMegamindPrepared::TError error;
            error.SetProcess(TMegamindPrepared::TError::P_MEGAMIND_PREPARED_REDUCER);
            error.SetReason(reason);
            error.SetMessage(message);
            if (successfulMegamindRequestResponse && successfulMegamindRequestResponse->HasUuid()) {
                error.SetUuid(successfulMegamindRequestResponse->GetUuid());
            } else if (successfulUniproxyPrepared && successfulUniproxyPrepared->HasUuid()) {
                error.SetUuid(successfulUniproxyPrepared->GetUuid());
            }
            if (successfulMegamindRequestResponse && successfulMegamindRequestResponse->HasRequestId()) {
                error.SetRequestId(successfulMegamindRequestResponse->GetRequestId());
            } else if (successfulUniproxyPrepared && successfulUniproxyPrepared->HasMegamindRequestId()) {
                error.SetUuid(successfulUniproxyPrepared->GetMegamindRequestId());
            }
            if (successfulMegamindRequestResponse && successfulMegamindRequestResponse->HasMessageId()) {
                error.SetMessageId(successfulMegamindRequestResponse->GetMessageId());
            } else if (successfulUniproxyPrepared && successfulUniproxyPrepared->HasMessageId()) {
                error.SetUuid(successfulUniproxyPrepared->GetMessageId());
            }
            if (auto setraceUrl =
                    TryGenerateSetraceUrl({error.HasMessageId() ? error.GetMessageId() : TMaybe<TString>{},
                                           error.HasRequestId() ? error.GetRequestId() : TMaybe<TString>{},
                                           error.HasUuid() ? error.GetUuid() : TMaybe<TString>{}})) {
                error.SetSetraceUrl(*setraceUrl);
            }
            writer->AddRow(error, TInputOutputTables::EOutIndices::Error);
        };
        for (auto& cursor : *reader) {
            switch (cursor.GetTableIndex()) {
                case TInputOutputTables::EInIndices::UniproxyPrepared: {
                    const auto& uniproxyPrepared = cursor.GetRow<TUniproxyPrepared>();
                    if (uniproxyPrepared.GetSuccessfulClientRetry()) {
                        if (successfulUniproxyPrepared) {
                            logError(TMegamindPrepared::TError::R_INVALID_VALUE,
                                     TStringBuilder{} << "Unexpected second uniproxy_prepared value"
                                                      << ToString(*successfulUniproxyPrepared));
                        }
                        successfulUniproxyPrepared = uniproxyPrepared;
                    }
                    break;
                }
                case TInputOutputTables::EInIndices::MegamindRequestResponse: {
                    // TODO investigate https://st.yandex-team.ru/MEGAMIND-1674
                    // https://yql.yandex-team.ru/Operations/XxYA9BpqvwcQ05AkZfeJL8YyqwgCcU81aerH3bCdc9w=
                    const auto& curMegamindRequestResponse =
                        cursor.GetRow<TMegamindPrepared::TMegamindRequestResponse>();

                    if (successfulMegamindRequestResponse && successfulMegamindRequestResponse->GetResponseId() ==
                                                                 curMegamindRequestResponse.GetResponseId()) {
                        logError(TMegamindPrepared::TError::R_INVALID_VALUE,
                                 TStringBuilder{} << "Dublicate row with the same response_id: "
                                                  << curMegamindRequestResponse.GetResponseId() << " "
                                                  << ToString(*successfulMegamindRequestResponse));
                    }

                    if (PreferableRequestResponse(successfulUniproxyPrepared, successfulMegamindRequestResponse,
                                                  curMegamindRequestResponse)) {
                        successfulMegamindRequestResponse = curMegamindRequestResponse;
                    }
                    break;
                }
                default:
                    Y_FAIL("Unexpected table index in TMegamindPreparedReducer");
            }
        }

        const auto initializeMegamindPrepared =
            [logError](TMegamindPrepared& megamindPrepared,
                       const TMegamindPrepared::TMegamindRequestResponse& successfulMegamindRequestResponse,
                       const TMaybe<TUniproxyPrepared>& successfulUniproxyPrepared) {
                megamindPrepared.SetUuid(successfulMegamindRequestResponse.GetUuid());
                if (successfulMegamindRequestResponse.HasSpeechKitRequest()) {
                    *megamindPrepared.MutableSpeechkitRequest() =
                        successfulMegamindRequestResponse.GetSpeechKitRequest();
                }
                if (successfulMegamindRequestResponse.HasSpeechKitResponse()) {
                    *megamindPrepared.MutableSpeechkitResponse() =
                        successfulMegamindRequestResponse.GetSpeechKitResponse();
                }
                megamindPrepared.SetTimestampLogMs(successfulMegamindRequestResponse.GetTimestampLogMs());
                *megamindPrepared.MutableEnvironment() = successfulMegamindRequestResponse.GetEnvironment();

                if (successfulMegamindRequestResponse.HasMessageId()) {
                    megamindPrepared.SetRealMessageId(true);
                    megamindPrepared.SetMessageId(successfulMegamindRequestResponse.GetMessageId());
                }
                if (successfulUniproxyPrepared) {
                    megamindPrepared.SetPresentInUniproxyLogs(true);
                    if (!megamindPrepared.HasMessageId() || megamindPrepared.GetMessageId().empty()) {
                        megamindPrepared.SetMessageId(successfulUniproxyPrepared->GetMessageId());
                    } else if (megamindPrepared.GetMessageId() != successfulUniproxyPrepared->GetMessageId()) {
                        logError(TMegamindPrepared::TError::R_DIFFERENT_VALUES,
                                 TStringBuilder{} << "Got different message_id: " << megamindPrepared.GetMessageId()
                                                  << " " << successfulUniproxyPrepared->GetMessageId());
                    }
                } else {
                    megamindPrepared.SetPresentInUniproxyLogs(false);
                }
                if (!megamindPrepared.HasMessageId()) {
                    megamindPrepared.SetMessageId(CreateGuidAsString());
                    megamindPrepared.SetRealMessageId(false);
                }
            };

        if (successfulMegamindRequestResponse) {
            TMegamindPrepared megamindPrepared;
            initializeMegamindPrepared(megamindPrepared, *successfulMegamindRequestResponse,
                                       successfulUniproxyPrepared);
            writer->AddRow(megamindPrepared, TInputOutputTables::EOutIndices::MegamindPrepared);
        }
    }
};

REGISTER_MAPPER(TMegamindRequestResponseMapper)
REGISTER_REDUCER(TMegamindPreparedReducer)

} // namespace

namespace NAlice::NWonderlogs {

void MakeMegamindPrepared(NYT::IClientPtr client, const TString& tmpDirectory, const TString& uniproxyPrepared,
                          const TVector<TString>& megamindAnalyticsLogs, const TString& outputTable,
                          const TString& errorTable, const TInstant& timestampFrom, const TInstant& timestampTo) {
    auto tx = client->StartTransaction();

    const auto megamindRequestResponseTmpTable = CreateRandomTable(tx, tmpDirectory, "megamind-request-response");
    const auto megamindRequestResponseTmpTableSorted =
        CreateRandomTable(tx, tmpDirectory, "megamind-request-response-sorted");
    const auto uniproxyPreparedTmpTableUnsorted = CreateRandomTable(tx, tmpDirectory, "uniproxy-prepared-unsorted");
    const auto megamindPreparedTmpTableUnsorted = CreateRandomTable(tx, tmpDirectory, "megamind-prepared-unsorted");

    CreateTable(tx, outputTable, TInstant::Now() + MONTH_TTL);
    CreateTable(tx, errorTable, TInstant::Now() + MONTH_TTL);

    TVector<TString> errorTables;
    for (int i = 0; i < 2; i++) {
        errorTables.push_back(CreateRandomTable(tx, tmpDirectory, "error-table"));
    }
    tx->Map(TMegamindRequestResponseMapper::TInputOutputTables{megamindAnalyticsLogs, megamindRequestResponseTmpTable,
                                                               errorTables[0]}
                .AddToOperationSpec(NYT::TMapOperationSpec{})
                .MapperSpec(NYT::TUserJobSpec{}.MemoryLimit(1_GB).MemoryReserveFactor(13)),
            new TMegamindRequestResponseMapper{timestampFrom, timestampTo},
            NYT::TOperationOptions{}.InferOutputSchema(true));

    {
        NYT::TOperationTracker tracker;
        for (const auto& [notSorted, sorted] :
             {std::tie(megamindRequestResponseTmpTable, megamindRequestResponseTmpTableSorted),
              std::tie(uniproxyPrepared, uniproxyPreparedTmpTableUnsorted)}) {
            tracker.AddOperation(tx->Sort(
                NYT::TSortOperationSpec{}.AddInput(notSorted).Output(sorted).SortBy({"uuid", "megamind_request_id"}),
                NYT::TOperationOptions{}.Wait(false)));
        }

        tracker.WaitAllCompleted();
    }

    tx->Reduce(TMegamindPreparedReducer::TInputOutputTables{uniproxyPreparedTmpTableUnsorted,
                                                            megamindRequestResponseTmpTableSorted,
                                                            megamindPreparedTmpTableUnsorted, errorTables[1]}
                   .AddToOperationSpec(NYT::TReduceOperationSpec{})
                   .ReducerSpec(NYT::TUserJobSpec{}.MemoryLimit(1_GB).MemoryReserveFactor(16))
                   // TODO(ran1s) reduce must be by {uuid, message_id}
                   .ReduceBy({"uuid", "megamind_request_id"}),
               new TMegamindPreparedReducer, NYT::TOperationOptions{}.InferOutputSchema(true));

    tx->Sort(NYT::TSortOperationSpec{}
                 .AddInput(megamindPreparedTmpTableUnsorted)
                 .Output(outputTable)
                 .SortBy({"uuid", "message_id"}));

    tx->Concatenate(errorTables, errorTable);
    tx->Commit();
}

} // namespace NAlice::NWonderlogs
