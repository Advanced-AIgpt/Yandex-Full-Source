#include "mapper.h"

#include "hist.h"

#include <alice/library/client/protos/client_info.pb.h>
#include <alice/library/yt/util.h>
#include <alice/megamind/protos/analytics/analytics_info.pb.h>
#include <alice/megamind/protos/analytics/megamind_analytics_info.pb.h>
#include <alice/megamind/protos/scenarios/analytics_info.pb.h>
#include <alice/megamind/protos/speechkit/request.pb.h>
#include <alice/megamind/protos/speechkit/response.pb.h>
#include <alice/wonderlogs/protos/wonderlogs.pb.h>

#include <library/cpp/containers/stack_vector/stack_vec.h>
#include <library/cpp/json/yson/json2yson.h>
#include <library/cpp/yson/node/node_io.h>

#include <functional>
#include <regex>

using namespace NAliceYT;
using namespace NYT;
using namespace NAlice::NWonderlogs;

namespace NAlice::NSlowestScenario {

namespace {

const TIntervals TIME_INTERVALS_US = {
    0,         100,       200,       300,       400,        500,        600,
    700,       800,       900,       1'000,     2'000,      3'000,      4'000,
    5'000,     6'000,     7'000,     8'000,     9'000,      10'000,     20'000,
    30'000,    40'000,    50'000,    60'000,    70'000,     80'000,     90'000,
    100'000,   150'000,   200'000,   250'000,   300'000,    350'000,    400'000,
    450'000,   500'000,   550'000,   600'000,   650'000,    700'000,    750'000,
    800'000,   850'000,   900'000,   950'000,   1'000'000,  1'100'000,  1'200'000,
    1'300'000, 1'400'000, 1'500'000, 1'600'000, 1'700'000,  1'800'000,  1'900'000,
    2'000'000, 2'200'000, 2'400'000, 2'600'000, 2'800'000,  3'000'000,  3'250'000,
    3'500'000, 3'750'000, 4'000'000, 5'000'000, 10'000'000, 50'000'000, 100'000'000'000};

constexpr size_t REQ_ID_COUNT = 5;
constexpr size_t MAX_COMPETITORS = 10;

static const TString RUN_STAGE = "run";
static const TString VINS_SCENARIO_NAME = "Vins";

struct TCompetitorStat {
    void Add(const TString& reqId) {
        ++Count;

        if (ReqIds.size() < REQ_ID_COUNT) {
            ReqIds.push_back(reqId);
        }
    }

    void Merge(const TCompetitorStat& rhs) {
        Count += rhs.Count;

        for (size_t i = 0, e = std::min(rhs.ReqIds.size(), REQ_ID_COUNT - ReqIds.size()); i < e; ++i) {
            ReqIds.push_back(rhs.ReqIds[i]);
        }
    }

    TNode ToNode() const {
        TNode node;
        node["count"] = Count;

        auto reqIds = TNode::CreateList();
        for (const auto& value : ReqIds) {
            reqIds.Add(value);
        }
        node["req_ids"] = std::move(reqIds);
        return node;
    }

    static TCompetitorStat FromNode(const TNode& node) {
        TCompetitorStat stat;
        stat.Count = node["count"].AsUint64();
        for (const auto& value : node["req_ids"].AsList()) {
            stat.ReqIds.push_back(value.AsString());
        }
        return stat;
    }

    size_t Count = 0;
    TStackVec<TString, REQ_ID_COUNT> ReqIds;
};

struct TScenarioStat {
    TScenarioStat(const TIntervals& intervals)
        : Histogram{intervals} {
    }

    TScenarioStat(THist&& hist)
        : Histogram{std::move(hist)} {
    }

    TScenarioStat(const TScenarioStat& stat) = default;
    TScenarioStat(TScenarioStat&& stat) = default;

    void Add(const TString& reqId, const TString& competitorName, ui64 diff) {
        Histogram.Add(diff);
        Competitors[competitorName].Add(reqId);
    }

    void Merge(const TScenarioStat& rhs) {
        Histogram.Merge(rhs.Histogram);
        for (const auto& [competitor, stat] : rhs.Competitors) {
            Competitors[competitor].Merge(stat);
        }
    }

    TNode ToNode() const {
        TNode node;
        node["hist"] = Histogram.ToNode();
        node["competitors"] = CompetitorsToNode();
        return node;
    }

    static TScenarioStat FromNode(const TNode& node, const TIntervals& intervals) {
        TScenarioStat stat{THist::FromNode(node["hist"], intervals)};
        for (const auto& [competitor, stats] : node["competitors"].AsMap()) {
            stat.Competitors[competitor] = TCompetitorStat::FromNode(stats);
        }
        return stat;
    }

    TNode CompetitorsToNode() const {
        TNode competitors = TNode::CreateMap();
        for (const auto& [competitor, stat] : Competitors) {
            competitors[competitor] = stat.ToNode();
        }
        return competitors;
    }

    TNode TrimmedCompetitorsToNode(size_t competitorsCount) {
        TVector<std::pair<TString, TCompetitorStat>> competitors(Reserve(Competitors.size()));
        for (const auto& [name, stat] : Competitors) {
            if (stat.Count >= REQ_ID_COUNT) {
                competitors.push_back({name, stat});
            }
        }

        Sort(competitors, [](const auto& rhs, const auto& lhs) { return rhs.second.Count > lhs.second.Count; });
        if (competitors.size() > competitorsCount) {
            competitors.resize(competitorsCount);
        }

        TNode result = TNode::CreateList();
        for (const auto& [name, stat] : competitors) {
            result.Add()[name] = stat.ToNode();
        }
        return result;
    }

    THist Histogram;
    THashMap<TString, TCompetitorStat> Competitors;
};

bool IsMegamindProdService(TStringBuf service) {
    return service.SkipPrefix("megamind_standalone_") && IsIn({"sas", "man", "vla"}, service);
}

ui64 GetLatestResponseTimestamp(const NJson::TJsonValue& timings) {
    const auto& stageTimings = timings[RUN_STAGE];
    const ui64 startTs = FromString<ui64>(stageTimings["start_timestamp"].GetString());

    ui64 longestReq = 0;
    for (const auto& [source, duration] : stageTimings["source_response_durations"].GetMap()) {
        longestReq = Max(longestReq, FromString<ui64>(duration.GetString()));
    }
    return startTs + longestReq;
}

ui64 GetLatestResponseTimestamp(
    const google::protobuf::Map<TString, NAlice::NScenarios::TAnalyticsInfo_TScenarioStageTimings>& timings) {
    const auto& stageTimings = timings.at(RUN_STAGE);
    const ui64 startTs = stageTimings.GetStartTimestamp();

    ui64 longestReq = 0;
    for (const auto& [source, duration] : stageTimings.GetSourceResponseDurations()) {
        longestReq = Max(longestReq, duration);
    }
    return startTs + longestReq;
}

NJson::TJsonValue TryParseJson(const TNode& node) {
    NJson::TJsonValue json;
    const auto jsonString = NJson2Yson::ConvertYson2Json(NYT::NodeToYsonString(node));
    if (!NJson::ReadJsonTree(jsonString, &json)) {
        Clog << "Can not parse json: " << jsonString << Endl;
    }
    return json;
}

template <class TTimings, class TGetScenarioTimingsDelegate>
TVector<std::pair<TString, ui64>> GetLongestRunsImpl(TTimings&& timings, const bool shouldSplitVinsIntents,
                                                     const std::function<TString()>& getVinsIntent,
                                                     TGetScenarioTimingsDelegate&& getScenarioTimings) {
    TVector<std::pair<TString, ui64>> longestRuns{};
    for (const auto& [scenarioName, scenarioTimings] : timings) {
        TString tableScenarioName = scenarioName;
        if (shouldSplitVinsIntents && scenarioName == VINS_SCENARIO_NAME) {
            // Works only if Vins is a winner.
            const auto& intent = getVinsIntent();
            if (!intent.empty()) {
                tableScenarioName = "Vins__" + intent;
            }
        }
        longestRuns.push_back({tableScenarioName, GetLatestResponseTimestamp(getScenarioTimings(scenarioTimings))});
    }
    return longestRuns;
}

template <class IRowType>
class TStatsMapperBase : public IMapper<TTableReader<IRowType>, TTableWriter<TNode>> {
public:
    TStatsMapperBase() = default;
    TStatsMapperBase(const TString& clientRegexp, const bool shouldSplitVinsIntents)
        : ClientRegexp{clientRegexp}
        , ShouldSplitVinsIntents{shouldSplitVinsIntents} {
    }

    Y_SAVELOAD_JOB(ClientRegexp, ShouldSplitVinsIntents);

    void Do(TTableReader<IRowType>* reader, TTableWriter<TNode>* writer) override {
        THashMap<TString, TScenarioStat> stats;
        TScenarioStat emptyStat{TIME_INTERVALS_US};
        TMaybe<std::regex> clientRegexp;
        if (!ClientRegexp.empty()) {
            clientRegexp.ConstructInPlace(ClientRegexp.c_str());
        }
        for (const auto& cursor : *reader) {
            const auto& row = cursor.GetRow();
            const auto requestId = GetRequestId(row);
            if (!IsRealReqId(requestId)) {
                continue;
            }
            if (clientRegexp) {
                const auto appId = GetAppId(row);
                if (!appId.empty()) {
                    if (!std::regex_match(appId.c_str(), *clientRegexp)) {
                        continue;
                    }
                }
            }
            auto longestRuns = GetLongestRuns(row, ShouldSplitVinsIntents);
            if (longestRuns.size() < 2) {
                continue;
            }
            std::nth_element(longestRuns.begin(), longestRuns.begin() + 1, longestRuns.end(),
                             [](const auto& time1, const auto& time2) { return time1.second > time2.second; });
            const ui64 diff = longestRuns[0].second - longestRuns[1].second;
            if (diff == 0) {
                continue;
            }
            Y_ASSERT(diff > 0);

            auto& stat = stats.try_emplace(longestRuns[0].first, emptyStat).first->second;
            stat.Add(requestId, longestRuns[1].first, diff);
        }
        for (const auto& [scenarioName, stat] : stats) {
            TNode node = stat.ToNode();
            node["scenario"] = scenarioName;
            writer->AddRow(std::move(node));
        }
    }

protected:
    virtual TVector<std::pair<TString, ui64>> GetLongestRuns(const IRowType& row,
                                                             bool shouldSplitVinsIntents) const = 0;
    virtual TString GetRequestId(const IRowType& row) const = 0;
    virtual TString GetAppId(const IRowType& row) const = 0;

private:
    TString ClientRegexp;
    bool ShouldSplitVinsIntents;
};

class TStatsMapper : public TStatsMapperBase<TNode> {
public:
    TStatsMapper() = default;
    TStatsMapper(const TString& clientRegexp, bool shouldSplitVinsIntents)
        : TStatsMapperBase<TNode>(clientRegexp, shouldSplitVinsIntents) {
    }

protected:
    TVector<std::pair<TString, ui64>> GetLongestRuns(const TNode& row,
                                                     const bool shouldSplitVinsIntents) const override {
        if (row["content_type"].AsString() != "application/json") {
            return {};
        }
        if (!IsMegamindProdService(row["environment"].AsString())) {
            return {};
        }

        const NJson::TJsonValue aInfoJson = TryParseJson(row["analytics_info"]);
        if (aInfoJson.IsNull()) {
            return {};
        }

        const auto& timings = aInfoJson["scenario_timings"];
        return GetLongestRunsImpl(
            timings.GetMap(), shouldSplitVinsIntents,
            [&aInfoJson] {
                return aInfoJson["analytics_info"]["Vins"]["scenario_analytics_info"]["product_scenario_name"]
                    .GetString();
            },
            [](const auto& scenarioTimings) { return scenarioTimings["timings"]; });
    }

    TString GetRequestId(const TNode& row) const override {
        return row["request_id"].AsString();
    }

    TString GetAppId(const TNode& row) const override {
        if (auto reqJson = TryParseJson(row["request"]); !reqJson.IsNull()) {
            return reqJson["application"]["app_id"].GetString();
        }
        return {};
    }
};

REGISTER_MAPPER(TStatsMapper);

class TStatsMapperOverWonderLogs : public TStatsMapperBase<TWonderlog> {
public:
    TStatsMapperOverWonderLogs() = default;
    TStatsMapperOverWonderLogs(const TString& clientRegexp, bool shouldSplitVinsIntents)
        : TStatsMapperBase<TWonderlog>(clientRegexp, shouldSplitVinsIntents) {
    }

protected:
    TVector<std::pair<TString, ui64>> GetLongestRuns(const TWonderlog& row,
                                                     const bool shouldSplitVinsIntents) const override {
        const auto& timings = row.GetSpeechkitResponse().GetMegamindAnalyticsInfo().GetScenarioTimings();
        return GetLongestRunsImpl(
            timings, shouldSplitVinsIntents,
            [&row, requestId = GetRequestId(row)] {
                const auto& analyticsInfo = row.GetSpeechkitResponse().GetMegamindAnalyticsInfo().GetAnalyticsInfo();
                if (!analyticsInfo.contains(VINS_SCENARIO_NAME)) {
                    Clog << "unable to find vins analytics info: " << requestId << Endl;
                    return TString{};
                }
                return analyticsInfo.at(VINS_SCENARIO_NAME).GetScenarioAnalyticsInfo().GetProductScenarioName();
            },
            [](const auto& scenarioTimings) { return scenarioTimings.GetTimings(); });
    }
    TString GetRequestId(const TWonderlog& row) const override {
        return row.GetSpeechkitRequest().GetHeader().GetRequestId();
    }
    TString GetAppId(const TWonderlog& row) const override {
        return row.GetSpeechkitRequest().GetApplication().GetAppId();
    }
};

REGISTER_MAPPER(TStatsMapperOverWonderLogs);

class TStatsReducer : public IReducer<TTableReader<TNode>, TTableWriter<TNode>> {
public:
    void Do(TReader* reader, TWriter* writer) override {
        TScenarioStat stat{TIME_INTERVALS_US};
        TString scenario;
        for (const auto& cursor : *reader) {
            const auto& row = cursor.GetRow();
            const TString& rowScenario = row["scenario"].AsString();
            if (scenario.empty()) {
                scenario = rowScenario;
            }
            Y_ENSURE(scenario == rowScenario);
            stat.Merge(TScenarioStat::FromNode(row, TIME_INTERVALS_US));
        }

        TNode result;
        result["scenario"] = scenario;
        result["count"] = stat.Histogram.Count;
        result["neg_count"] = -static_cast<i64>(stat.Histogram.Count);
        result["competitors"] = stat.TrimmedCompetitorsToNode(MAX_COMPETITORS);
        for (auto perc : {25, 50, 75, 90, 95, 99}) {
            auto percentile = stat.Histogram.ComputePercentile(perc / 100.);
            result["p" + ToString(perc)] = percentile;
        }
        writer->AddRow(std::move(result));
    }
};

REGISTER_REDUCER(TStatsReducer);

template <class TInputType, class TMapper>
void ComputeTimingsImpl(const TString& clientRegexp, const bool shouldSplitVinsIntents, IClientPtr client,
                        const TVector<TYPath>& from, const TYPath& to) {
    auto spec = DefaultMapReduceOperationSpec(/* memoryLimitMB= */ 2 * 1024).MapJobCount(2000);
    for (const TYPath& path : from) {
        spec.AddInput<TInputType>(path);
    }
    spec.AddOutput<TNode>(to).ReduceBy("scenario");
    client->MapReduce(spec, MakeIntrusive<TMapper>(clientRegexp, shouldSplitVinsIntents),
                      MakeIntrusive<TStatsReducer>(), DefaultOperationOptions());
    client->Sort(NYT::TSortOperationSpec().AddInput(to).Output(to).SortBy({"neg_count"}));
}

} // namespace

void ComputeTimings(const TString& clientRegexp, const bool shouldSplitVinsIntents, IClientPtr client,
                    const TVector<TYPath>& from, const TYPath& to) {
    ComputeTimingsImpl<TNode, TStatsMapper>(clientRegexp, shouldSplitVinsIntents, client, from, to);
}

void ComputeTimingsOverWonderLogs(const TString& clientRegexp, const bool shouldSplitVinsIntents,
                                  NYT::IClientPtr client, const TVector<NYT::TYPath>& from, const NYT::TYPath& to) {
    ComputeTimingsImpl<TWonderlog, TStatsMapperOverWonderLogs>(clientRegexp, shouldSplitVinsIntents, client, from, to);
}

} // namespace NAlice::NSlowestScenario
