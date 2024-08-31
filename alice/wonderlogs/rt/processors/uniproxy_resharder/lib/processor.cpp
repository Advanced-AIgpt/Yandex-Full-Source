#include "processor.h"

#include <alice/wonderlogs/rt/protos/uniproxy_event.pb.h>

#include <alice/wonderlogs/library/common/utils.h>
#include <alice/wonderlogs/library/parsers/uniproxy.h>

#include <ads/bsyeti/big_rt/lib/queue/qyt/queue.h>

#include <logfeller/lib/chunk_splitter/chunk_splitter.h>

#include <yt/yt/client/api/transaction.h>

namespace NAlice::NWonderlogs {

TUniproxyResharder::TUniproxyResharder(NBigRT::TStatelessShardProcessor::TConstructionArgs spArgs,
                                       const TUniproxyResharderConfig::TShardingConfig& config)
    : TStatelessShardProcessor(spArgs)
    , Config(config) {
}

void TUniproxyResharder::Process(TString /* dataSource */, NBigRT::TMessageBatch messageBatch) {
    for (auto& message : messageBatch.Messages) {
        if (!message.Unpack()) {
            continue;
        }

        NLogFeller::NChunkSplitter::TRecordContext context;
        TStringBuf record, skip;

        auto splitter{NLogFeller::NChunkSplitter::CreateChunkSplitter(Config.GetSplitter().GetLogfellerSplitter())};
        ui64 failsParseJson = 0;
        ui64 addedRows = 0;
        ui64 skippedRows = 0;
        for (auto iterator = splitter->CreateIterator(message.Data); iterator.NextRecord(record, skip, context);) {
            NJson::TJsonValue json;
            if (!NJson::ReadJsonTree(record, &json)) {
                ++failsParseJson;
                continue;
            }
            constexpr TStringBuf MESSAGE = "message";
            if (!json.IsMap() || !json.Has(MESSAGE) || !json[MESSAGE].IsString()) {
                ++failsParseJson;
                continue;
            }
            TStringBuf logMessage = json[MESSAGE].GetString();
            if (!logMessage.SkipPrefix(SESSIONLOG)) {
                ++skippedRows;
                continue;
            }
            NJson::TJsonValue messageJson;
            if (!NJson::ReadJsonTree(logMessage, &messageJson)) {
                ++failsParseJson;
                continue;
            }
            const auto uuid = ParseUuid(messageJson);
            if (!uuid) {
                ++failsParseJson;
                continue;
            }

            const auto hash = HashStringToUi64(*uuid);

            if (hash % 100 < Config.GetSkipUsersPercent()) {
                ++skippedRows;
                continue;
            }

            ++addedRows;

            const auto timestamp = TInstant::Seconds(messageJson["_logfeller_timestamp"].GetUInteger());

            TMaybe<TUniproxyPrepared::TEnvironment> environment;
            if (messageJson["qloud_application"].IsString() && messageJson["qloud_project"].IsString()) {
                TUniproxyPrepared::TEnvironment env;
                env.SetQloudApplication(messageJson["qloud_application"].GetString());
                env.SetQloudProject(messageJson["qloud_project"].GetString());
                environment = env;
            }

            TUniproxyLogsParser logsParser(messageJson, environment, timestamp, logMessage);
            TUniproxyEvent uniproxyEvent;

            switch (logsParser.GetType()) {
                case EUniproxyEventType::MegamindRequest: {
                    const auto mmRequest = logsParser.ParseMegamindRequest().ParsedEvent;
                    if (mmRequest) {
                        *uniproxyEvent.MutableMegamindRequest() = *mmRequest;
                    }
                    break;
                }
                case EUniproxyEventType::MegamindResponse: {
                    const auto mmResponse = logsParser.ParseMegamindResponse().ParsedEvent;
                    if (mmResponse) {
                        *uniproxyEvent.MutableMegamindResponse() = *mmResponse;
                    }
                    break;
                }
                case EUniproxyEventType::RequestStat:
                    [[fallthrough]];
                case EUniproxyEventType::SpotterValidation:
                    [[fallthrough]];
                case EUniproxyEventType::SpotterStream:
                    [[fallthrough]];
                case EUniproxyEventType::Stream:
                    [[fallthrough]];
                case EUniproxyEventType::LogSpotter:
                    [[fallthrough]];
                case EUniproxyEventType::MessageIdToConnectSessionId:
                    [[fallthrough]];
                case EUniproxyEventType::MessageIdToEnvironment:
                    [[fallthrough]];
                case EUniproxyEventType::MessageIdToClientIp:
                    [[fallthrough]];
                case EUniproxyEventType::SynchronizeState:
                    [[fallthrough]];
                case EUniproxyEventType::VoiceInput:
                    [[fallthrough]];
                case EUniproxyEventType::AsrRecognize:
                    [[fallthrough]];
                case EUniproxyEventType::AsrResult:
                    [[fallthrough]];
                case EUniproxyEventType::MegamindTimings:
                    [[fallthrough]];
                case EUniproxyEventType::TtsTimings:
                    [[fallthrough]];
                case EUniproxyEventType::TtsGenerate:
                    [[fallthrough]];
                case EUniproxyEventType::MessageIdToDoNotUseUserLogs:
                    [[fallthrough]];
                case EUniproxyEventType::AsrDebug:
                    [[fallthrough]];
                case EUniproxyEventType::TestIds:
                    [[fallthrough]];
                case EUniproxyEventType::Unknown: {
                    break;
                }
            }

            if (TUniproxyEvent::VALUE_NOT_SET != uniproxyEvent.Value_case()) {
                Data[hash % Config.GetShardsCount()].push_back(uniproxyEvent.SerializeAsString());
            }
        }
        // ctx.Get<NSFStats::TSumMetric<ui64>>("fails_parse_json_per_second").Inc(failsParseJson);
        // ctx.Get<NSFStats::TSumMetric<ui64>>("added_rows_per_second").Inc(addedRows);
        // ctx.Get<NSFStats::TSumMetric<ui64>>("skipped_rows_per_second").Inc(skippedRows);

        message.Data.clear();
    }
}

NYT::TFuture<TUniproxyResharder::TPrepareForAsyncWriteResult> TUniproxyResharder::PrepareForAsyncWrite() {
    auto dataForWrite = MakeAtomicShared<decltype(Data)>(std::move(Data));
    Data.clear();
    return NYT::MakeFuture<TPrepareForAsyncWriteResult>({.AsyncWriter = [this,
                                                                         dataForWrite](NYT::NApi::ITransactionPtr tx) {
        NBigRT::TYtQueue{Config.GetOutputQueue(), tx->GetClient()}.Write(tx, *dataForWrite, Config.GetOutputCodec());
    }});
}

} // namespace NAlice::NWonderlogs
