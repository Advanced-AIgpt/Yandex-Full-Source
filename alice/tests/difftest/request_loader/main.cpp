#include "request_filters.h"

#include <alice/megamind/protos/speechkit/request.pb.h>
#include <alice/rtlog/rthub/protos/megamind_log.pb.h>
#include <library/cpp/getopt/last_getopt.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/logger/global/global.h>
#include <library/cpp/protobuf/json/json2proto.h>

#include <mapreduce/yt/interface/client.h>

#include <util/generic/guid.h>
#include <util/generic/string.h>
#include <util/stream/file.h>
#include <util/system/user.h>

using namespace NJson;
using namespace NProtobufJson;

const TString DEFAULT_SAVE_FOLDER = "output";
const int DEFAULT_LIMIT = 100;
const TString DEFAULT_TABLE_PATH = "//home/logfeller/logs/megamind-log/1d/2020-01-13";
const TString DEFAULT_FAKE_GUID_MARKER = "deadbeef";
const TString DEFAULT_CLUSTER = "hahn";

namespace NRequestsLoader {

struct TLoaderCtx {
    TString SaveFolder;
    TString Cluster;
    TString TablePath;
    TString FakeGuidMarker;
    THolder<TRequestFilter> RequestFilter;
    int Limit;
};

class TLoader {
public:
    explicit TLoader(TLoaderCtx ctx)
        : LoaderCtx_(std::move(ctx))
        , JsonProtoConfig_(TJson2ProtoConfig().SetUseJsonName(true))
        , JsonWriterConfig_(ConstructJsonWriterConfig())
    {
    }

    void Load() {
        INFO_LOG << "Load " << LoaderCtx_.Limit << " requests from table " << LoaderCtx_.TablePath << Endl;
        INFO_LOG << "Save to folder \"" << LoaderCtx_.SaveFolder << "\"" << Endl;
        TFsPath(LoaderCtx_.SaveFolder).MkDirs();

        GenerateSessionId();

        INFO_LOG << "Initialize YT" << Endl;
        NYT::JoblessInitialize();

        const auto client = NYT::CreateClient(LoaderCtx_.Cluster);
        const auto reader = client->CreateTableReader<NRTLog::TMegamindLog>(LoaderCtx_.TablePath);

        INFO_LOG << "Start reading table" << Endl;

        auto& filter = LoaderCtx_.RequestFilter;
        int requestNum = 0;
        int rowNum = 0;
        for (const auto& cursor : *reader) {
            ++rowNum;
            const auto& row = cursor.GetRow();
            const TString& message = row.message();

            TMaybe<TJsonValue> request = LogMessageToSpeechkitRequest(message);
            if (request.Defined()) {
                if (filter == nullptr || filter->Filter(request.GetRef())) {
                    // the request is correct, save it
                    ++requestNum;

                    INFO_LOG << "Got speechkit request #" << requestNum << " on row #" << rowNum << Endl;
                    DumpSpeechkitRequest(request.GetRef());

                    if (requestNum >= LoaderCtx_.Limit) {
                        break;
                    }
                }
            }
        }

        INFO_LOG << "Stopped reading table" << Endl;
    }

private:
    const TLoaderCtx LoaderCtx_;
    const TJson2ProtoConfig JsonProtoConfig_;
    const TJsonWriterConfig JsonWriterConfig_;

private:
    TString CreateFakeGuidAsString() const {
        TString guid = CreateGuidAsString();
        return guid.replace(0, LoaderCtx_.FakeGuidMarker.Size(), LoaderCtx_.FakeGuidMarker);
    }

    constexpr static TJsonWriterConfig ConstructJsonWriterConfig() {
        TJsonWriterConfig config;
        config.FormatOutput = true;
        config.SortKeys = true;
        return config;
    }

    TMaybe<TJsonValue> LogMessageToSpeechkitRequest(const TString& message) {
        try {
            // validate that this is SpeechKitRequest
            NAlice::TSpeechKitRequestProto requestProto;
            Json2Proto(message, requestProto, JsonProtoConfig_);

            // read to common json holder
            TJsonValue request;
            TStringStream ss(message);
            ReadJsonTree(&ss, &request);

            return request;
        } catch (...) {
            return Nothing();
        }
    }

    void RedefineSpeechkitRequestSecrets(TJsonValue& request) {
        auto& header = request["header"];
        if (header.Has("prev_req_id") || header.Has("sequence_number")) {
            header["prev_req_id"] = CreateFakeGuidAsString();
        }
        header["request_id"] = CreateFakeGuidAsString();

        auto& addOpts = request["request"]["additional_options"];
        addOpts.EraseValue("oauth_token"); // top secret, to be defined in sender application
        addOpts.EraseValue("bass_url");
    }

    TString SpeechkitRequestToJsonString(const TJsonValue& request) {
        TStringStream ss;
        WriteJson(&ss, &request, JsonWriterConfig_);
        return ss.Str();
    }

    void DumpSpeechkitRequest(TJsonValue& request) {
        RedefineSpeechkitRequestSecrets(request);
        TString dump = SpeechkitRequestToJsonString(request);

        TFsPath requestPath = JoinFsPaths(LoaderCtx_.SaveFolder, request["header"]["request_id"].GetString() + ".json");
        try {
            TFile file(requestPath, CreateAlways | WrOnly);
            file.Write(dump.data(), dump.Size());
            file.Close();
        } catch (...) {
            ERROR_LOG << "Unsuccessful saving SpeechkitRequest to " << requestPath.GetPath() << Endl;
        }
    }

    void GenerateSessionId() {
        TString guid = CreateGuidAsString();
        INFO_LOG << "New session id is: " << guid << Endl;

        TFsPath sessionIdPath = JoinFsPaths(LoaderCtx_.SaveFolder, "session_id.txt");
        try {
            TFile file(sessionIdPath, CreateAlways | WrOnly);
            TFileOutput out(file);
            out << guid << Endl;
        } catch (...) {
            ERROR_LOG << "Unsuccessful saving session id to " << sessionIdPath.GetPath() << Endl;
        }
    }
};

} // namespace NRequestsLoader

int main(int argc, char* argv[]) {
    NRequestsLoader::TLoaderCtx ctx;
    NLastGetopt::TOpts opts;
    opts.AddHelpOption('h');
    opts.AddLongOption("save-folder").Help("Folder to save requests").DefaultValue(DEFAULT_SAVE_FOLDER).StoreResult(&ctx.SaveFolder);
    opts.AddLongOption("guid-marker").Help("Fake guid marker").DefaultValue(DEFAULT_FAKE_GUID_MARKER).StoreResult(&ctx.FakeGuidMarker);
    opts.AddLongOption("limit").Help("Maximal count of requests").DefaultValue(DEFAULT_LIMIT).StoreResult(&ctx.Limit);
    opts.AddLongOption("table-path").Help("YT table path").DefaultValue(DEFAULT_TABLE_PATH).StoreResult(&ctx.TablePath);
    opts.AddLongOption("cluster").Help("YT table cluster").DefaultValue(DEFAULT_CLUSTER).StoreResult(&ctx.Cluster);
    opts.AddLongOption("filter").Help("Enabled requests filter").StoreMappedResultT<TStringBuf>(&ctx.RequestFilter, &NRequestsLoader::BuildRequestFilter);

    NLastGetopt::TOptsParseResult{&opts, argc, argv};
    NRequestsLoader::TLoader(std::move(ctx)).Load();

    return EXIT_SUCCESS;
}
