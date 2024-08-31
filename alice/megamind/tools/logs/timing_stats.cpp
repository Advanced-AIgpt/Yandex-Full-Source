#include "timing_stats.h"
#include "util.h"
#include "util/generic/algorithm.h"
#include "util/generic/vector.h"

#include <alice/megamind/tools/logs/protos/logs.pb.h>
#include <alice/library/yt/protos/logs.pb.h>

#include <alice/library/yt/util.h>

#include <cstddef>
#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/util/temp_table.h>

#include <library/cpp/compute_graph/compute_graph.h>
#include <library/cpp/json/json_writer.h>

using namespace NAliceYT;
using namespace NYT;

namespace NMegamindLog {

namespace {

static constexpr TStringBuf VINS_NAMESPACE = "Vins";
static constexpr TStringBuf CLIENT_VINS_NAMESPACE = "ClientVins";
static constexpr TStringBuf UNIPROXY_VINS_TIMINGS_NAME = "UniproxyVinsTimings";
static constexpr TStringBuf UNIPROXY_TTS_TIMINGS_NAME = "UniproxyTTSTimings";

bool IsTimingName(TStringBuf name) {
    return name == UNIPROXY_VINS_TIMINGS_NAME || name == UNIPROXY_TTS_TIMINGS_NAME;
}

bool IsTimingDirective(TStringBuf namespace_, TStringBuf name) {
    return namespace_ == "System" && IsTimingName(name);
}

bool IsTimingRow(TStringBuf namespace_, TStringBuf name) {
    return namespace_ == VINS_NAMESPACE && IsTimingName(name);
}

class TAnalyticsToIntentMapper : public IMapper<TTableReader<TNode>, TTableWriter<TTimingInfoLogRow>> {
public:
    void Do(TReader* reader, TWriter* writer) override {
        for (; reader->IsValid(); reader->Next()) {
            const auto& in = reader->GetRow();

            const TString& reqId = in["request_id"].AsString();
            if (!IsRealReqId(reqId)) {
                continue;
            }

            const auto& info = in["analytics_info"];
            TMaybe<TString> scenario, productScenarioName, intent;
            ParseIntent(info, scenario, productScenarioName, intent);

            if (scenario.Defined()) {
                TTimingInfoLogRow row;
                row.SetReqId(reqId);
                row.SetScenario(*scenario);
                row.SetUniproxyEpoch(in["server_time_us"].AsUint64());

                if (productScenarioName.Defined()) {
                    row.SetProductScenarioName(*productScenarioName);
                }

                if (intent.Defined()) {
                    row.SetIntent(*intent);
                }

                writer->AddRow(row);
            }
        }
    }

private:
    void ParseIntent(const TNode& info, TMaybe<TString>& scenario, TMaybe<TString>& productScenarioName, TMaybe<TString>& intent) {
        if (!info.IsMap() || !info.HasKey("analytics_info")) {
            return;
        }
        for (const auto& [key, value] : info["analytics_info"].AsMap()) {
            scenario.ConstructInPlace(key);
            if (value.HasKey("scenario_analytics_info")) {
                const auto& scenarioInfo = value["scenario_analytics_info"];
                if (scenarioInfo.HasKey("intent")) {
                    intent.ConstructInPlace(scenarioInfo["intent"].AsString());
                }
                if (scenarioInfo.HasKey("product_scenario_name")) {
                    productScenarioName.ConstructInPlace(scenarioInfo["product_scenario_name"].AsString());
                }
            }
        }
    }
};
REGISTER_MAPPER(TAnalyticsToIntentMapper);

class TMegamindIntentReducer : public IReducer<TTableReader<TTimingInfoLogRow>, TTableWriter<TTimingInfoLogRow>> {
public:
    void Do(TReader* reader, TWriter* writer) override {
        TMaybe<TTimingInfoLogRow> intentRow;
        TMaybe<TTimingInfoLogRow> logRow;

        for (; reader->IsValid(); reader->Next()) {
            auto tableIndex = reader->GetTableIndex();
            auto row = reader->GetRow();

            if (tableIndex == 0) {
                if (intentRow.Empty() || intentRow->GetUniproxyEpoch() < row.GetUniproxyEpoch()) {
                    intentRow = std::move(row);
                }
            } else if (tableIndex == 1) {
                logRow = std::move(row);
            }
        }

        if (logRow.Defined()) {
            if (intentRow.Defined()) {
                logRow->MergeFrom(*intentRow);
            }
            writer->AddRow(std::move(*logRow));
        }
    }
};
REGISTER_REDUCER(TMegamindIntentReducer);

class TUniproxyVinsTimingsMapper : public IMapper<TTableReader<TTimingInfoLogRow>, TTableWriter<TNode>> {
public:
    void Do(TReader* reader, TWriter* writer) override {
        for (; reader->IsValid(); reader->Next()) {
            const auto& in = reader->GetRow();

            if (in.GetName() != UNIPROXY_VINS_TIMINGS_NAME) {
                continue;
            }
            if (!in.HasReqId()) {
                continue;
            }

            TStringBuf messageBuf(in.GetAdditionalInfo());
            NJson::TJsonValue timings;
            if (!ReadJsonTree(messageBuf, &timings, false)) {
                continue;
            }

            TNode row = TNode::CreateMap();
            row["req_id"] = in.GetReqId();
            for (const auto& [key, value] : timings.GetMap()) {
                row[key] = CastToTNode(value);
            }

            TStringBuf fielddate(in.GetUniproxyTimestamp());
            row["fielddate"] = fielddate.NextTok('T');

            writer->AddRow(row);
        }
    }

private:
    TNode CastToTNode(const NJson::TJsonValue& value) {
        switch (value.GetType()) {
            case NJson::JSON_INTEGER:
                return {value.GetInteger()};
            case NJson::JSON_BOOLEAN:
                return {static_cast<int>(value.GetBoolean())};
            case NJson::JSON_DOUBLE:
                return {value.GetDouble()};
            default:
                return {};
        }
    }
};
REGISTER_MAPPER(TUniproxyVinsTimingsMapper);

class TMegamindLogMapper : public IMapper<TTableReader<TNode>, TTableWriter<TTimingInfoLogRow>> {
public:
    void Do(TReader* reader, TWriter* writer) override {
        for (; reader->IsValid(); reader->Next()) {
            const auto& in = reader->GetRow();

            TTimingInfoLogRow row;

            row.SetReqId(in["request_id"].AsString());

            // Fill megamind request info
            const auto& req = in["request"];
            if (!req.IsMap()) {
                continue;
            }
            const auto& app = req["application"];
            if (!app.IsMap()) {
                continue;
            }
            if (app.HasKey("device_model") && app["device_model"].IsString()) {
                row.SetDevice(app["device_model"].AsString());
            } else {
                row.SetDevice("");
            }
            if (app.HasKey("app_id") && app["app_id"].IsString()) {
                row.SetAppId(app["app_id"].AsString());
            } else {
                row.SetAppId("");
            }
            if (app.HasKey("app_version") && app["app_version"].IsString()) {
                row.SetAppVersion(app["app_version"].AsString());
            } else {
                row.SetAppVersion("");
            }
            if (app.HasKey("os_version") && app["os_version"].IsString()) {
                row.SetOsVersion(app["os_version"].AsString());
            } else {
                row.SetOsVersion("");
            }
            if (app.HasKey("platform") && app["platform"].IsString()) {
                row.SetPlatform(app["platform"].AsString());
            } else {
                row.SetPlatform("");
            }
            if (app.HasKey("uuid") && app["uuid"].IsString()) {
                row.SetUuid(app["uuid"].AsString());
            } else {
                row.SetUuid("");
            }
            if (app.HasKey("lang") && app["lang"].IsString()) {
                row.SetLang(app["lang"].AsString());
            } else {
                row.SetLang("");
            }
            if (app.HasKey("client_time") && app["client_time"].IsString()) {
                row.SetClientTime(app["client_time"].AsString());
            } else {
                row.SetClientTime("");
            }
            if (app.HasKey("timezone") && app["timezone"].IsString()) {
                row.SetTimezone(app["timezone"].AsString());
            } else {
                row.SetTimezone("");
            }
            if (app.HasKey("timestamp") && app["timestamp"].IsString()) {
                row.SetTimestamp(app["timestamp"].AsString());
            } else {
                row.SetTimestamp("");
            }
            writer->AddRow(row);
        }
    }
};
REGISTER_MAPPER(TMegamindLogMapper);

class TUniproxyLogMapper : public IMapper<TTableReader<TUniproxyLogRow>, TTableWriter<TTimingInfoLogRow>> {
public:
    void Do(TReader* reader, TWriter* writer) override {
        for (; reader->IsValid(); reader->Next()) {
            const auto& in = reader->GetRow();

            if (!IsAliceRow(in)) {
                continue;
            }

            TStringBuf messageBuf(in.GetMessage());
            TTimingInfoLogRow row;

            if (!TryParseLogMessage(messageBuf, row)) {
                continue;
            }

            row.SetUniproxyTimestamp(in.GetTimestamp());
            row.SetUniproxyEpoch(in.GetLogfellerTimestamp());
            row.SetQloudEnvironment(in.GetQloudEnvironment());
            writer->AddRow(row, 0);

            if (row.HasReqId() && !row.GetReqId().empty()) {
                TTimingInfoLogRow reqIdRow;
                reqIdRow.SetReqId(row.GetReqId());
                reqIdRow.SetEventId(row.GetEventId());
                writer->AddRow(reqIdRow, 1);
            }
        }
    }

private:
    bool IsAliceRow(const TUniproxyLogRow& row) {
        return row.GetQloudProject() == TStringBuf("alice") && row.GetQloudApplication() == TStringBuf("uniproxy");
    }

    bool TryParseLogMessage(TStringBuf messageBuf, TTimingInfoLogRow& row) {
        auto logType = messageBuf.NextTok(" ");
        if (logType == TStringBuf("SESSIONLOG:")) {
            return TryParseSessionLogMessage(messageBuf, row);
        }
        if (logType == TStringBuf("uniproxy.vins.adapter")
            || logType == TStringBuf("uniproxy.event.processor")
        ) {
            return TryParseUniproxyTimingLogMessage(messageBuf, row);
        }
        return false;
    }

    //uniproxy.vins.adapter INFO DATE TIME: SESSION_ID UniproxyVinsTimings {...}
    //uniproxy.event.processor INFO DATE TIME: SESSION_ID UniproxyTTSTimings {...}
    bool TryParseUniproxyTimingLogMessage(TStringBuf messageBuf, TTimingInfoLogRow& row) {
        TStringBuf logLevel = messageBuf.NextTok(" ");
        if (logLevel != TStringBuf("ERROR")) {
            if (logLevel != TStringBuf("INFO")) {
                return false;
            }
        }

        messageBuf.NextTok(" ");
        messageBuf.NextTok(" ");
        messageBuf.NextTok(" ");

        auto name = ToString(messageBuf.NextTok(" "));
        if (name != UNIPROXY_VINS_TIMINGS_NAME && name != UNIPROXY_TTS_TIMINGS_NAME) {
            return false;
        }

        row.SetNamespace(VINS_NAMESPACE.data(), VINS_NAMESPACE.size());
        row.SetName(name);

        row.SetEventId(ToString(messageBuf.NextTok(" ")));
        row.SetAdditionalInfo(ToString(messageBuf));

        return true;
    }

    bool TryParseSessionLogMessage(TStringBuf messageBuf, TTimingInfoLogRow& row) {
        NJson::TJsonValue message;
        if (!ReadJsonTree(messageBuf, &message, false)) {
            return false;
        }

        return TryParseEventMessage(message, row) || TryParseDirectiveMessage(message, row) || TryParseDirectiveInDirectiveMessage(message, row);
    }

    bool TryParseEventMessage(const NJson::TJsonValue& message, TTimingInfoLogRow& row) {
        if (!message.Has("Event")) {
            return false;
        }

        const auto& event = message["Event"]["event"];
        const auto& header = event["header"];
        if (!IsGoodEvent(event, header)) {
            return false;
        }
        row.SetEventId(header["messageId"].GetString());
        row.SetNamespace(header["namespace"].GetString());
        row.SetName(header["name"].GetString());

        auto reqId = event["payload"]["header"]["request_id"];
        row.SetReqId(reqId.GetString());

        if (row.GetName() == "RequestStat") {
            row.SetEventId(event["payload"]["refMessageId"].GetString());
            row.SetAdditionalInfo(WriteJson(event["payload"]["timestamps"]));
        }

        return true;
    }



    bool IsGoodEvent(const NJson::TJsonValue& event, const NJson::TJsonValue& header) {
        const auto& interface = header["namespace"].GetString();
        const auto& name = header["name"].GetString();

        if (interface == "Vins") {
            if (name == "VoiceInput"
                || name == "MusicInput"
                || name == "TextInput"
                || name == "CustomInput"
            ) {
                return true;
            }
        }

        if (interface == "TTS") {
            return true;
        }

        if (interface == "Log"
            && name == "RequestStat"
            && event["payload"]["timestamps"] != "null"
        ) {
            return true;
        }

        return false;
    }

    bool TryParseDirectiveInDirectiveMessage(const NJson::TJsonValue& message, TTimingInfoLogRow& row) {
        if (!message.Has("Directive")) {
            return false;
        }

        const auto& directive = message["Directive"]["directive"];
        const auto& header = directive["header"];
        if (!IsGoodDirective(header)) {
            return false;
        }
        const auto& namespace_ = header["namespace"].GetString();
        const auto& name = header["name"].GetString();

        row.SetName(name);
        row.SetEventId(header["refMessageId"].GetString());

        if (IsTimingDirective(namespace_, name)) {
            row.SetNamespace(CLIENT_VINS_NAMESPACE.data(), CLIENT_VINS_NAMESPACE.size());
            row.SetAdditionalInfo(WriteJson(directive["payload"]));
            return true;
        }

        row.SetNamespace(namespace_);

        auto reqId = directive["payload"]["header"]["request_id"];
        row.SetReqId(reqId.GetString());

        return true;
    }

    bool TryParseDirectiveMessage(const NJson::TJsonValue& message, TTimingInfoLogRow& row) {
        if (!message.Has("Directive")) {
            return false;
        }

        const auto& directive = message["Directive"];
        const auto& header = directive["header"];
        if (!IsGoodDirective(header)) {
            return false;
        }
        const auto& namespace_ = header["namespace"].GetString();
        const auto& name = header["name"].GetString();

        row.SetName(name);
        row.SetEventId(header["refMessageId"].GetString());

        if (IsTimingDirective(namespace_, name)) {
            row.SetNamespace(VINS_NAMESPACE.data(), VINS_NAMESPACE.size());
            row.SetAdditionalInfo(WriteJson(directive["payload"]));
            return true;
        }

        row.SetNamespace(namespace_);

        auto reqId = directive["payload"]["header"]["request_id"];
        row.SetReqId(reqId.GetString());

        return true;
    }

    bool IsGoodDirective(const NJson::TJsonValue& header) {
        const auto& interface = header["namespace"].GetString();
        const auto& name = header["name"].GetString();

        if (interface == "TTS") {
            return true;
        }

        if (IsTimingDirective(interface, name)) {
            return true;
        }

        if (interface == "ASR" && name == "Result") {
            return true;
        }

        if (interface == "Vins" && name == "VinsResponse") {
            return true;
        }

        return false;
    }
};
REGISTER_MAPPER(TUniproxyLogMapper);
REGISTER_REDUCER(TUniqueReducer<TNode>);
REGISTER_REDUCER(TUniqueReducer<TTimingInfoLogRow>);

class TLogMergerReduce : public IReducer<TTableReader<TTimingInfoLogRow>, TTableWriter<TTimingInfoLogRow>> {
public:
    void Do(TReader* reader, TWriter* writer) override {
        TTimingInfoLogRow clientInfo;
        TVector<TTimingInfoLogRow> savedRows;

        for (; reader->IsValid(); reader->Next()) {
            auto tableIndex = reader->GetTableIndex();
            auto row = reader->GetRow();

            if (tableIndex == 0) {
                clientInfo = std::move(row);
            } else if (tableIndex == 1) {
                savedRows.emplace_back(std::move(row));
            }
        }

        for (auto&& row: savedRows) {
            row.MergeFrom(clientInfo);
            writer->AddRow(row);
        }
    }
};

class TUniproxyLogMergerReduce : public IReducer<TTableReader<TTimingInfoLogRow>, TTableWriter<TTimingInfoLogRow>> {
public:
    void Do(TReader* reader, TWriter* writer) override {
        TString reqId;
        TVector<TTimingInfoLogRow> rows;
        TVector<TTimingInfoLogRow> timingRows;

        for (; reader->IsValid(); reader->Next()) {
            auto tableIndex = reader->GetTableIndex();
            auto row = reader->GetRow();

            if (tableIndex == 0) {
                reqId = row.GetReqId();
            } else if (tableIndex == 1) {
                // avoid empty EventId (millions of rows)
                if (!row.GetEventId().Empty()) {
                    if (IsTimingRow(row.GetNamespace(), row.GetName())){
                        if (!row.GetAdditionalInfo() || row.GetAdditionalInfo() == NULL){
                            continue;
                        }

                        if (!AnyOf(timingRows, [&row](const auto& r) { return r.GetName() == row.GetName(); })) {
                            timingRows.emplace_back(std::move(row));
                        }
                    } else {
                        rows.emplace_back(std::move(row));
                    }
                }
            }
        }

        if (IsRealReqId(reqId)) {
            for (auto&& row : timingRows) {
                row.SetReqId(reqId);
                writer->AddRow(row);
            }

            for (auto&& row : rows) {
                row.SetReqId(reqId);
                writer->AddRow(row);
            }
        }
    }
};
REGISTER_REDUCER(TLogMergerReduce)
REGISTER_REDUCER(TUniproxyLogMergerReduce)

void PrepareUniproxyLog(IClientPtr client, const NYT::TUserJobSpec& jobSpec, const TYPath& from, const TYPath& to) {
    NComputeGraph::TJobRunner runner;
    runner.SetTerminateOnExceptions(true);

    TTempTable uniproxy(client), reqId(client);
    {
        TMapOperationSpec spec;
        spec.AddInput<TUniproxyLogRow>(from)
            .AddOutput<TTimingInfoLogRow>(uniproxy.Name())
            .AddOutput<TTimingInfoLogRow>(reqId.Name())
            .MapperSpec(jobSpec);
        client->Map(spec, new TUniproxyLogMapper);
    }
    runner.AddJob([&]() {
        TSortOperationSpec spec;
        spec.SortBy({"EventId"})
            .AddInput(uniproxy.Name())
            .Output(uniproxy.Name());
        client->Sort(spec);
    });
    runner.AddJob([&]() {
        TSortOperationSpec spec;
        spec.SortBy({"EventId", "ReqId"})
            .AddInput(reqId.Name())
            .Output(reqId.Name());
        client->Sort(spec);
    });
    runner.Run();

    {
        TReduceOperationSpec spec;
        spec.ReduceBy({"EventId"})
            .AddInput<TTimingInfoLogRow>(reqId.Name())
            .AddInput<TTimingInfoLogRow>(uniproxy.Name())
            .AddOutput<TTimingInfoLogRow>(to)
            .ReducerSpec(jobSpec);
        client->Reduce(spec, new TUniproxyLogMergerReduce);
    }
}

void PrepareMegamindLog(IClientPtr client, const NYT::TUserJobSpec& jobSpec, const TYPath& analyticsLogs, const TYPath& to) {
    NComputeGraph::TJobRunner runner;
    runner.SetTerminateOnExceptions(true);

    TTempTable reqIdToIntent(client), mappedLogs(client);

    runner.AddJob([&]() {
        {
            TMapOperationSpec spec;
            spec.AddInput<TNode>(analyticsLogs)
                .AddOutput<TTimingInfoLogRow>(reqIdToIntent.Name())
                .MapperSpec(jobSpec);
            client->Map(spec, new TAnalyticsToIntentMapper);
        }
        {
            TSortOperationSpec spec;
            spec.SortBy({"ReqId"})
                .AddInput(reqIdToIntent.Name())
                .Output(reqIdToIntent.Name());
            client->Sort(spec);
        }
    });

    runner.AddJob([&]() {
        {
            TMapOperationSpec spec;
            spec.AddInput<TNode>(analyticsLogs)
                .AddOutput<TTimingInfoLogRow>(mappedLogs.Name())
                .MapperSpec(jobSpec);
            client->Map(spec, new TMegamindLogMapper);
        }
        {
            TSortOperationSpec spec;
            spec.SortBy({"ReqId"})
                .AddInput(mappedLogs.Name())
                .Output(mappedLogs.Name());
            client->Sort(spec);
        }
    });
    runner.Run();

    {
        TReduceOperationSpec spec;
        spec.ReduceBy({"ReqId"})
            .AddInput<TTimingInfoLogRow>(reqIdToIntent.Name())
            .AddInput<TTimingInfoLogRow>(mappedLogs.Name())
            .AddOutput<TTimingInfoLogRow>(to)
            .ReducerSpec(jobSpec);
        client->Reduce(spec, new TMegamindIntentReducer);
    }
}

} // namespace

void PrepareTimingTable(IClientPtr client, const NYT::TUserJobSpec& jobSpec,
        const TYPath& uniproxyLog, const TYPath& megamindAnalyticsLog, const TYPath& to) {
    NComputeGraph::TJobRunner runner;
    runner.SetTerminateOnExceptions(true);

    TTempTable megamind(client), uniproxy(client);
    runner.AddJob([&]() {
        PrepareUniproxyLog(client, jobSpec, uniproxyLog, uniproxy.Name());

        TSortOperationSpec spec;
        spec.SortBy({"ReqId"})
            .AddInput(uniproxy.Name())
            .Output(uniproxy.Name());
        client->Sort(spec);
    });

    runner.AddJob([&]() {
        PrepareMegamindLog(client, jobSpec, megamindAnalyticsLog, megamind.Name());

        TSortOperationSpec spec;
        spec.SortBy({"ReqId"})
            .AddInput(megamind.Name())
            .Output(megamind.Name());
        client->Sort(spec);
    });
    runner.Run();

    NAliceYT::CreateTableWithSchema<TTimingInfoLogRow>(client, to, true);
    {
        TReduceOperationSpec spec;
        spec.ReduceBy({"ReqId"})
            .AddInput<TTimingInfoLogRow>(megamind.Name())
            .AddInput<TTimingInfoLogRow>(uniproxy.Name())
            .AddOutput<TTimingInfoLogRow>(to)
            .ReducerSpec(jobSpec);
        client->Reduce(spec, new TLogMergerReduce);
    }
    {
        TSortOperationSpec spec;
        spec.SortBy({"ReqId", "UniproxyEpoch"})
            .AddInput(to)
            .Output(to);
        client->Sort(spec);
    }
}

void PrepareUniproxyVinsTimings(IClientPtr client, const TYPath& from, const TYPath& to) {
    TTempTable timings(client);
    {
        TMapOperationSpec spec;
        spec.AddInput<TTimingInfoLogRow>(from)
            .AddOutput<TNode>(timings.Name());
        client->Map(spec, new TUniproxyVinsTimingsMapper);
    }
    {
        TSortOperationSpec spec;
        spec.SortBy({"req_id"})
            .AddInput(timings.Name())
            .Output(timings.Name());
        client->Sort(spec);
    }
    {
        TReduceOperationSpec spec;
        spec.ReduceBy({"req_id"})
            .AddInput<TNode>(timings.Name())
            .AddOutput<TNode>(to);
        client->Reduce(spec, new TUniqueReducer<TNode>);
    }
}

} //namespace NMegamindLog
