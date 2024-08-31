#include "processor.h"

#include <alice/wonderlogs/protos/wonderlogs.pb.h>

#include <alice/megamind/library/request/event/event.h>
#include <alice/megamind/protos/common/events.pb.h>
#include <alice/megamind/protos/speechkit/request.pb.h>
#include <alice/megamind/protos/speechkit/response.pb.h>

#include <dict/dictutil/str.h>

#include <mapreduce/yt/interface/client.h>

namespace NAlice::NWonderlogs {

TStaticTableProcessor::TStaticTableProcessor(NBigRT::TStatelessShardProcessor::TConstructionArgs spArgs,
                                             const TStaticTableCreatorConfig::TProcessorConfig& config)
    : NBigRT::TStatelessShardProcessor(spArgs)
    , Config(config) {
}

void TStaticTableProcessor::Process(TString /* dataSource */, NBigRT::TMessageBatch messageBatch) {
    auto ctx = NSFStats::TSolomonContext(SensorsContext, /* labels= */ {}).Detached();
    ui64 invalidProtosPerSecond = 0;
    for (auto& message : messageBatch.Messages) {
        if (!message.Unpack()) {
            continue;
        }
        TWonderlog wonderlog;
        if (!wonderlog.ParseFromString(message.Data)) {
            ++invalidProtosPerSecond;
            continue;
        }

        TLogviewer logviewer;
        logviewer.SetUuid(wonderlog.GetUuid());
        logviewer.SetMessageId(wonderlog.GetMessageId());
        const auto& event = wonderlog.GetSpeechkitRequest().GetRequest().GetEvent();
        logviewer.SetReqId(wonderlog.GetMegamindRequestId());
        // Moscow to UTC easiest approach;
        const i64 threeHours = 3 * 60 * 60;
        TString timestamp =
            TInstant::MilliSeconds(
                wonderlog.GetSpeechkitRequest().GetRequest().GetAdditionalOptions().GetServerTimeMs() + threeHours)
                .ToStringUpToSeconds();
        // ToStringUpToSeconds() returns '2015-11-21T23:30:27Z'. We don't need Z at the end
        timestamp.pop_back();
        ReplaceAll(timestamp, 'T', ' ');
        logviewer.SetTs(timestamp);
        switch (event.GetType()) {
            case NAlice::EEventType::text_input:
                if (event.HasText()) {
                    logviewer.SetQuery(event.GetText());
                }
                if (wonderlog.GetSpeechkitResponse().GetResponse().CardsSize() > 0 &&
                    wonderlog.GetSpeechkitResponse().GetResponse().GetCards(0).HasText()) {
                    logviewer.SetReply(wonderlog.GetSpeechkitResponse().GetResponse().GetCards(0).GetText());
                }
            case NAlice::EEventType::voice_input:
                if (event.AsrResultSize() > 0 && event.GetAsrResult(0).HasNormalized()) {
                    logviewer.SetQuery(event.GetAsrResult(0).GetNormalized());
                }
                if (wonderlog.GetSpeechkitResponse().GetVoiceResponse().GetOutputSpeech().HasText()) {
                    logviewer.SetReply(
                        wonderlog.GetSpeechkitResponse().GetVoiceResponse().GetOutputSpeech().GetText());
                }
            case NAlice::EEventType::suggested_input:
                [[fallthrough]];
            case NAlice::EEventType::music_input:
                [[fallthrough]];
            case NAlice::EEventType::image_input:
                [[fallthrough]];
            case NAlice::EEventType::server_action:;
        }
        if (logviewer.HasQuery() && logviewer.HasReply()) {
            LogviwerData.push_back(logviewer);
        }

        message.Data.clear();
    }
    ctx.Get<NSFStats::TSumMetric<ui64>>("invalid_protos_per_second").Inc(invalidProtosPerSecond);
}

NYT::TFuture<TStaticTableProcessor::TPrepareForAsyncWriteResult> TStaticTableProcessor::PrepareForAsyncWrite() {
    auto logviewerData(std::move(LogviwerData));
    LogviwerData.clear();

    // Moscow to UTC easiest approach;
    const i64 threeHours = 3 * 60 * 60;

    TString tableName = TInstant::Seconds(TInstant::Now().Seconds() / (Config.GetGranularityMinutes() * 60) *
                                              (Config.GetGranularityMinutes() * 60) +
                                          threeHours)
                            .ToStringUpToSeconds();
    // ToStringUpToSeconds() returns '2015-11-21T23:30:27Z'. We don't need Z at the end
    tableName.pop_back();

    return NYT::MakeFuture<TPrepareForAsyncWriteResult>(
        {.AsyncWriter = [this, data = std::move(logviewerData), tableName](NYT::NApi::ITransactionPtr /* tx */) {
            auto client = NYT::CreateClient(Config.GetCluster());
            auto writer = client->CreateTableWriter<TLogviewer>(
                NYT::TRichYPath(Config.GetOutputDir() + "/" + tableName).Append(true));

            for (const auto& logviewer : data) {
                writer->AddRow(logviewer);
            }

            writer->Finish();
        }});
}

} // namespace NAlice::NWonderlogs
