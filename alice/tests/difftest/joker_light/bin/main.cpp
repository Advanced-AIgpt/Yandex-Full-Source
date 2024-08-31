#include <alice/joker/library/log/log.h>
#include <alice/tests/difftest/joker_light/library/core/server.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/getopt/small/modchooser.h>
#include <library/cpp/sighandler/async_signals_handler.h>

using namespace NAlice::NJokerLight;

namespace {

int Server(int argc, const char** argv) {
    NLastGetopt::TOpts opts;
    opts.SetFreeArgsNum(1);
    opts.SetFreeArgTitle(0, "<config.json>", "configuration file in json");
    NLastGetopt::TOptsParseResult res{&opts, argc, argv};

    TString configFileName{res.GetFreeArgs()[0]};

    TContext ctx{std::move(configFileName)};
    TServer server{ctx};

    LOG(INFO) << "Config: " << ctx.Config().ToJson() << Endl;

    auto shutdown = [&server](int) {
        server.Shutdown();
    };

    SetAsyncSignalFunction(SIGTERM, shutdown);
    SetAsyncSignalFunction(SIGINT, shutdown);

    if (!server.Start()) {
        LOG(INFO) << "Can't start server, check that the port is free" << Endl;
    }
    server.Wait();

    SetAsyncSignalFunction(SIGTERM, nullptr);
    SetAsyncSignalFunction(SIGINT, nullptr);
    return EXIT_SUCCESS;
}

} // namespace

int main(int argc, const char** argv) {
    TLogging::InitTlsUniqId();

    DoInitGlobalLog("console", TLOG_DEBUG, /* rotation = */ false, /* startAsDaemon = */ false);

    TModChooser modChooser;
    modChooser.SetDescription("Joker mocker YDB-only version.");
    modChooser.AddMode("server", Server, "run server");

    return modChooser.Run(argc, argv);
}
