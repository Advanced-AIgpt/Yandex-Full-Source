#include <alice/bass/libs/logging/logger.h>

#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/util/temp_table.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/threading/future/async.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/str.h>
#include <util/thread/pool.h>

#include <future>

using namespace NYT;

class TOutputReducer : public IReducer<TTableReader<TNode>, TTableWriter<TNode>> {
public:
    void Do(TReader* reader, TWriter* writer) override {
        TString oldDurations;
        TString newDurations;
        TString code;
        for (; reader->IsValid(); reader->Next()) {
            const auto& row = reader->GetRow();
            if (reader->GetTableIndex() == 0) {
                code = row["code"].AsString();
                oldDurations = row["durations"].AsString();
            } else {
                newDurations = row["durations"].AsString();
            }
        }
        if (oldDurations && newDurations) {
            TNode newRow;
            newRow["code"] = code;
            newRow["first_day_durations"] = oldDurations;
            newRow["second_day_durations"] = newDurations;
            SetQuantilesDelta(newRow);
            writer->AddRow(newRow);
        }
    }

private:
    TVector<double> GetSortedDurations(const TString& data) {
        TVector<double> values;
        TStringBuf buf(data);
        while (buf) {
            values.push_back(FromString<double>(buf.NextTok(' ')));
        }
        Sort(values.begin(), values.end());
        return values;
    }

    double GetQuantile(const TVector<double>& values, double quantile) {
        Y_ENSURE(!values.empty());
        Y_ENSURE(quantile <= 1.0);
        size_t n = (size_t) ((values.size() - 1) * quantile);
        if (n >= values.size()) {
            n = values.size() - 1;
        }
        return values[n];
    }

    void SetQuantilesDelta(TNode& row) {
        TVector<double> oldValues = GetSortedDurations(row["first_day_durations"].AsString());
        TVector<double> newValues = GetSortedDurations(row["second_day_durations"].AsString());
        TVector<double> quantiles = {0.5, 0.75, 1.0};
        for (auto quantile : quantiles) {
            row["delta " + ToString(quantile) + "q"] = GetQuantile(newValues, quantile) - GetQuantile(oldValues, quantile);
        }
    }
};
REGISTER_REDUCER(TOutputReducer);

class TOutputMapper : public IMapper<TTableReader<TNode>, TTableWriter<TNode>> {
public:
    void Do(TReader* reader, TWriter* writer) override {
        for (; reader->IsValid(); reader->Next()) {
            const auto& row = reader->GetRow();
            TNode newRow;
            newRow["code"] = row["code"].AsString();
            newRow["duration"] = row["duration"].AsDouble();
            newRow["request_id"] = row["reqId"].AsString();
            newRow["date"] = row["date"].AsString();
            writer->AddRow(newRow);
        }
    }
};
REGISTER_MAPPER(TOutputMapper);

class TUnionReducer : public IReducer<TTableReader<TNode>, TTableWriter<TNode>> {
public:
    void Do(TReader* reader, TWriter* writer) override {
        TVector<double> durations;
        TString durationsStr;
        TString code;
        for (; reader->IsValid(); reader->Next()) {
            const auto& row = reader->GetRow();
            if (durations.size() == 0) {
                code = row["code"].AsString();
            }
            durations.push_back(row["duration"].AsDouble());
        }
        Sort(durations.begin(), durations.end());
        for (auto duration : durations) {
            if (durationsStr) {
                durationsStr += " ";
            }
            durationsStr += ToString(duration);
        }
        TNode newRow;
        newRow["durations"] = durationsStr;
        newRow["code"] = code;
        writer->AddRow(newRow);
    }
};
REGISTER_REDUCER(TUnionReducer);


void UnionByDuration(const TString& inputTable, const TString& outputTable, TIntrusivePtr<IClient> client) {
    client->Reduce(
        TReduceOperationSpec()
            .ReduceBy({"code"})
            .AddInput<TNode>(inputTable)
            .AddOutput<TNode>(outputTable),
        new TUnionReducer);
}

void SortByCode(const TString& inputTable, const TString& outputTable, TIntrusivePtr<IClient> client) {
    client->Sort(
        TSortOperationSpec()
            .AddInput(inputTable)
            .Output(outputTable)
            .SortBy({"code"}));
}

void Process(const TString& inputTable, const TString& outputTable, TIntrusivePtr<IClient> client) {
    TTempTable sortedTable(client);
    SortByCode(inputTable, sortedTable.Name(), client);
    LOG(INFO) << "Sort->Reduce->Sort stage 1 finished (Sort)" << Endl;
    TTempTable unionTable(client);
    UnionByDuration(sortedTable.Name(), unionTable.Name(), client);
    LOG(INFO) << "Sort->Reduce->Sort stage 2 finished (Reduce)" << Endl;
    SortByCode(unionTable.Name(), outputTable, client);
    LOG(INFO) << "Sort->Reduce->Sort stage 3 finished (Sort)" << Endl;
}

void OutputMap(const TString& table1, const TString& table2, const TString& outputTable, TIntrusivePtr<IClient> client) {
    LOG(INFO) << "Launching Map" << Endl;
    client->Map(
        TMapOperationSpec()
            .AddInput<TNode>(table1)
            .AddInput<TNode>(table2)
            .AddOutput<TNode>(outputTable),
        new TOutputMapper);
    LOG(INFO) << "Map finished" << Endl;
}

void PrintOutputTable(const TString& name) {
    LOG(INFO) << "Output table: https://yt.yandex-team.ru/hahn/#page=navigation&offsetMode=row&path=" << name << Endl;
}

int main(int argc, const char* argv[]) {
    Initialize(argc, argv);
    TString table1;
    TString table2;
    TString folder;
    TString cluster;
    NLastGetopt::TOpts options;
    options.AddLongOption('a', "day1", "first day table").RequiredArgument().StoreResult(&table1);
    options.AddLongOption('b', "day2", "second day table").RequiredArgument().StoreResult(&table2);
    options.AddLongOption('o', "output_folder", "path to output folder").RequiredArgument().StoreResult(&folder);
    options.AddLongOption('s', "cluster", "cluster name").OptionalArgument().DefaultValue("hahn").StoreResult(&cluster);
    options.AddHelpOption();
    NLastGetopt::TOptsParseResult optsResult(&options, argc, argv);
    auto client = CreateClient(cluster);


    TTempTable sortedTable1(client);
    TTempTable sortedTable2(client);
    TString sorted1 = sortedTable1.Name();
    TString sorted2 = sortedTable2.Name();
    TString output1 = folder + "/durations-quntile_deltas";
    TString output2 = folder + "/date-duration-req_id";

    LOG(INFO) << "Launching 2 parallel Sort->Reduce->Sort processes" << Endl;
    TAdaptiveThreadPool queue;
    queue.Start(0);
    NThreading::TFuture<void> future1 = NThreading::Async([&table1, &sorted1, client] () {
        Process(table1, sorted1, client);
    }, queue);

    NThreading::TFuture<void> future2 = NThreading::Async([&] () {
        Process(table2, sorted2, client);
    }, queue);
    NThreading::TFuture<void> futureForSecondTable = NThreading::Async([&] () {
        OutputMap(table1, table2, output2, client);
    }, queue);

    future1.GetValueSync();
    future2.GetValueSync();
    LOG(INFO) << "2 parallel Sort->Reduce->Sort processes finished. Two tables are created (table1, table2)" << Endl;

    LOG(INFO) << "Launching Reduce of two tables (table1, table2) for " << output1 << Endl;
    client->Reduce(
        TReduceOperationSpec()
            .ReduceBy({"code"})
            .AddInput<TNode>(sorted1)
            .AddInput<TNode>(sorted2)
            .AddOutput<TNode>(output1),
        new TOutputReducer);
    PrintOutputTable(output1);
    futureForSecondTable.GetValueSync();
    PrintOutputTable(output2);
    return 0;
}
