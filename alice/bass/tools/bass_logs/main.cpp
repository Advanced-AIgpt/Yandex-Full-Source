#include <alice/bass/tools/bass_logs/protos/logs.pb.h>
#include <alice/library/yt/protos/logs.pb.h>

#include <alice/bass/libs/client/client_info.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <alice/library/yt/util.h>

#include <mapreduce/yt/common/config.h>
#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/interface/logging/logger.h>
#include <mapreduce/yt/interface/init.h>
#include <mapreduce/yt/library/table_schema/protobuf.cpp>
#include <mapreduce/yt/util/temp_table.h>
#include <mapreduce/yt/library/lambda/wrappers.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/compute_graph/compute_graph.h>
#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/getopt/modchooser.h>
#include <library/cpp/http/misc/httpcodes.h>
#include <library/cpp/protobuf/yt/proto2yt.h>
#include <library/cpp/scheme/scheme.h>

#include <util/generic/string.h>
#include <util/string/cast.h>
#include <util/string/split.h>
#include <util/string/strip.h>

using namespace NYT;

using TFunc = std::function<void(IClientBasePtr, const TYPath&, const TYPath&)>;

constexpr TStringBuf DEVICE_ID = "device_id";

template <typename TSchema>
void MapTables(NYT::IClientPtr client, const TYPath& from, const TYPath& to, bool force, bool append, int count, TFunc func) {
    TString type = client->Get(from + "/@type").AsString();
    if (client->Exists(to)) {
        Y_ENSURE(client->Get(to + "/@type").AsString() == type, "Target path " << to << " is not a " << type);
    }

    if (type == "table") {
        if (client->Exists(to) && !force && !append) {
            Cerr << "Table " << to << " already exists, skipping" << Endl;
            return;
        }

        auto tx = client->StartTransaction();
        NAliceYT::CreateTableWithSchema<TSchema>(tx, to, force);
        func(tx, from, to);
        tx->Commit();
        return;
    }
    if (type == "map_node") {
        if (!client->Exists(to)) {
            client->Create(to, NT_MAP);
        }

        auto list = client->List(from);
        for (const auto& l : list) {
            const TString& name = l.AsString();
            MapTables<TSchema>(client, from + "/" + name, to + "/" + name, force, append, count, func);
        }
        return;
    }
    ythrow yexception() << "Unsupported type " << type << " of node " << from;
}

template <class T>
class TSplitMapper : public NYT::NDetail::TLambdaOpBase<IMapper, T, T, bool (*)(const T&)> {
public:
    using F = bool (*)(const T&);
    using TBase = NYT::NDetail::TLambdaOpBase<IMapper, T, T, F>;

    TSplitMapper() {
    }
    TSplitMapper(F func) : TBase(func) {
        (void)Registrator; // make sure this static member is not optimized out
    }

    void Do(TTableReader<T>* reader, TTableWriter<T>* writer) override {
        for (; reader->IsValid(); reader->Next()) {
            const auto& curRow = reader->GetRow();
            int tableIndex = TBase::Func(curRow) ? 0 : 1;
            writer->AddRow(curRow, tableIndex);
        }
    }

private:
    static bool Registrator;
};

template <class T>
bool TSplitMapper<T>::Registrator = NYT::NDetail::RegisterMapperStatic<TSplitMapper<T>>();

template <class T>
void Split(
    IClientBasePtr c,
    const TRichYPath& from,
    const TRichYPath& toTrue,
    const TRichYPath& toFalse,
    bool (*p)(const T&))
{
    auto spec = TMapOperationSpec();
    spec.Ordered(true)
        .AddInput<T>(from)
        .template AddOutput<T>(toTrue)
        .template AddOutput<T>(toFalse);
    c->Map(spec, p ? new TSplitMapper<T>(p) : nullptr);
}

// Returns a string in format 'YYYY-MM-DDThh:mm:ss'
TString AddMinutes(const TString& time, int minutes) {
    auto date = TInstant::ParseIso8601(time);
    date += TDuration::Minutes(minutes);
    return TString{TStringBuf(date.ToString()).Before('.')};
}

std::pair<TString, TString> SplitPath(const TString& path) {
    TStringBuf left, right;
    TStringBuf(path).RSplit('/', left, right);
    return std::make_pair(TString(left) + '/', TString(right));
}

TString ParseTime(const TYPath& in) {
    TString answer = SplitPath(in).second;
    TInstant inst;
    Y_ENSURE(TInstant::TryParseIso8601(TStringBuf(answer), inst), "Table name is not ISO 8601: " << answer);
    return answer;
}

template <typename TSpec>
void AddInputsNoSchema(TSpec& spec, int cntTables, int minutes, const TYPath& in, bool onlyDay = false) {
    auto [prefix, inPath] = SplitPath(in);
    for (int i = 0; i < cntTables; ++i) {
        if (onlyDay) {
            // Table name format: YYYY-MM-DD
            spec.AddInput(prefix + inPath.substr(0, 10));
        } else {
            // Table name format: YYYY-MM-DDThh:mm:ss
            spec.AddInput(prefix + inPath);
        }
        inPath = AddMinutes(inPath, minutes);
    }
}

template <typename TSchema, typename TSpec>
void AddInputsSchema(TSpec& spec, int cntTables, int minutes, const TYPath& in, bool onlyDay = false) {
    auto [prefix, inPath] = SplitPath(in);
    for (int i = 0; i < cntTables; ++i) {
        if (onlyDay) {
            // Table name format: YYYY-MM-DD
            spec.template AddInput<TSchema>(prefix + inPath.substr(0, 10));
        } else {
            // Table name format: YYYY-MM-DDThh:mm:ss
            spec.template AddInput<TSchema>(prefix + inPath);
        }
        inPath = AddMinutes(inPath, minutes);
    }
}

class TConvertLogsMapper : public IMapper<TTableReader<TBassLegacyLogRow>, TTableWriter<TBassLogRow>> {
public:
    void Do(TReader* reader, TWriter* writer) override {
        for (; reader->IsValid(); reader->Next()) {
            const TBassLegacyLogRow& input = reader->GetRow();

            TBassLogRow row;
            row.SetTimestamp(input.GetTimestamp());
            row.SetTime(ParseTime(input.GetTimestamp()).MilliSeconds());
            row.SetType(input.GetType());
            row.SetContent(input.GetContent());
            row.SetReqId(input.GetReqId());
            row.SetEnv(input.GetEnv());
            row.SetHypothesisNumber(input.GetHypothesisNumber());
            ParseRequest(input.GetContent(), &row);
            writer->AddRow(row);
        }
    }

private:
    static TInstant ParseTime(TStringBuf ts) {
        tm tm;
        tm.tm_isdst = 0;
        char* ret = ::strptime(ts.data(), "%Y-%m-%d %H:%M:%S", &tm);
        Y_ENSURE(ret, "Bad timestamp format " << ts);
        TInstant t = TInstant::Seconds(mktime(&tm));
        return t + TDuration::MilliSeconds(FromString<ui32>(ts.After('.').Before(' ')));
    }

    void RemoveNestedQuotes(TString& s) {
        bool balance = false;
        int last = -1;
        bool foundDelimeter = false;
        for (size_t i = 0; i < s.size(); ++i) {
            const auto symbol = s.substr(i, 1);
            if (symbol != "\"") {
                if (symbol == "," || symbol == ":") {
                    foundDelimeter = true;
                }
                continue;
            }
            balance ^= true;
            if (balance && !foundDelimeter && last != -1) {
                s.erase(i, 1);
                s.erase(last, 1);
                i -= 2;
                last = -1;
            } else {
                last = i;
            }
            foundDelimeter = false;
        }
    }

    void ParseRequest(TStringBuf content, TBassLogRow* row) {
        TStringBuf message = content.NextTok(":");
        if (!message) {
            return;
        }

        TString s = Strip(TString{content});
        RemoveNestedQuotes(s);
        NSc::TValue v = NSc::TValue::FromJson(s);
        if (v.IsNull()) {
            return;
        }

        row->SetJson(s);
        row->SetMessage(Strip(TString{message}));

        auto form = v["form"];
        auto meta = v["meta"];
        auto deviceState = meta["device_state"];

        const TStringBuf formName = form["name"].GetString();
        const TStringBuf uuid = meta["uuid"].GetString();
        const TStringBuf clientId = meta["client_id"].GetString();
        const TStringBuf requestId = meta["request_id"].GetString();

        const TStringBuf actionName = v["action"]["name"].GetString();

        TStringBuf deviceId = deviceState[DEVICE_ID].GetString();
        if (deviceId.empty()) {
            deviceId = meta[DEVICE_ID];
        }

        const ui64 sequenceNumber = meta["sequence_number"].GetIntNumber();
        const TStringBuf utterance = meta["utterance"].GetString();

        if (formName) {
            row->SetFormName(TString{formName});
        }
        if (actionName) {
            row->SetActionName(TString{actionName});
        }
        if (uuid) {
            row->SetUuid(TString{uuid});
        }

        if (clientId) {
            row->SetClientId(TString{clientId});
        }

        if (deviceId) {
            row->SetDeviceId(TString{deviceId});
        }

        if (sequenceNumber) {
            row->SetSequenceNumber(sequenceNumber);
        }

        if (utterance) {
            row->SetUtterance(TString{utterance});
        }

        if (requestId) {
            row->SetReqId(TString{requestId});
        }

    }
};
REGISTER_MAPPER(TConvertLogsMapper);

struct TCommonOpts {
    static constexpr ui64 DEFAULT_MEMORY_LIMIT_MB = 1024;

    TString Server;
    TString Input;
    TString Output;
    ui64 MemoryLimitMB = DEFAULT_MEMORY_LIMIT_MB;
    int Count = 1;
    bool Force = false;
    bool Append = false;

    TCommonOpts(NLastGetopt::TOpts& opts) {
        opts.AddLongOption('s', "server", "YT server")
            .Required().RequiredArgument().StoreResult(&Server);
        opts.AddLongOption('i', "input", "Input YT path (table or folder to scan for tables in)")
            .Required().RequiredArgument().StoreResult(&Input);
        opts.AddLongOption('o', "output", "Output YT path (table or folder)")
            .Required().RequiredArgument().StoreResult(&Output);
        opts.AddLongOption('f', "force", "Force rewrite destination tables")
            .NoArgument().SetFlag(&Force);
        opts.AddLongOption('a', "append", "Append into destination tables")
            .NoArgument().SetFlag(&Append);
        opts.AddLongOption('c', "count", "Number of input tables")
            .RequiredArgument().StoreResult(&Count);
        opts.AddLongOption("memory-limit-mb", "Memory limit for a worker, in megabytes")
            .DefaultValue(ToString(DEFAULT_MEMORY_LIMIT_MB))
            .StoreResult(&MemoryLimitMB);
    }

    static TCommonOpts Parse(int argc, const char** argv) {
        NLastGetopt::TOpts opts;
        TCommonOpts common(opts);
        NLastGetopt::TOptsParseResult optsRes(&opts, argc, argv);
        return common;
    }
};

int RunConvert(int argc, const char** argv) {
    const auto opts = TCommonOpts::Parse(argc, argv);
    auto client = NYT::CreateClient(opts.Server);

    MapTables<TBassLogRow>(client, opts.Input, opts.Output, opts.Force, opts.Append, opts.Count,
        [&opts](IClientBasePtr c, const TYPath& in, const TYPath& out) {
            auto spec = NAliceYT::DefaultMapOperationSpec(opts.MemoryLimitMB);
            spec.AddOutput<TBassLogRow>(TRichYPath(out).Append(opts.Append))
                .Ordered(true);
            AddInputsSchema<TBassLegacyLogRow, TMapOperationSpec>(spec, opts.Count, 5, in);
            c->Map(spec, new TConvertLogsMapper, NAliceYT::DefaultOperationOptions());
        });
    return 0;
}

class TBriefRequestMapper : public IMapper<TTableReader<TBassLogRow>, TTableWriter<TBassLogRow>> {
public:
    void Do(TReader* reader, TWriter* writer) override {
        for (; reader->IsValid(); reader->Next()) {
            if (!reader->GetRow().GetReqId().empty()) {
                writer->AddRow(reader->GetRow());
            }
        }
    }
};
REGISTER_MAPPER(TBriefRequestMapper);

class TBriefRequestReducer : public IReducer<TTableReader<TBassLogRow>, TTableWriter<TBassBriefLogRow>> {
public:
    void Do(TReader* reader, TWriter* writer) override {
        TString startTs, finishTs;
        TDuration start = TDuration::Max();
        TDuration finish = TDuration::Zero();
        ui64 count = 0;
        ui64 errCount = 0;

        TBassBriefLogRow row;

        for (; reader->IsValid(); reader->Next()) {
            const auto& in = reader->GetRow();

            if (in.GetReqId().empty()) {
                continue;
            }

#define COPY_FIELD(name)                           \
            do {                                   \
                if (in.Has##name()) {              \
                    row.Set##name(in.Get##name()); \
                }                                  \
            } while (0);

            COPY_FIELD(ReqId);
            COPY_FIELD(Uuid);
            COPY_FIELD(ClientId);
            COPY_FIELD(DeviceId);
            COPY_FIELD(SequenceNumber);
            COPY_FIELD(FormName);
            COPY_FIELD(Utterance);
            COPY_FIELD(ActionName);
            COPY_FIELD(Env);
            COPY_FIELD(NannyServiceId);
            COPY_FIELD(HypothesisNumber);
#undef COPY_FIELD

            if (!startTs || startTs > in.GetTimestamp()) {
                startTs = in.GetTimestamp();
            }
            if (!finishTs || finishTs < in.GetTimestamp()) {
                finishTs = in.GetTimestamp();
            }

            TDuration time = TDuration::MilliSeconds(in.GetTime());
            start = Min(start, time);
            finish = Max(finish, time);

            if (in.GetType() == "ERROR") {
                ++errCount;
            }
            ++count;

            if (in.GetMessage() == "form response") {
                NSc::TValue json = NSc::TValue::FromJson(in.GetJson());
                for (const auto& block : json["blocks"].GetArray()) {
                    if (block["type"].GetString() == "error") {
                        row.SetHasErrorBlock(true);
                    }
                }
            }
            TStringBuf headerReqId;
            if (TStringBuf(in.GetContent()).AfterPrefix("BASS reqid : ", headerReqId)) {
                row.SetHeaderReqId(TString{headerReqId});
            }
            TStringBuf content = in.GetContent();
            if (content.AfterPrefix("search request: ", content)) {
                TCgiParameters cgi(content.After('?'));
                row.SetSearchRequest(cgi.Get("text"));
                row.MutableData()->AddSearchRequests(cgi.Get("text"));
            }
        }

        if (count == 0) {
            return;
        }

        row.SetStartTimestamp(startTs);
        row.SetStartTime(start.MilliSeconds());
        row.SetFinishTimestamp(finishTs);
        row.SetFinishTime(finish.MilliSeconds());
        row.SetDuration(finish.MilliSeconds() - start.MilliSeconds());
        row.SetLogMessagesCount(count);
        row.SetErrorMessagesCount(errCount);
        writer->AddRow(row);
    }
};
REGISTER_REDUCER(TBriefRequestReducer);

int RunBrief(int argc, const char** argv) {
    const auto opts = TCommonOpts::Parse(argc, argv);
    auto client = NYT::CreateClient(opts.Server);

    MapTables<TBassBriefLogRow>(client, opts.Input, opts.Output, opts.Force, opts.Append, opts.Count,
        [&opts](IClientBasePtr c, const TYPath& in, const TYPath& out) {
            TTempTable tmp(c);
            {
                auto spec = NAliceYT::DefaultMapReduceOperationSpec(opts.MemoryLimitMB);
                spec.AddOutput<TBassBriefLogRow>(tmp.Name())
                    .SortBy({"ReqId"})
                    .ReduceBy({"ReqId"});
                AddInputsSchema<TBassLogRow, TMapReduceOperationSpec>(spec, opts.Count, 5, in);
                c->MapReduce(spec, new TBriefRequestMapper, new TBriefRequestReducer, NAliceYT::DefaultOperationOptions());
            }
            {
                TSortOperationSpec spec;
                spec.AddInput(tmp.Name()).AddInput(out).Output(out).SortBy({"StartTime", "ReqId"});
                c->Sort(spec);
            }
        });
    return 0;
}

int RunSort(int argc, const char** argv) {
    const auto opts = TCommonOpts::Parse(argc, argv);

    auto client = NYT::CreateClient(opts.Server);

    MapTables<TBassLogRow>(client, opts.Input, opts.Output, opts.Force, opts.Append, opts.Count,
        [&opts](IClientBasePtr c, const TYPath& in, const TYPath& out) {
            {
                TSortOperationSpec spec;
                spec.AddInput(out).Output(out).SortBy({"ReqId"});
                AddInputsNoSchema<TSortOperationSpec>(spec, opts.Count, 5, in);
                c->Sort(spec);
            }
        });
    return 0;
}

class TSourceErrorReducer : public IReducer<TTableReader<TBassBriefLogRow>, TTableWriter<TBassErrorStatsRow>> {
public:
    explicit TSourceErrorReducer(const TString& startTime)
        : StartTime(startTime) {
    }

    TSourceErrorReducer() = default;

    Y_SAVELOAD_JOB(StartTime);

public:
    void Do(TReader* reader, TWriter* writer) override {
        ui64 logCount = 0;
        ui64 errCount = 0;
        TBassErrorStatsRow row;

        for (; reader->IsValid(); reader->Next()) {
            const auto& in = reader->GetRow();

            if (in.GetFormName().empty()) {
                continue;
            }

            logCount += in.GetLogMessagesCount();
            errCount += in.GetErrorMessagesCount();

            row.SetFormName(in.GetFormName());
        }

        if (logCount == 0) {
            return;
        }

        row.SetStartTimestamp(StartTime);
        row.SetLogMessagesCount(logCount);
        row.SetErrorMessagesCount(errCount);
        writer->AddRow(row);
    }

private:
    TString StartTime;
};
REGISTER_REDUCER(TSourceErrorReducer);

int RunErrorsStats(int argc, const char** argv) {
    const auto opts = TCommonOpts::Parse(argc, argv);

    auto client = NYT::CreateClient(opts.Server);

    MapTables<TBassErrorStatsRow>(client, opts.Input, opts.Output, opts.Force, opts.Append, opts.Count,
        [&opts](IClientBasePtr c, const TYPath& in, const TYPath& out) {
            TTempTable convertedTable(c), briefTable(c), tmp(c);
            {
                auto spec = NAliceYT::DefaultMapOperationSpec(opts.MemoryLimitMB);
                spec.AddOutput<TBassLogRow>(convertedTable.Name())
                    .Ordered(true);
                AddInputsSchema<TBassLegacyLogRow, TMapOperationSpec>(spec, opts.Count, 5, in);
                c->Map(spec, new TConvertLogsMapper);
            }
            {
                auto spec = NAliceYT::DefaultMapReduceOperationSpec(opts.MemoryLimitMB);
                spec.AddInput<TBassLogRow>(convertedTable.Name())
                    .AddOutput<TBassBriefLogRow>(briefTable.Name())
                    .SortBy({"ReqId"})
                    .ReduceBy({"ReqId"});
                c->MapReduce(spec, new TBriefRequestMapper, new TBriefRequestReducer);
            }
            {
                auto spec = NAliceYT::DefaultMapReduceOperationSpec(opts.MemoryLimitMB);
                spec.AddInput<TBassBriefLogRow>(briefTable.Name())
                    .AddOutput<TBassErrorStatsRow>(tmp.Name())
                    .SortBy({"FormName"})
                    .ReduceBy({"FormName"});
                c->MapReduce(spec, nullptr, new TSourceErrorReducer(ParseTime(in)));
            }
            {
                TSortOperationSpec spec;
                spec.AddInput(tmp.Name()).AddInput(out).Output(out).SortBy({"fielddate", "formname"});
                c->Sort(spec);
            }
        });

    return 0;
}

class TUniproxyLogMapper : public IMapper<TTableReader<TUniproxyLogRow>, TTableWriter<TBassUniproxyLogRow>> {
public:
    void Do(TReader* reader, TWriter* writer) override {
        for (; reader->IsValid(); reader->Next()) {
            const auto& in = reader->GetRow();

            int tableIndex = reader->GetTableIndex();
            if (!IsAliceRow(in, GetQloudProject(tableIndex))) {
                continue;
            }

            TStringBuf messageBuf(in.GetMessage());
            TBassUniproxyLogRow row;

            if (!TryParseLogMessage(messageBuf, row)) {
                continue;
            }

            row.SetTimestamp(in.GetTimestamp());
            writer->AddRow(row);
        }
    }

private:
    TStringBuf GetQloudProject(int tableIndex) {
        return tableIndex == 0 ? TStringBuf("voice-ext") : TStringBuf("alice");
    }

    bool IsAliceRow(const TUniproxyLogRow& row, TStringBuf qloudProject) {
        return row.GetQloudProject() == qloudProject && row.GetQloudApplication() == TStringBuf("uniproxy");
    }

    bool TryParseLogMessage(TStringBuf messageBuf, TBassUniproxyLogRow& row) {
        auto logType = messageBuf.NextTok(" ");
        if (logType == TStringBuf("ACCESSLOG:")) {
            return TryParseAccessLogMessage(messageBuf, row);
        }
        if (logType == TStringBuf("SESSIONLOG:")) {
            return TryParseSessionLogMessage(messageBuf, row);
        }
        return false;
    }

    //AccessLog message example:
    //ACCESSLOG: HOST_NAME HOST_IP \"POST /vins 1/1\" 200 \"EVENT_ID\" \"UUID\" \"SESSION_ID\" \"GUID\" 0 - 0 \"0.5847442150\" 5616
    bool TryParseAccessLogMessage(TStringBuf messageBuf, TBassUniproxyLogRow& row) {
        messageBuf.NextTok("\"");
        if (messageBuf.NextTok("\" ") != TStringBuf("POST /vins 1/1")) {
            return false;
        }

        auto httpCode = FromString<ui64>(messageBuf.NextTok(" "));
        if (httpCode != HttpCodes::HTTP_OK && httpCode != HttpCodes::HTTP_RESET_CONTENT) {
            return false;
        }

        messageBuf.NextTok("\"");
        auto eventId = ToString(messageBuf.NextTok("\" "));

        messageBuf.NextTok(" ");
        messageBuf.NextTok(" ");
        messageBuf.NextTok(" ");

        messageBuf.NextTok("\"");
        auto duration = FromString<float>(messageBuf.NextTok("\""));

        row.SetEventId(eventId);
        row.SetDuration(duration);
        return true;
    }

    bool TryParseSessionLogMessage(TStringBuf messageBuf, TBassUniproxyLogRow& row) {
        NJson::TJsonValue message;
        if (!ReadJsonTree(messageBuf, &message, false)) {
            return false;
        }

        if (!message.Has("Event")) {
            return false;
        }
        auto type = message["Event"]["event"]["header"]["namespace"];
        if (type.GetString() != "Vins") {
            return false;
        }

        auto reqId = message["Event"]["event"]["payload"]["header"]["request_id"];
        auto eventId = message["Event"]["event"]["header"]["messageId"];

        row.SetReqId(reqId.GetString());
        row.SetEventId(eventId.GetString());
        return true;
    }
};
REGISTER_MAPPER(TUniproxyLogMapper);

class TUniproxyLogReducer : public IReducer<TTableReader<TBassUniproxyLogRow>, TTableWriter<TBassUniproxyLogRow>> {
public:
    void Do(TReader* reader, TWriter* writer) override {
        TString reqId;
        bool hasDurationRow = false;
        TBassUniproxyLogRow lastRow;
        for (; reader->IsValid(); reader->Next()) {
            const auto& row = reader->GetRow();
            if (row.HasReqId()) {
                reqId = row.GetReqId();
            } else {
                lastRow = row;
                hasDurationRow = true;
            }
        }

        if (!reqId.empty() && hasDurationRow) {
            lastRow.SetReqId(reqId);
            writer->AddRow(lastRow);
        }
    }
};
REGISTER_REDUCER(TUniproxyLogReducer)

class TBassUniproxyLogReducer : public IReducer<TTableReader<Message>, TTableWriter<TBassDurationStatsRow>> {
public:
    void Do(TReader* reader, TWriter* writer) override {
        bool hasBass = false;
        bool hasUniproxy = false;

        TBassDurationStatsRow row;
        for (; reader->IsValid(); reader->Next()) {
            auto tableIndex = reader->GetTableIndex();
            if (tableIndex == 0) {
                hasUniproxy = true;
                const auto& uniproxyRow = reader->GetRow<TBassUniproxyLogRow>();
                row.SetUniproxyDuration(uniproxyRow.GetDuration());
            }
            if (tableIndex == 1) {
                hasBass = true;
                const auto& bassRow = reader->GetRow<TBassBriefLogRow>();

                row.SetStartTime(bassRow.GetStartTime());
                row.SetStartTimestamp(bassRow.GetStartTimestamp());
                row.SetFinishTime(bassRow.GetFinishTime());
                row.SetFinishTimestamp(bassRow.GetFinishTimestamp());
                row.SetReqId(bassRow.GetReqId());
                row.SetUuid(bassRow.GetUuid());
                if (bassRow.HasActionName()) {
                    row.SetFormName("bass_action." + bassRow.GetActionName());
                } else {
                    row.SetFormName(bassRow.GetFormName());
                }
                row.SetIsQuasar(NBASS::TClientInfo(bassRow.GetClientId()).IsQuasar());
                row.SetBassDuration(bassRow.GetDuration());
            }
        }

        if (hasUniproxy && hasBass && !row.GetFormName().empty()) {
            writer->AddRow(row);
        }
    }
};
REGISTER_REDUCER(TBassUniproxyLogReducer)

int RunUniproxy(int argc, const char** argv) {
    NLastGetopt::TOpts opts;

    TString newAliceLog;
    opts.AddLongOption("new-alice-input", "Input YT path (table or folder to scan for tables in)")
        .Required().RequiredArgument().StoreResult(&newAliceLog);

    TCommonOpts common(opts);
    NLastGetopt::TOptsParseResult optsRes(&opts, argc, argv);

    auto client = NYT::CreateClient(common.Server);

    MapTables<TBassUniproxyLogRow>(client, common.Input, common.Output, common.Force, common.Append, common.Count,
        [&common, &newAliceLog](IClientBasePtr c, const TYPath& in, const TYPath& out) {
            TTempTable uniproxyLog(c), sortedBriefLog(c);
            {
                auto spec = NAliceYT::DefaultMapOperationSpec(common.MemoryLimitMB);
                spec.AddInput<TUniproxyLogRow>(in)
                    .AddInput<TUniproxyLogRow>(newAliceLog)
                    .AddOutput<TBassUniproxyLogRow>(uniproxyLog.Name());
                c->Map(spec, new TUniproxyLogMapper);
            }
            {
                TSortOperationSpec spec;
                spec.SortBy({"EventId", "Timestamp"})
                    .AddInput(uniproxyLog.Name())
                    .Output(uniproxyLog.Name());
                c->Sort(spec);
            }
            {
                auto spec = NAliceYT::DefaultReduceOperationSpec(common.MemoryLimitMB);
                spec.ReduceBy({"EventId"})
                    .SortBy({"EventId", "Timestamp"})
                    .AddInput<TBassUniproxyLogRow>(uniproxyLog.Name())
                    .AddOutput<TBassUniproxyLogRow>(uniproxyLog.Name()),
                c->Reduce(spec, new TUniproxyLogReducer);
            }
            {
                TSortOperationSpec spec;
                spec.SortBy({"Timestamp", "ReqId"})
                    .AddInput(uniproxyLog.Name())
                    .Output(out);
                c->Sort(spec);
            }
        });

    return 0;
}

int RunMergeUniproxyAndBrief(int argc, const char** argv) {
    NLastGetopt::TOpts opts;

    TString briefLog;
    opts.AddLongOption("brief-input", "Input YT path (table or folder to scan for tables in)")
        .Required().RequiredArgument().StoreResult(&briefLog);

    TCommonOpts common(opts);
    NLastGetopt::TOptsParseResult optsRes(&opts, argc, argv);

    auto client = NYT::CreateClient(common.Server);

    MapTables<TBassDurationStatsRow>(client, common.Input, common.Output, common.Force, common.Append, common.Count,
        [&common, &briefLog](IClientBasePtr c, const TYPath& in, const TYPath& out) {
            NComputeGraph::TJobRunner runner;
            runner.SetTerminateOnExceptions(true);

            TTempTable uniproxyLog(c), sortedBriefLog(c);
            runner.AddJob([&]() {
                TSortOperationSpec spec;
                spec.SortBy({"ReqId"})
                    .AddInput(in)
                    .Output(uniproxyLog.Name());
                c->Sort(spec);
            });
            runner.AddJob([&]() {
                TSortOperationSpec spec;
                spec.SortBy({"ReqId"})
                    .AddInput(briefLog)
                    .Output(sortedBriefLog.Name());
                c->Sort(spec);
            });
            runner.Run();

            {
                auto spec = NAliceYT::DefaultReduceOperationSpec(common.MemoryLimitMB);
                spec.ReduceBy({"ReqId"})
                    .AddInput<TBassUniproxyLogRow>(uniproxyLog.Name())
                    .AddInput<TBassBriefLogRow>(sortedBriefLog.Name())
                    .AddOutput<TBassDurationStatsRow>(out);
                c->Reduce(spec, new TBassUniproxyLogReducer);
            }
        });

    return 0;
}

void FillQuantileStats(const TVector<double>& quantiles, TBassQuantileStatsRow& row);
TVector<size_t> CalculateQuantileIndexes(const TVector<double>& quantiles, size_t count);

class TUniproxyQuantileReducer : public IReducer<TTableReader<TBassDurationStatsRow>, TTableWriter<TBassQuantileStatsRow>> {
public:
    TUniproxyQuantileReducer()
        : RequestCount(0) {
    }

    TUniproxyQuantileReducer(const TString& formType, const TString& startTime, i64 requestCount, const TVector<double>& quantiles)
        : FormType(formType), StartTime(startTime), RequestCount(requestCount), Quantiles(quantiles) {
    }

    Y_SAVELOAD_JOB(FormType, StartTime, RequestCount, Quantiles);

    void Do(TReader* reader, TWriter* writer) override {
        TVector<double> durations;

        TString formName;
        for (; reader->IsValid(); reader->Next()) {
            const auto& in = reader->GetRow();
            formName = in.GetFormName();
            durations.push_back(in.GetUniproxyDuration());
        }

        Y_VERIFY(!durations.empty());
        Sort(durations.begin(), durations.end());
        writer->AddRow(MakeRow(formName, durations));
    }

private:
    TString FormType;
    TString StartTime;
    i64 RequestCount;
    TVector<double> Quantiles;

    TBassQuantileStatsRow MakeRow(const TString& formName, const TVector<double>& durations) {
        TBassQuantileStatsRow row;
        row.SetFormName(formName);
        row.SetFormType(FormType);
        row.SetStartTimestamp(StartTime);

        FillQuantileStats(CalculateQuantiles(durations), row);

        double intentRequestCount = static_cast<double>(durations.size());
        row.SetTF(intentRequestCount);
        row.SetPercentage(intentRequestCount / RequestCount);

        return row;
    }

    TVector<double> CalculateQuantiles(const TVector<double>& durations) {
        TVector<double> quantileValues;
        for (auto i : CalculateQuantileIndexes(Quantiles, durations.size())) {
            quantileValues.push_back(durations[i]);
        }
        return quantileValues;
    }
};
REGISTER_REDUCER(TUniproxyQuantileReducer);

class TTFMapper : public IMapper<TTableReader<TBassQuantileStatsRow>, TTableWriter<TBassQuantileStatsRow>> {
public:
    TTFMapper()
        : IntentCount(0) {
    }

    explicit TTFMapper(i64 intentCount)
        : IntentCount(intentCount) {
    }

    Y_SAVELOAD_JOB(IntentCount);

    void Do(TReader* reader, TWriter* writer) override {
        for (; reader->IsValid(); reader->Next()) {
            auto row = reader->GetRow();
            row.SetTF(row.GetTF() / IntentCount);
            writer->AddRow(row);
        }
    }

private:
    i64 IntentCount;
};
REGISTER_MAPPER(TTFMapper)

void CalculateTotalQuantiles(IClientBasePtr c, const TYPath& in, const TYPath& out,
                             TBassQuantileStatsRow row, const TVector<double>& quantiles ) {
    auto path = TRichYPath(in);
    auto count = c->Get(in + "/@row_count").AsInt64();
    if (count == 0) {
        Cerr << "Input data by type \"" << row.GetFormType() << "\" is empty" << Endl;
        return;
    }
    for (auto i : CalculateQuantileIndexes(quantiles, count)) {
        path.AddRange(TReadRange().Exact(TReadLimit().RowIndex(i)));
    }

    TVector<double> quantileValues;
    auto reader = c->CreateTableReader<TBassDurationStatsRow>(path);
    for (; reader->IsValid(); reader->Next()) {
        quantileValues.push_back(reader->GetRow().GetUniproxyDuration());
    }

    auto writer = c->CreateTableWriter<TBassQuantileStatsRow>(TRichYPath(out).Append(true));
    FillQuantileStats(quantileValues, row);
    writer->AddRow(row);
}

struct TTempQuasarTables {
    explicit TTempQuasarTables(IClientBasePtr c)
        : All(c)
        , Quasar(c)
        , NoQuasar(c) {
    }

    TTempTable All;
    TTempTable Quasar;
    TTempTable NoQuasar;
};

void CalculateQuantiles(IClientBasePtr c, const TYPath& formNameIn, const TYPath& durationIn, const TYPath& out,
                        const TString& startTime, const TString& formType, const TVector<double>& quantiles, const TCommonOpts& opts) {
    TTempTable tmp(c);
    {
        auto count = c->Get(formNameIn + "/@row_count").AsInt64();
        auto spec = NAliceYT::DefaultReduceOperationSpec(opts.MemoryLimitMB);
        spec.ReduceBy({"FormName"})
            .AddInput<TBassDurationStatsRow>(formNameIn)
            .AddOutput<TBassQuantileStatsRow>(tmp.Name()),
        c->Reduce(spec, new TUniproxyQuantileReducer(formType, startTime, count, quantiles));
    }
    {
        auto count = c->Get(tmp.Name() + "/@row_count").AsInt64();
        auto spec = NAliceYT::DefaultMapOperationSpec(opts.MemoryLimitMB);
        spec.AddInput<TBassQuantileStatsRow>(tmp.Name())
            .AddOutput<TBassQuantileStatsRow>(out);
        c->Map(spec, new TTFMapper(count));
    }
    {
        TBassQuantileStatsRow totalRow;
        totalRow.SetFormName("total");
        totalRow.SetFormType(formType);
        totalRow.SetStartTimestamp(startTime);
        totalRow.SetPercentage(1);
        CalculateTotalQuantiles(c, durationIn, out, totalRow, quantiles);
    }
}

void CalculateSkillQuantiles(IClientBasePtr c, const TTempQuasarTables& inAll, const TTempQuasarTables& inSkills, const TYPath& out,
                             const TString& startTime, const TString& formName, const TVector<double>& quantiles) {
    TBassQuantileStatsRow totalRow;
    totalRow.SetFormName(formName);
    totalRow.SetStartTimestamp(startTime);
    auto calculateFunc = [&] (const TYPath& inAllTable, const TYPath& inSkillsTable, const TString& formType) {
        auto allCount = c->Get(inAllTable + "/@row_count").AsInt64();
        auto skillCount = c->Get(inSkillsTable + "/@row_count").AsInt64();
        if (allCount == 0 || skillCount == 0) {
            Cerr << "Input data by type \"" << formType << "\" is empty" << Endl;
            return;
        }
        totalRow.SetFormType(formType);
        totalRow.SetPercentage(static_cast<double>(skillCount) / allCount);
        CalculateTotalQuantiles(c, inSkillsTable, out, totalRow, quantiles);
    };
    calculateFunc(inAll.All.Name(), inSkills.All.Name(), "all");
    calculateFunc(inAll.Quasar.Name(), inSkills.Quasar.Name(), "quasar");
    calculateFunc(inAll.NoQuasar.Name(), inSkills.NoQuasar.Name(), "no_quasar");
}

void SortAndSplitByQuasar(IClientBasePtr c, const TYPath& in, const TTempQuasarTables& out, const TString& sortField) {
    {
        TSortOperationSpec spec;
        spec.AddInput(in).Output(out.All.Name()).SortBy({sortField});
        c->Sort(spec);
    }
    Split<TBassDurationStatsRow>(
        c, out.All.Name(),
        NYT::TRichYPath(out.Quasar.Name()).SortedBy({sortField}),
        NYT::TRichYPath(out.NoQuasar.Name()).SortedBy({sortField}),
        [] (const auto& row) { return row.GetIsQuasar(); }
    );
}

TVector<size_t> CalculateQuantileIndexes(const TVector<double>& quantiles, size_t count) {
    TVector<size_t> indexes;
    for (auto q : quantiles) {
        indexes.push_back(static_cast<size_t>(count * q / 100));
    }
    return indexes;
}

void FillQuantileStats(const TVector<double>& quantiles, TBassQuantileStatsRow& row) {
    Y_VERIFY(quantiles.size() == 6);
    row.SetQuantile_50(quantiles[0]);
    row.SetQuantile_75(quantiles[1]);
    row.SetQuantile_90(quantiles[2]);
    row.SetQuantile_95(quantiles[3]);
    row.SetQuantile_99(quantiles[4]);
    row.SetQuantile_99_9(quantiles[5]);
}

int RunQuantileStats(int argc, const char** argv) {
    const auto opts = TCommonOpts::Parse(argc, argv);

    auto client = NYT::CreateClient(opts.Server);

    MapTables<TBassQuantileStatsRow>(client, opts.Input, opts.Output, opts.Force, opts.Append, opts.Count,
        [&opts](IClientBasePtr c, const TYPath& in, const TYPath& out) {
            static const TVector<double> quantiles = {50, 75, 90, 95, 99, 99.9};
            NComputeGraph::TJobRunner runner;
            runner.SetTerminateOnExceptions(true);

            TTempQuasarTables formNameTables(c);
            auto formNameSort = runner.AddJob([&]() {
                SortAndSplitByQuasar(c, in, formNameTables, "FormName");
            });

            TTempQuasarTables durationTables(c);
            auto durationSort = runner.AddJob([&]() {
                SortAndSplitByQuasar(c, in, durationTables, "UniproxyDuration");
            });

            TTempQuasarTables quantileTables(c);
            auto startTime = ParseTime(in);
            runner.AddJob([&]() {
                CalculateQuantiles(
                    c,
                    formNameTables.All.Name(),
                    durationTables.All.Name(),
                    quantileTables.All.Name(),
                    startTime,
                    "all",
                    quantiles,
                    opts
                );
            }, {formNameSort, durationSort});

            runner.AddJob([&]() {
                CalculateQuantiles(
                    c,
                    formNameTables.Quasar.Name(),
                    durationTables.Quasar.Name(),
                    quantileTables.Quasar.Name(),
                    startTime,
                    "quasar",
                    quantiles,
                    opts
                );
            }, {formNameSort, durationSort});

            runner.AddJob([&]() {
                CalculateQuantiles(
                    c,
                    formNameTables.NoQuasar.Name(),
                    durationTables.NoQuasar.Name(),
                    quantileTables.NoQuasar.Name(),
                    startTime,
                    "no_quasar",
                    quantiles,
                    opts
                );
            }, {formNameSort, durationSort});

            TTempTable externalSkills(c), internalSkills(c);
            runner.AddJob([&]() {
                TTempQuasarTables externalTables(c), internalTables(c);
                auto isExternalSkill = [] (const auto& row) {
                    return row.GetFormName().Contains("external_skill");
                };
                TString sortField = "UniproxyDuration";
                Split<TBassDurationStatsRow>(
                    c, durationTables.All.Name(),
                    NYT::TRichYPath(externalTables.All.Name()).SortedBy({sortField}),
                    NYT::TRichYPath(internalTables.All.Name()).SortedBy({sortField}),
                    isExternalSkill
                );
                Split<TBassDurationStatsRow>(
                    c, durationTables.Quasar.Name(),
                    NYT::TRichYPath(externalTables.Quasar.Name()).SortedBy({sortField}),
                    NYT::TRichYPath(internalTables.Quasar.Name()).SortedBy({sortField}),
                    isExternalSkill
                );
                Split<TBassDurationStatsRow>(
                    c, durationTables.NoQuasar.Name(),
                    NYT::TRichYPath(externalTables.NoQuasar.Name()).SortedBy({sortField}),
                    NYT::TRichYPath(internalTables.NoQuasar.Name()).SortedBy({sortField}),
                    isExternalSkill
                );
                CalculateSkillQuantiles(
                    c, durationTables, externalTables, externalSkills.Name(),
                    startTime, "total_external_skills", quantiles
                );
                CalculateSkillQuantiles(
                    c, durationTables, internalTables, internalSkills.Name(),
                    startTime, "total_internal_skills", quantiles
                );
            }, {durationSort});
            runner.Run();

            {
                TMergeOperationSpec spec;
                spec.AddInput(quantileTables.All.Name())
                    .AddInput(quantileTables.Quasar.Name())
                    .AddInput(quantileTables.NoQuasar.Name())
                    .AddInput(externalSkills.Name())
                    .AddInput(internalSkills.Name())
                    .Output(out);
                c->Merge(spec);
            }
            {
                TSortOperationSpec spec;
                spec.AddInput(out).Output(out).SortBy({"percentage", "tf"});
                c->Sort(spec);
            }
        });
    return 0;
}

int RunMergeConvert(int argc, const char** argv) {
    const auto opts = TCommonOpts::Parse(argc, argv);

    auto client = NYT::CreateClient(opts.Server);

    MapTables<TBassLogRow>(client, opts.Input, opts.Output, false, true, opts.Count,
        [&opts](IClientBasePtr c, const TYPath& in, const TYPath& out) {
            {
                TMergeOperationSpec spec;
                spec.AddInput(out).Output(out);
                AddInputsNoSchema<TMergeOperationSpec>(spec, opts.Count, 5, in);
                c->Merge(spec);
            }
        });

    return 0;
}

int RunMergeBrief(int argc, const char** argv) {
    const auto opts = TCommonOpts::Parse(argc, argv);

    auto client = NYT::CreateClient(opts.Server);

    MapTables<TBassBriefLogRow>(client, opts.Input, opts.Output, false, true, opts.Count,
        [&opts, &client](IClientBasePtr c, const TYPath& in, const TYPath& out) {
            {
                TMergeOperationSpec spec;
                spec.Output(out).MergeBy({"StartTime", "ReqId"}).Mode(EMergeMode::MM_SORTED);
                if (client->Exists(out)) {
                    spec.AddInput(out);
                }
                AddInputsNoSchema<TMergeOperationSpec>(spec, opts.Count, 5, in);
                c->Merge(spec);
            }
        });

    return 0;
}

class TNogeoMapper : public IMapper<TTableReader<TBassLogRow>, TTableWriter<TBassNogeoLogRow>> {
public:
    void Do(TTableReader<TBassLogRow>* reader, TTableWriter<TBassNogeoLogRow>* writer) override {
        for (; reader->IsValid(); reader->Next()) {
            const auto& in = reader->GetRow();
            TBassNogeoLogRow row;

            if (in.GetReqId().empty()) {
                continue;
            }

            TString message = in.GetMessage();
            row.SetMessage(message);
            NSc::TValue v = NSc::TValue::FromJson(in.GetJson());
            if (v.IsNull()) {
                continue;
            }
            const auto form = v["form"];
            for (const auto& slot: form["slots"].GetArray()) {
                if (slot["name"].GetString() == "where" && slot["type"].GetString() == "string") {
                    row.SetWhere(TString(slot["source_text"]));
                }
            }

            if (message == "request") {
                if (form["name"] != "personal_assistant.scenarios.get_weather") {
                    continue;
                }
                const auto meta = v["meta"];
                row.SetReqId(TString(meta["request_id"]));
                row.SetUtterance(TString(meta["utterance"]));
                writer->AddRow(row);
            } else if (message == "form response") {
                row.SetReqId(in.GetReqId());
                bool hasNogeo = false;
                for (const auto& block: v["blocks"].GetArray()) {
                    if (block["type"] == "error" && block["error"]["type"] == "nogeo") {
                        hasNogeo = true;
                        break;
                    }
                }
                if (hasNogeo) {
                    writer->AddRow(row);
                }
            }
        }
    }
};
REGISTER_MAPPER(TNogeoMapper);

class TNogeoReducer : public IReducer<TTableReader<TBassNogeoLogRow>, TTableWriter<TBassNogeoLogRow>> {
public:
    void Do(TTableReader<TBassNogeoLogRow>* reader, TTableWriter<TBassNogeoLogRow>* writer) override {
        TBassNogeoLogRow row;
        bool hasRequest = false;
        bool hasFormResponse = false;

        for (; reader->IsValid(); reader->Next()) {
            const auto& in = reader->GetRow();
            if (in.GetMessage() == "request") {
                hasRequest = true;
                row.SetUtterance(in.GetUtterance());
                row.SetWhere(in.GetWhere());
            } else if (in.GetMessage() == "form response") {
                hasFormResponse = true;
            }
            row.SetReqId(in.GetReqId());
        }

        if (hasRequest && hasFormResponse) {
            writer->AddRow(row);
        }
    }
};
REGISTER_REDUCER(TNogeoReducer);

class TNogeoStatReducer : public IReducer<TTableReader<TBassNogeoLogRow>, TTableWriter<TBassNogeoStatLogRow>> {
public:
    void Do(TTableReader<TBassNogeoLogRow>* reader, TTableWriter<TBassNogeoStatLogRow>* writer) override {
        TBassNogeoStatLogRow row;
        int count = 0;
        TMap<TString, int> cntUtterances;

        for (; reader->IsValid(); reader->Next()) {
            const auto& in = reader->GetRow();
            ++count;
            ++cntUtterances[TString(in.GetUtterance())];
            row.SetWhere(in.GetWhere());
        }

        TVector<std::pair<int, TString>> topUtterances;
        for (const auto& [key, value] : cntUtterances) {
            topUtterances.emplace_back(-value, key);
        }
        sort(topUtterances.begin(), topUtterances.end());

        TString topUtterance;
        for (int i = 0; i < std::min(5, (int)topUtterances.size()); ++i) {
            if (i) {
                topUtterance += "; ";
            }
            topUtterance += NSc::TValue(-topUtterances[i].first).ForceString();
            topUtterance += ": " + topUtterances[i].second;
        }

        row.SetCount(count);
        row.SetTopUtterance(topUtterance);

        writer->AddRow(row);
    }
};
REGISTER_REDUCER(TNogeoStatReducer);


int RunNogeo(int argc, const char** argv) {
    const auto opts = TCommonOpts::Parse(argc, argv);
    auto client = NYT::CreateClient(opts.Server);

    MapTables<TBassNogeoStatLogRow>(client, opts.Input, opts.Output, opts.Force, opts.Append, opts.Count,
        [&opts](IClientBasePtr c, const TYPath& in, const TYPath& out) {
            TTempTable tmp(c), tmp2(c);
            {
                auto spec = NAliceYT::DefaultMapOperationSpec(opts.MemoryLimitMB);
                AddInputsSchema<TBassLegacyLogRow, TMapOperationSpec>(spec, opts.Count, 24*60, in, true);
                spec.AddOutput<TBassLogRow>(tmp.Name())
                    .Ordered(true);
                c->Map(spec, new TConvertLogsMapper);
            }
            {
                auto spec = NAliceYT::DefaultMapReduceOperationSpec(opts.MemoryLimitMB);
                spec.AddInput<TBassLogRow>(tmp.Name())
                    .ReduceBy("ReqId")
                    .SortBy("ReqId")
                    .AddOutput<TBassNogeoLogRow>(tmp2.Name());
                c->MapReduce(spec, new TNogeoMapper, new TNogeoReducer);
            }
            {
                auto spec = NAliceYT::DefaultMapReduceOperationSpec(opts.MemoryLimitMB);
                spec.AddInput<TBassNogeoLogRow>(tmp2.Name())
                    .ReduceBy("Where")
                    .SortBy("Where")
                    .AddOutput<TBassNogeoStatLogRow>(out);
                c->MapReduce(spec, nullptr, new TNogeoStatReducer);
            }
            {
                TSortOperationSpec spec;
                spec.AddInput(out).Output(out).SortBy({"Count", "Where"});
                c->Sort(spec);
            }
        });
    return 0;
}

int main(int argc, const char** argv) {
    NYT::Initialize(argc, argv);
    NYT::SetLogger(NYT::CreateStdErrLogger(NYT::ILogger::INFO));
    NYT::TConfig::Get()->UseClientProtobuf = false;

    TModChooser mc;
    mc.AddMode("convert", RunConvert, "convert old bass logs to new structured format");
    mc.AddMode("brief", RunBrief, "convert new bass logs to new brief format");
    mc.AddMode("uniproxy", RunUniproxy, "make uniproxy timing table from brief and uniproxy logs");
    mc.AddMode("sort-by-reqid", RunSort, "sort new logs by reqid");
    mc.AddMode("errors-stats", RunErrorsStats, "make table with errors and logs stats");
    mc.AddMode("quantile-stats", RunQuantileStats, "make table with quantile stats");
    mc.AddMode("merge-convert", RunMergeConvert, "merge convert input table to output table");
    mc.AddMode("merge-brief", RunMergeBrief, "merge brief input table to output table");
    mc.AddMode("merge-uniproxy-brief", RunMergeUniproxyAndBrief, "merge brief and uniproxy input table to output table");
    mc.AddMode("nogeo", RunNogeo, "nogeo weather info with 'where' slot and top utterances");
    try {
        return mc.Run(argc, argv);
    } catch (yexception& e) {
        LOG(ERR) << e.what() << Endl;
        return 1;
    }
}
