#include "processor.h"

#include <alice/wonderlogs/library/common/utils.h>
#include <alice/wonderlogs/library/parsers/megamind.h>
#include <alice/wonderlogs/library/parsers/uniproxy.h>

#include <ads/bsyeti/big_rt/lib/queue/qyt/queue.h>

#include <library/cpp/yson/node/node_io.h>

#include <logfeller/lib/chunk_splitter/chunk_splitter.h>

#include <yt/yt/client/api/transaction.h>

namespace NAlice::NWonderlogs {

TMegamindResharder::TMegamindResharder(NBigRT::TStatelessShardProcessor::TConstructionArgs spArgs,
                                       const TMegamindResharderConfig::TShardingConfig& config)
    : TStatelessShardProcessor(spArgs)
    , Config(config) {
}

void TMegamindResharder::Process(TString /* dataSource */, NBigRT::TMessageBatch messageBatch) {
    for (auto& message : messageBatch.Messages) {
        if (!message.Unpack()) {
            continue;
        }
        NLogFeller::NChunkSplitter::TRecordContext context;
        TStringBuf record, skip;

        auto splitter{NLogFeller::NChunkSplitter::CreateChunkSplitter(Config.GetSplitter().GetLogfellerSplitter())};
        // ui64 failsParseJson = 0;
        // ui64 addedRows = 0;
        ui64 skippedRows = 0;
        for (auto iterator = splitter->CreateIterator(message.Data); iterator.NextRecord(record, skip, context);) {
            const auto mmReqRespNode = NYT::NodeFromJsonString(record);
            TMegamindLogsParser logsParser(mmReqRespNode);
            const auto parsedLog = logsParser.Parse();
            if (parsedLog.RequestResponse) {
                const auto hash = HashStringToUi64(parsedLog.RequestResponse->GetUuid());

                if (hash % 100 < Config.GetSkipUsersPercent()) {
                    ++skippedRows;
                    continue;
                }
                const auto requestResponse = parsedLog.RequestResponse;
                Data[hash % Config.GetShardsCount()].push_back(requestResponse->SerializeAsString());
            }
        }
        // ctx.Get<NSFStats::TSumMetric<ui64>>("fails_parse_json_per_second").Inc(failsParseJson);
        // ctx.Get<NSFStats::TSumMetric<ui64>>("added_rows_per_second").Inc(addedRows);
        // ctx.Get<NSFStats::TSumMetric<ui64>>("skipped_rows_per_second").Inc(skippedRows);

        message.Data.clear();
    }
}

NYT::TFuture<TMegamindResharder::TPrepareForAsyncWriteResult> TMegamindResharder::PrepareForAsyncWrite() {
    auto dataForWrite = MakeAtomicShared<decltype(Data)>(std::move(Data));
    Data.clear();
    return NYT::MakeFuture<TPrepareForAsyncWriteResult>({.AsyncWriter = [this,
                                                                         dataForWrite](NYT::NApi::ITransactionPtr tx) {
        NBigRT::TYtQueue{Config.GetOutputQueue(), tx->GetClient()}.Write(tx, *dataForWrite, Config.GetOutputCodec());
    }});
}

} // namespace NAlice::NWonderlogs
