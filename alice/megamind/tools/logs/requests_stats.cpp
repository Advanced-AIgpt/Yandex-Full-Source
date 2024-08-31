#include "requests_stats.h"

#include <alice/megamind/tools/logs/protos/logs.pb.h>
#include <alice/megamind/tools/logs/util.h>

#include <alice/library/yt/protos/logs.pb.h>
#include <alice/library/yt/util.h>

#include <contrib/libs/re2/re2/re2.h>

#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/util/temp_table.h>

#include <library/cpp/compute_graph/compute_graph.h>
#include <library/cpp/json/json_writer.h>

using namespace NYT;

namespace NMegamindLog {

namespace {

class TMegamindLogRequestsMapper : public IMapper<TTableReader<TMegamindLogRow>, TTableWriter<TRequestsFetchInfoRow>> {
public:
    void Do(TReader* reader, TWriter* writer) override {
        for (; reader->IsValid(); reader->Next()) {
            const auto& row = reader->GetRow();

            if (IsRealReqId(row.GetReqId())) {
                if (auto fetchInfo = TryParseFetchBegin(row.GetMessage())) {
                    WriteRow(writer, std::move(*fetchInfo), row);
                } else if (auto fetchInfo = TryParseFetchEnd(row.GetMessage())) {
                    WriteRow(writer, std::move(*fetchInfo), row);
                }
            }
        }
    }

private:
    void WriteRow(TWriter* writer, TRequestsFetchInfoRow fetchInfoRow, const TMegamindLogRow& logRow) {
        fetchInfoRow.SetReqId(logRow.GetReqId());
        fetchInfoRow.SetTimestamp(logRow.GetTimestamp());
        writer->AddRow(fetchInfoRow);
    }

    TMaybe<TRequestsFetchInfoRow> TryParseFetchBegin(const TString& message) {
        ui64 fetchId;
        TString address;
        // Take address without CGI
        if (re2::RE2::FullMatch(message, "Request .* (\\d+) \\(fetcher \\d+\\) is about to fetch: \"([^?]*).*\"", &fetchId, &address)) {
            TRequestsFetchInfoRow row;
            row.SetFetchId(fetchId);
            row.SetAddress(address);
            return row;
        }
        return Nothing();
    }

    TMaybe<TRequestsFetchInfoRow> TryParseFetchEnd(const TString& message) {
        ui64 fetchId;
        double duration;
        if (re2::RE2::FullMatch(message, "Request .* (\\d+) attempt \\d+ has won, time taken: (.*)s", &fetchId, &duration)) {
            TRequestsFetchInfoRow row;
            row.SetFetchId(fetchId);
            row.SetDuration(duration);
            return row;
        }
        return Nothing();
    }
};
REGISTER_MAPPER(TMegamindLogRequestsMapper);

class TRequestsReducer : public IReducer<TTableReader<TRequestsFetchInfoRow>, TTableWriter<TRequestsInfoRow>> {
private:
    using TTime = std::pair<ui64, double>;  // <Timestamp, Duration>
    using TTimes = TVector<TTime>;

public:
    void Do(TReader* reader, TWriter* writer) override {
        // every FetchId has one Address and some Times
        THashMap<ui64, TString> fetchIdAddress;
        THashMap<ui64, TTimes> fetchIdTimes;

        TString reqId;
        for (; reader->IsValid(); reader->Next()) {
            const auto& row = reader->GetRow();

            if (reqId.Empty()) {
                reqId = row.GetReqId();
            }

            ui64 fetchId = row.GetFetchId();
            if (row.HasAddress()) {
                fetchIdAddress[fetchId] = row.GetAddress();
            } else if (row.HasDuration()) {
                fetchIdTimes[fetchId].emplace_back(row.GetTimestamp(), row.GetDuration());
            }
        }

        THashMap<TString, TTimes> addressTimes = ConstructAddressTimes(fetchIdAddress, fetchIdTimes);

        TRequestsInfoRow row;
        row.SetReqId(std::move(reqId));
        if (auto info = ConstructInfo(std::move(addressTimes))) {
            row.SetInfo(std::move(*info));
        }
        writer->AddRow(row);
    }

private:
    THashMap<TString, TTimes> ConstructAddressTimes(const THashMap<ui64, TString>& fetchIdAddress, const THashMap<ui64, TTimes>& fetchIdTimes) {
        THashMap<TString, TTimes> addressTimes;
        for (const auto& p : fetchIdAddress) {
            const auto fetchId = p.first;
            const auto& address = p.second;

            auto iter = fetchIdTimes.find(fetchId);
            if (iter != fetchIdTimes.end()) {
                auto& times = addressTimes[address];
                Copy(iter->second.begin(), iter->second.end(), std::back_inserter(times));
            }
        }
        return addressTimes;
    }

    TMaybe<TString> ConstructInfo(THashMap<TString, TTimes> addressTimes) {
        if (addressTimes.empty()) {
            return Nothing();
        }

        NJson::TJsonValue infoJson;
        for (auto& p : addressTimes) {
            const auto& address = p.first;
            auto& times = p.second;

            Sort(times.begin(), times.end());
            NJson::TJsonValue timesJson;
            for (const auto& time : times) {
                timesJson.AppendValue(time.second);
            }

            NJson::TJsonValue requestsInfoJson;
            requestsInfoJson["times"] = timesJson;

            infoJson[address] = requestsInfoJson;
        }

        TStringStream ss;
        NJson::TJsonWriter(&ss, /* formatOutput = */ false, /* sortKeys = */ true).Write(infoJson);

        return ss.Str();
    }
};
REGISTER_REDUCER(TRequestsReducer);

} // namespace

void TRequestsStatsTablePreparer::Prepare(NYT::IClientPtr client, const NYT::TYPath& megamindLog, const NYT::TYPath& to) const {
    TTempTable table(client);

    {
        TMapOperationSpec spec;
        spec.AddInput<TMegamindLogRow>(megamindLog)
            .AddOutput<TRequestsFetchInfoRow>(table.Name());
        client->Map(spec, new TMegamindLogRequestsMapper);
    }
    {
        TSortOperationSpec spec;
        spec.SortBy({"ReqId"})
            .AddInput(table.Name())
            .Output(table.Name());
        client->Sort(spec);
    }

    NAliceYT::CreateTableWithSchema<TRequestsInfoRow>(client, to, /* force = */ true);
    {
        TReduceOperationSpec spec;
        spec.ReduceBy({"ReqId"})
            .AddInput<TRequestsFetchInfoRow>(table.Name())
            .AddOutput<TRequestsInfoRow>(to);
        client->Reduce(spec, new TRequestsReducer);
    }
    {
        TSortOperationSpec spec;
        spec.SortBy({"ReqId"})
            .AddInput(to)
            .Output(to);
        client->Sort(spec);
    }
}

} //namespace NMegamindLog

