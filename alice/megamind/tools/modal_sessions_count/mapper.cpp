#include "mapper.h"

#include "config_helpers.h"

#include <alice/library/yt/protos/logs.pb.h>
#include <alice/library/yt/util.h>

#include <alice/megamind/library/classifiers/util/modes.h>
#include <alice/megamind/library/session/session.h>
#include <alice/megamind/tools/modal_sessions_count/protos/modal_session_stat.pb.h>

#include <library/cpp/iterator/enumerate.h>

#include <util/generic/is_in.h>

using namespace NAliceYT;
using namespace NYT;

namespace NAlice::NModalSessionMapper {

namespace {

struct TSessionStat {
    ui64 TotalReqCnt = 0;
    ui64 ModalReqCnt = 0;

    TModalSessionsStats ToStatRow(const TString& tableName) const {
        TModalSessionsStats outStats;
        outStats.SetTable(tableName);
        outStats.SetTotalRequests(TotalReqCnt);
        outStats.SetModalRequests(ModalReqCnt);
        outStats.SetModalPercentage(TotalReqCnt ? static_cast<double>(ModalReqCnt) / TotalReqCnt * 100 : 0);
        return outStats;
    }
};

bool IsMegamindProdService(TStringBuf service) {
    return IsIn({TStringBuf("megamind_sas"), TStringBuf("megamind_man"), TStringBuf("megamind_vla")}, service);
}

class TStatsMapper : public IMapper<TTableReader<TMegamindLogRow>, TTableWriter<TModalSessionsStats>> {
public:
    TStatsMapper() = default;

    TStatsMapper(TTableMapping tableMapping, TScenarioMaxTurns config)
        : TableMapping{std::move(tableMapping)}
        , Config{std::move(config)}
    {
    }

    Y_SAVELOAD_JOB(TableMapping, Config);

    void Do(TReader* reader, TWriter* writer) override {
        TVector<TSessionStat> stats(TableMapping.size());

        for (const auto& cursor : *reader) {
            const auto& row = cursor.GetRow();
            auto& stat = stats[cursor.GetTableIndex()];

            if (!IsMegamindProdService(row.GetNannyService())) {
                continue;
            }
            TStringBuf messageBuf{row.GetMessage()};
            NJson::TJsonValue message;
            if (!ReadJsonTree(messageBuf, &message, /* throwOnError= */ false)
                    || !message.Has("request") || !message.Has("header")
                    || !IsRealReqId(message["header"]["request_id"].GetString()))
            {
                continue;
            }

            ++stat.TotalReqCnt;

            const NJson::TJsonValue& sessionJson = message["session"];
            if (!sessionJson.IsString()) {
                continue;
            }
            auto session = NAlice::DeserializeSession(sessionJson.GetString());
            if (IsActiveModalScenario(session.Get())) {
                ++stat.ModalReqCnt;
            }
        }

        for (const auto& [i, stat] : Enumerate(stats)) {
            if (stat.TotalReqCnt) {
                writer->AddRow(stat.ToStatRow(TableMapping[i]));
            }
        }
    }

private:
    i32 GetMaxActivityTurns(const TString& scenario) {
        const i32* found = Config.FindPtr(scenario);
        return found ? *found : 0;
    }

    bool IsActiveModalScenario(const ISession* session) {
        if (!session) {
            return false;
        }
        const TString& prevScenario = session->GetPreviousScenarioName();
        return IsInModalMode(session, GetMaxActivityTurns(prevScenario), prevScenario,
                             /* activeScenarioTimeoutMs= */ 0, /* serverTimeMs= */ TInstant::Now().MilliSeconds());
    }

private:
    TTableMapping TableMapping;
    TScenarioMaxTurns Config;
};

REGISTER_MAPPER(TStatsMapper);

class TStatsReducer : public IReducer<TTableReader<TModalSessionsStats>, TTableWriter<TModalSessionsStats>> {
public:
    void Do(TReader* reader, TWriter* writer) override {
        TSessionStat tableStat;
        TString tableName;
        for (const auto& cursor : *reader) {
            const auto& row = cursor.GetRow();
            const TString& rowTableName = row.GetTable();
            if (tableName.empty()) {
                tableName = rowTableName;
            }
            Y_ENSURE(tableName == rowTableName);
            tableStat.TotalReqCnt += row.GetTotalRequests();
            tableStat.ModalReqCnt += row.GetModalRequests();
        }
        writer->AddRow(tableStat.ToStatRow(tableName));
    }
};

REGISTER_REDUCER(TStatsReducer);

} // namespace

void ComputeModalStats(TScenarioMaxTurns config, IClientPtr client, TVector<TYPath> from, const TYPath& to) {
    auto spec = DefaultMapReduceOperationSpec(/* memoryLimitMB= */ 2 * 1024);
    for (const TYPath& path : from) {
        spec.AddInput<TMegamindLogRow>(path);
    }
    spec.AddOutput<TModalSessionsStats>(to).ReduceBy("TableName");
    client->MapReduce(spec, MakeIntrusive<TStatsMapper>(std::move(from), std::move(config)),
                      MakeIntrusive<TStatsReducer>(), DefaultOperationOptions());
}

} // namespace NAlice::NModalSessionMapper
