#include <alice/bass/tools/bass_logs/protos/logs.pb.h>
#include <alice/library/yt/protos/logs.pb.h>

#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/interface/logging/logger.h>
#include <mapreduce/yt/library/operation_tracker/operation_tracker.h>
#include <mapreduce/yt/util/temp_table.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/timezone_conversion/civil.h>
#include <library/cpp/timezone_conversion/convert.h>

#include <util/generic/string.h>
#include <util/stream/output.h>
#include <util/system/user.h>


using namespace NYT;


class TBassMapper
    : public IMapper<TTableReader<TNode>, TTableWriter<TNode>>
{
public:
    TBassMapper() = default;

    TBassMapper(TString id)
        : Id_(std::move(id))
    { }


    void Do(TReader* reader, TWriter* writer) override {
        for (; reader->IsValid(); reader->Next()) {
            const auto& in = reader->GetRow();
            const auto& id = in["ReqId"];
            TNode row;
            if (id == Id_) {
                row["type"] = "Bass Logs";
                row["timestamp"] = ParseTime(in["Timestamp"].AsString());
                row["content"] = in["Content"];
                row["reqId"] = id;
                writer->AddRow(row);
            }
        }
    }

    Y_SAVELOAD_JOB(Id_);

private:
   TString ParseTime(TStringBuf str) {
       // удаляем из логов часть строки " + 0300", чтобы было удобно смотреть в таблицу
       // (в vins и uniproxy тоже время +0300, но это не написано, поэтому путает)
       return TString{str.Chop(6)};
   }

private:
    TString Id_;
};
REGISTER_MAPPER(TBassMapper);


class TVinsMapper
    : public IMapper<TTableReader<TNode>, TTableWriter<TNode>>
{
public:
    TVinsMapper() = default;

    TVinsMapper(TString id)
        : Id_(std::move(id))
    { }

    void Do(TReader* reader, TWriter* writer) override {
        for (; reader->IsValid(); reader->Next()) {
            const auto& in = reader->GetRow();
            if (in["fields"].IsMap() && in["fields"]["context"].IsMap() &&
                in["fields"]["context"]["request_id"].IsString())
            {
                const auto& id = in["fields"]["context"]["request_id"].AsString();
                TNode row;
                if (id == Id_) {
                    row["levelStr"] = in["levelStr"];
                    row["fields"] = in["fields"];
                    row["timestamp"] = ParseTime(in["timestamp"].AsString());
                    row["loggerName"] = in["loggerName"];
                    row["message"] = in["message"];
                    row["reqId"] = id;
                    row["type"] = "Vins Logs";
                    writer->AddRow(row);
                }
             }
        }
    }

    Y_SAVELOAD_JOB(Id_);

private:
    TString ParseTime(const TStringBuf &ts) {
        TInstant t = TInstant::ParseIso8601(ts);
        NDatetime::TTimeZone msk = NDatetime::GetTimeZone("Europe/Moscow");
        NDatetime::TCivilSecond cs  = NDatetime::Convert(t, msk);
        return NDatetime::Format("%Y-%m-%d %H:%M:%S", cs, msk);
    }

private:
    TString Id_;
};
REGISTER_MAPPER(TVinsMapper);


class TUniproxyMapper
    : public IMapper<TTableReader<TUniproxyLogRow>, TTableWriter<TNode>>
{
public:
    TUniproxyMapper() = default;

    TUniproxyMapper(TString id)
        : Id_(std::move(id))
    { }

    void Do(TReader* reader, TWriter* writer) override {
        for (; reader->IsValid(); reader->Next()) {
            const auto& in = reader->GetRow();
            if (in.GetQloudProject() != "voice-ext" ||
                in.GetQloudApplication() != "uniproxy") {
                continue;
            }
            TStringBuf messageBuf(in.GetMessage());
            TNode row;
            if (TryParseLogMessage(messageBuf, row)) {
                row["timestamp"] = ParseTime(in.GetTimestamp());
                row["levelStr"] = in.GetLevelStr();
                row["message"] = in.GetMessage();
                row["type"] = "Uniproxy Logs";
                writer->AddRow(row);
            }
        }
    }

    Y_SAVELOAD_JOB(Id_);

private:
    bool  TryParseLogMessage(TStringBuf messageBuf, TNode& row) {
        auto logType = messageBuf.NextTok(" ");
        if (logType == "SESSIONLOG:") {
            return CompareReqId(messageBuf, row);
        }
        return false;
    }

    bool CompareReqId(TStringBuf messageBuf, TNode& row) {
        NJson::TJsonValue message;
        if (!ReadJsonTree(messageBuf, &message, false)) {
            return false;
        }
        if (message.Has("Event")) {
            auto reqId = message["Event"]["event"]["payload"]["header"]["request_id"];
            return SetReqId(reqId.GetString(), row);
        }
        if (!message["Directive"].Has("Body")) {
            auto reqId = message["Directive"]["Body"]["header"]["request_id"];
            return SetReqId(reqId.GetString(), row);
        }
        if (message["Directive"]["directive"]["payload"].Has("header")) {
            auto reqId = message["Directive"]["directive"]["payload"]["header"]["request_id"];
            return SetReqId(reqId.GetString(), row);
        }
        if (message["Directive"]["directive"]["payload"].Has("RequestId")) {
            auto reqId = message["Directive"]["directive"]["payload"]["RequestId"];
            return SetReqId(reqId.GetString(), row);
        }
        auto reqId = message["Directive"]["directive"]["payload"]["ServerMessage"]["ClientMessage"]["LogData"]["RequestId"];
        return SetReqId(reqId.GetString(), row);
    }

    bool SetReqId(const TStringBuf &recievedId, TNode& row) {
        if (recievedId != Id_) {
            return false;
        }
        row["reqId"] = Id_;
        return true;
    }

    TString ParseTime(const TStringBuf &ts) {
        TInstant t = TInstant::ParseIso8601(ts);
        NDatetime::TTimeZone msk = NDatetime::GetTimeZone("Europe/Moscow");
        NDatetime::TCivilSecond cs  = NDatetime::Convert(t, msk);
        return NDatetime::Format("%Y-%m-%d %H:%M:%S.", cs, msk) + FromString<TString>(ts.After('.').Before('Z'));
    }

private:
    TString Id_;
};
REGISTER_MAPPER(TUniproxyMapper);


int main(int argc, const char** argv) {
    NYT::Initialize(argc, argv);
    NYT::SetLogger(NYT::CreateStdErrLogger(NYT::ILogger::INFO));


    NLastGetopt::TOpts opts;

    TString server;
    opts.AddLongOption('s', "server")
        .Required()
        .RequiredArgument()
        .StoreResult(&server);

    TString date;
    opts.AddLongOption('d', "date")
        .Required()
        .RequiredArgument()
        .StoreResult(&date);

    TString id;
    opts.AddLongOption('r', "req_id")
        .Required()
        .RequiredArgument()
        .StoreResult(&id);

    TString outputSorted;
    opts.AddLongOption('o', "output")
        .Required()
        .RequiredArgument()
        .StoreResult(&outputSorted);

    bool Force = false;
    opts.AddLongOption('f', "force", "Force rewrite destination tables")
        .NoArgument()
        .SetFlag(&Force);

    NLastGetopt::TOptsParseResult optsRes(&opts, argc, argv);


    auto client = CreateClient(server);

    TString inputTableBass = "//home/bass/logs/convert/1d/" + date;
    TString inputTableVins = "//logs/vins-qloud-runtime-log/1d/" + date;
    TString inputTableUniproxy = "//home/logfeller/logs/qloud-runtime-log/1d/" + date;

    TTempTable bassTable(client), vinsTable(client), uniproxyTable(client);


    TOperationTracker tracker;

    tracker.AddOperation(
        client->Map(
            TMapOperationSpec()
                .AddInput<TNode>(inputTableBass)
                .AddOutput<TNode>(bassTable.Name()),
            new TBassMapper(id),
            TOperationOptions().Wait(false)
        )
    );

    tracker.AddOperation(
        client->Map(
            TMapOperationSpec()
                .AddInput<TNode>(inputTableVins)
                .AddOutput<TNode>(vinsTable.Name()),
            new TVinsMapper(id),
            TOperationOptions().Wait(false)
        )
    );

    tracker.AddOperation(
        client->Map(
            TMapOperationSpec()
                .AddInput<TUniproxyLogRow>(inputTableUniproxy)
                .AddOutput<TNode>(uniproxyTable.Name()),
            new TUniproxyMapper(id),
            TOperationOptions().Wait(false)
        )
    );

    tracker.WaitAllCompleted();


    client->Sort(
        TSortOperationSpec()
            .AddInput(bassTable.Name())
            .AddInput(vinsTable.Name())
            .AddInput(uniproxyTable.Name())
            .Output(outputSorted)
            .SortBy({"timestamp"})
    );

    return 0;
}
