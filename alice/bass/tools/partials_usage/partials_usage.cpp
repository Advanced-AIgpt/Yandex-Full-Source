#include <alice/bass/tools/partials_usage/data.pb.h>

#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/interface/init.h>
#include <mapreduce/yt/library/operation_tracker/operation_tracker.h>
#include <mapreduce/yt/util/temp_table.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_value.h>
#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/protobuf/yt/proto2yt.h>
#include <library/cpp/threading/future/async.h>
#include <library/cpp/timezone_conversion/civil.h>

#include <util/datetime/base.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>
#include <util/string/cast.h>
#include <util/string/split.h>
#include <util/system/user.h>
#include <util/thread/pool.h>

#include <future>

using namespace NYT;
using namespace NDatetime;

class TFilterMapper
    : public IMapper<TTableReader<TNode>, TTableWriter<TFilteredMessage>> {
public:
    void Do(TReader* reader, TWriter* writer) override {
        for(; reader->IsValid(); reader->Next()) {
            const auto& row = reader->GetRow();
            TStringBuf messageBuf = row["message"].AsString();
            auto logType = messageBuf.NextTok(" ");
            if (logType != TStringBuf("SESSIONLOG:") || !messageBuf.Contains("VinsPartial"))
                continue;
            NJson::TJsonValue messageJson;
            if (!NJson::ReadJsonTree(messageBuf, &messageJson, false /* throwOnError */ )) {
                continue;
            }
            TFilteredMessage outRow;
            outRow.SetStatus(messageJson["Directive"]["status"].GetString());
            writer->AddRow(outRow);
        }
    }
};
REGISTER_MAPPER(TFilterMapper);

class TCountPartialsUsageReducer
    : public IReducer<TTableReader<TFilteredMessage>, TTableWriter<TStatus>> {
public:
    void Do(TReader* reader, TWriter* writer) override {
        const auto& row = reader->GetRow();
        TString status = row.GetStatus();

        ui64 count = 0;
        for (; reader->IsValid(); reader->Next()) {
            ++count;
        }
        TStatus outRow;
        outRow.SetStatus(status);
        outRow.SetCount(count);
        writer->AddRow(outRow);
    }
};
REGISTER_REDUCER(TCountPartialsUsageReducer);

class TTransposeMapper
    : public IMapper<TTableReader<TStatus>, TTableWriter<TTransposedStatus>> {
public:
    TTransposeMapper() = default;
    explicit TTransposeMapper(const TString& date) : Date(date) {}

    Y_SAVELOAD_JOB(Date);

    void Do(TReader *reader, TWriter* writer) override {
        TTransposedStatus outRow;
        ui64 total = 0;
        for (; reader->IsValid(); reader->Next()) {
            const auto& row = reader->GetRow();
            TStringBuf status = row.GetStatus();
            ui64 count = row.GetCount();
            if (status == "good_partial")
                outRow.SetGoodPartial(count);
            else if (status == "bad_partial")
                outRow.SetBadPartial(count);
            else if (status == "partial_206")
                outRow.SetPartial206(count);
            else
                ythrow yexception() << "Incorrect status : " << status.data();
            total += count;
        }
        outRow.SetTotal(total);
        outRow.SetDate(Date);
        writer->AddRow(outRow);
    }

private:
    TString Date;
};
REGISTER_MAPPER(TTransposeMapper);

class TGlueReducer
    : public IReducer<TTableReader<TTransposedStatus>, TTableWriter<TTransposedStatus>> {
public:
    void Do(TReader* reader, TWriter* writer) override {
        for (; reader->IsValid(); reader->Next()) {
            auto const& row = reader->GetRow();
            writer->AddRow(row);
        }
    }
};
REGISTER_REDUCER(TGlueReducer);

void FilterUniproxyLogs(TIntrusivePtr<IClient> client,
                        const TString& inputTableName,
                        const TString& outputTableName)
{
    client->Map(
        TMapOperationSpec()
            .AddInput<TNode>(inputTableName)
            .AddOutput<TFilteredMessage>(outputTableName),
        new TFilterMapper);
}

void SortFilteredMessage(TIntrusivePtr<IClient> client,
                         const TString& inputTableName,
                         const TString& outputTableName)
{
    client->Sort(
        TSortOperationSpec()
            .AddInput(inputTableName)
            .Output(outputTableName)
            .SortBy({"status"}));
}

void ReduceStatusCount(TIntrusivePtr<IClient> client,
                       const TString& inputTableName,
                       const TString& outputTableName)
{
    client->Reduce(
        TReduceOperationSpec()
            .ReduceBy({"status"})
            .AddInput<TFilteredMessage>(inputTableName)
            .AddOutput<TStatus>(outputTableName),
        new TCountPartialsUsageReducer);
}

void TransposeTable(TIntrusivePtr<IClient> client,
                      const TString& inputTableName,
                      const TString& outputTableName,
                      const TString& date)
{
    client->Map(
        TMapOperationSpec()
            .AddInput<TStatus>(inputTableName)
            .AddOutput<TTransposedStatus>(outputTableName),
        new TTransposeMapper(date));
}


TTempTable Process(TIntrusivePtr<IClient> client, const TString& day) {
    TTempTable filteredTable(client);
    FilterUniproxyLogs(client, "//logs/alice-production-uniproxy/1d/" + day, filteredTable.Name());

    TTempTable sortedTable(client);
    SortFilteredMessage(client, filteredTable.Name(), sortedTable.Name());

    TTempTable reducedTable(client);
    ReduceStatusCount(client, sortedTable.Name(), reducedTable.Name());

    TTempTable transposedTable(client);
    TransposeTable(client, reducedTable.Name(), transposedTable.Name(), day);

    TTempTable resultTable(client);
    client->Sort(
        TSortOperationSpec()
            .AddInput(transposedTable.Name())
            .Output(resultTable.Name())
            .SortBy({"Date"}));

    return resultTable;
}

void TableForPartialsGraphic(TInstant dayStart, TInstant dayFinish, const TString& path) {
    auto client = CreateClient("hahn");

    TAdaptiveThreadPool queue;
    queue.Start(0);

    TVector<NThreading::TFuture<TTempTable>> futures;

    for (TInstant day = dayStart; day <= dayFinish; day += TDuration::Days(1)) {
        futures.push_back(
            NThreading::Async([client, day] () {
                auto moscowTZ = GetTimeZone("Europe/Moscow");
                TString dayStr = Format("%Y-%m-%d", Convert(day, moscowTZ), moscowTZ);
                return Process(client, dayStr);
            }, queue));
    }

    client->CreateTable<TTransposedStatus>(path);

    TReduceOperationSpec spec;
    spec.ReduceBy({"Date"});
    for (auto& futureOperation : futures) {
        spec.AddInput<TTransposedStatus>(futureOperation.GetValueSync().Name());
    }
    spec.AddOutput<TTransposedStatus>(path);
    client->Reduce(spec, new TGlueReducer);
}

int main(int argc, const char* argv[]) {
    Initialize(argc, argv);

    TStringBuf dayStartStr;
    TStringBuf dayFinishStr;
    TString path;

    TInstant dayStart;
    TInstant dayFinish;

    NLastGetopt::TOpts opts;
    opts.AddLongOption('s', "dayStart", "dayStart : ISO8601").RequiredArgument().StoreResult(&dayStartStr);
    opts.AddLongOption('f', "dayFinish", "dayFinish : ISO8601").RequiredArgument().StoreResult(&dayFinishStr);
    opts.AddLongOption('w', "path", "where totalTable will be hold").RequiredArgument().StoreResult(&path);
    opts.AddHelpOption();
    NLastGetopt::TOptsParseResult optsResult(&opts, argc, argv);

    if (!TInstant::TryParseIso8601(dayStartStr, dayStart)) {
        ythrow yexception() << "Incorrect dayStart";
    }

    if (!TInstant::TryParseIso8601(dayFinishStr, dayFinish)) {
        ythrow yexception() << "Incorrect dayFinish";
    }
    TableForPartialsGraphic(dayStart, dayFinish, path);

    return 0;
}
