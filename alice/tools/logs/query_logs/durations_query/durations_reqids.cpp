#include <alice/bass/libs/client/client_info.h>
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
class TComputeRequestedDurationsMapper : public IMapper<TTableReader<TNode>, TTableWriter<TNode>> {
public:
    Y_SAVELOAD_JOB(DurationFrom, DurationTo, Date);

    TComputeRequestedDurationsMapper() = default;

    TComputeRequestedDurationsMapper(double durationFrom, double durationTo, TString date)
        : DurationFrom(durationFrom)
        , DurationTo(durationTo)
        , Date(date)
    { }

    void Do(TReader* reader, TWriter* writer) override {
        for (; reader->IsValid(); reader->Next()) {
            TNode in = reader->GetRow();
            in["date"] = Date;

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

REGISTER_MAPPER(TComputeRequestedDurationsMapper);

int main(int argc, const char** argv) {
    NYT::Initialize(argc, argv);

    double durationFrom;
    double durationTo;
    TString date;
    TString durationsRangeAnswer = "outputTable";
    NLastGetopt::TOpts opts;

    opts.AddLongOption("from").StoreResult(&durationFrom).DefaultValue("0").Required();
    opts.AddLongOption("to").StoreResult(&durationTo).DefaultValue("100").Required();
    opts.AddLongOption("date").StoreResult(&date).DefaultValue("2019-06-20").Required();
    opts.AddLongOption("output").StoreResult(&durationsRangeAnswer).DefaultValue("outputTable").Required();
    NLastGetopt::TOptsParseResult res(&opts, argc, argv);

    auto client = CreateClient("hahn");
    auto outputTable = NYT::TTempTable(client, "alice-logs-durations");
    auto outputSortedTable = NYT::TTempTable(client, "alice-sorted-logs-durations");
    auto outputReducedTable = NYT::TTempTable(client, "alice-reduced-logs-duration");
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
    client->Map(
        TMapOperationSpec()
            .AddInput<TNode>(outputReducedTable.Name())
            .AddOutput<TNode>(durationsRangeAnswer),
        new TComputeRequestedDurationsMapper(durationFrom, durationTo, date));
    Cerr << "Output table: https://yt.yandex-team.ru/hahn/#page=navigation&offsetMode=row&path=" << durationsRangeAnswer << Endl;
    return 0;
}
