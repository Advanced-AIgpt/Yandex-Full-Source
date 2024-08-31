#include <alice/library/logs/uniproxy/uniproxy.h>

#include <mapreduce/yt/common/config.h>
#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/interface/logging/logger.h>
#include <mapreduce/yt/interface/init.h>
#include <mapreduce/yt/util/temp_table.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/http/misc/httpcodes.h>

#include <util/generic/string.h>
#include <util/stream/output.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <util/string/split.h>
#include <util/string/strip.h>
#include <util/string/join.h>
#include <util/system/user.h>

using namespace NYT;
using TFunc = std::function<void(IClientBasePtr, const TYPath&, const TYPath&)>;

//Input table contains uniproxy-logs, output table contains row with fields duration, eventId, reqId and other request data
class TComputeDurationsMapper : public IMapper<TTableReader<TNode>, TTableWriter<TNode>> {
public:
    void Do(TReader* reader, TWriter* writer) override {
        for (; reader->IsValid(); reader->Next()) {
            const auto& in = reader->GetRow();

            TNode row;
            TStringBuf messageBuf(in["message"].AsString());

            if (!NAlice::TryParseLogMessage(messageBuf, row)) {
                continue;
            }

            row["Timestamp"] = in["timestamp"];
            writer->AddRow(row);
       }
    }
};
REGISTER_MAPPER(TComputeDurationsMapper);

//In the input table two types of rows : with duration and eventId and with eventId and reqestInfo. Matches them in the output table by eventId
class TComputeDurationsReducer : public IReducer<TTableReader<TNode>, TTableWriter<TNode>> {
public:
    void Do(TReader* reader, TWriter* writer) override {
        TNode reducedRow;

        for (; reader->IsValid(); reader->Next()) {
            const auto& curRow = reader->GetRow();
            const auto rowType = curRow["type"];
            if (rowType == "ACCESS") {
                reducedRow["duration"] = curRow["duration"];
            } else if (rowType == "SESSION") {
                reducedRow["reqId"] = curRow["reqId"];
                reducedRow["text"] = curRow["text"];
                reducedRow["app_version"] = curRow["app_version"];
                reducedRow["platform"] = curRow["platform"];
                reducedRow["os_version"] = curRow["os_version"];
                reducedRow["app_id"] = curRow["app_id"];
                reducedRow["code"] = Join("\t", curRow["app_id"].AsString(), curRow["app_version"].AsString(), curRow["os_version"].AsString(), curRow["platform"].AsString(), curRow["text"].AsString());
            }
        }

        if (reducedRow.HasKey("duration") && reducedRow.HasKey("reqId")) {
            writer->AddRow(reducedRow);
        }
    }
};
REGISTER_REDUCER(TComputeDurationsReducer);

//input table contains column "duration", output table contains row with "duration" in given range
class TComputeLogsInRangeMapper  : public IMapper<TTableReader<TNode>, TTableWriter<TNode>> {
public:
    Y_SAVELOAD_JOB(DurationFrom, DurationTo, Date);

    TComputeLogsInRangeMapper() = default;

    TComputeLogsInRangeMapper(double durationFrom, double durationTo, TString date)
        : DurationFrom(durationFrom)
        , DurationTo(durationTo)
        , Date(date)
    { }

    void Do(TReader* reader, TWriter* writer) override {
        for (; reader->IsValid(); reader->Next()) {
            TNode in = reader->GetRow();
            in["date"] = Date;
            if (in["code"].Size() > 16000) {
                continue;
            }

            double duration = in["duration"].AsDouble();
            if (duration >= DurationFrom && duration <= DurationTo) {
                writer->AddRow(in);
            }
        }
    }
private:
    double DurationFrom;
    double DurationTo;
    TString Date;
};
REGISTER_MAPPER(TComputeLogsInRangeMapper);

int main(int argc, const char** argv) {
    NYT::Initialize(argc, argv);

    double quantileFrom;
    double quantileTo;
    TString date;
    TString quantilesRangeAnswer = "outputTable";
    NLastGetopt::TOpts opts;
    opts.AddLongOption("from").StoreResult(&quantileFrom).DefaultValue("0").Required();
    opts.AddLongOption("to").StoreResult(&quantileTo).DefaultValue("100").Required();
    opts.AddLongOption("date").StoreResult(&date).DefaultValue("2019-06-20").Required();
    opts.AddLongOption("output").StoreResult(&quantilesRangeAnswer).DefaultValue("output").Required();
    NLastGetopt::TOptsParseResult res(&opts, argc, argv);

    auto client = CreateClient("hahn");
    auto outputTable = NYT::TTempTable(client, "alice-logs-quantiles");
    auto outputSortedTable = NYT::TTempTable(client, "alice-logs-sorted-quantiles");
    auto outputReducedTable = NYT::TTempTable(client, "alice-logs-reduced-quantiles");
    auto outputReducedSortedTable = NYT::TTempTable(client, "alice-logs-reduced-sorted-quantiles");

    client->Map(
        TMapOperationSpec()
            .AddInput<TNode>("//home/logfeller/logs/alice-production-uniproxy/1d/" + date)
            .AddOutput<TNode>(outputTable.Name()),
        new TComputeDurationsMapper);
    client->Sort(
         TSortOperationSpec()
            .AddInput(outputTable.Name())
            .Output(outputSortedTable.Name())
            .SortBy({"eventId"}));
    client->Reduce(
        TReduceOperationSpec()
            .ReduceBy({"eventId"})
            .AddInput<TNode>(outputSortedTable.Name())
            .AddOutput<TNode>(outputReducedTable.Name()),
        new TComputeDurationsReducer);
    client->Sort(
        TSortOperationSpec()
            .AddInput(outputReducedTable.Name())
            .Output(outputReducedSortedTable.Name())
            .SortBy({"duration"}));

    ui64 size = 0;
    auto r = client->CreateTableReader<TNode>(outputReducedSortedTable.Name());
    for (; r->IsValid(); r->Next()) {
        ++size;
    }

    ui64 indexFrom = (ui64)(size * quantileFrom / 100);
    ui64 indexTo = (ui64)(size * quantileTo / 100);

    auto readerFrom = client->CreateTableReader<TNode>(TRichYPath(outputReducedSortedTable.Name()).AddRange(
        TReadRange()
            .Exact(TReadLimit().RowIndex(indexFrom))));
    auto readerTo = client->CreateTableReader<TNode>(TRichYPath(outputReducedSortedTable.Name()).AddRange(
        TReadRange()
            .Exact(TReadLimit().RowIndex(indexTo - 1))));

    double durationFrom;
    double durationTo;

    for (; readerFrom->IsValid(); readerFrom->Next()) { // выполнится один раз
        TNode row = readerFrom->GetRow();
        durationFrom = row["duration"].AsDouble();
    }
    for (; readerTo->IsValid(); readerTo->Next()) { // выполнится один раз
        TNode row = readerTo->GetRow();
        durationTo = row["duration"].AsDouble();
    }

    client->Map(
        TMapOperationSpec()
            .AddInput<TNode>(outputReducedSortedTable.Name())
            .AddOutput<TNode>(quantilesRangeAnswer),
       new TComputeLogsInRangeMapper(durationFrom, durationTo, date));

    Cerr << "Output table: https://yt.yandex-team.ru/hahn/#page=navigation&offsetMode=row&path=" << quantilesRangeAnswer << Endl;
    return 0;
}
