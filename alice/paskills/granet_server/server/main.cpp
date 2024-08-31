#include <util/stream/output.h>
#include <util/generic/ptr.h>

#include <alice/paskills/granet_server/library/json_log_backend.h>
#include <alice/paskills/granet_server/library/server.h>
#include <alice/paskills/granet_server/config/proto/config.pb.h>

#include <google/protobuf/text_format.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/logger/global/global.h>

#include <util/stream/file.h>
#include <util/stream/output.h>


int main(int argc, char** argv) {
    DoInitGlobalLog(MakeHolder<TJsonLogBackend>());
    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();
    TString configPath;
    int port = 0;
    opts.AddLongOption("config", "Config file")
        .Required()
        .RequiredArgument("PATH")
        .StoreResult(&configPath);
    opts.AddLongOption("port", "Override port from config")
        .StoreResult(&port);
    NLastGetopt::TOptsParseResult res(&opts, argc, argv);

    const TString protoString = TFileInput(configPath).ReadAll();
    NGranetServer::TGranetServerConfig config;
    const bool parseResult = google::protobuf::TextFormat::ParseFromString(protoString, &config);
    Y_ENSURE(parseResult, "Error parsing config");

    if (port > 0) {
        config.MutableHttpServer()->SetPort(port);
    }
    NGranetServer::TGranetServer server{config};
    server.Start();
    server.Wait();

    return 0;
}
