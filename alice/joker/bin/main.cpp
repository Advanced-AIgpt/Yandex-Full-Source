#include <alice/joker/library/core/config.h>
#include <alice/joker/library/core/globalctx.h>
#include <alice/joker/library/core/server.h>
#include <alice/joker/library/core/ctrl_session.h>
#include <alice/joker/library/log/log.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/getopt/small/modchooser.h>
#include <library/cpp/sighandler/async_signals_handler.h>

#include <util/datetime/base.h>
#include <library/cpp/deprecated/atomic/atomic.h>

using namespace NAlice::NJoker;

template <> TMaybe<ui16> FromString<TMaybe<ui16>>(const TStringBuf& v);
template <> TMaybe<TString> FromString<TMaybe<TString>>(const TStringBuf& v);
template <> TSessionId FromString<TSessionId>(const TStringBuf& v);

namespace {

int Server(int argc, const char** argv) {
    NLastGetopt::TOpts opts;
    opts.SetFreeArgsNum(1);
    opts.SetFreeArgTitle(0, "<config.json>", "configuration file in json");

    TMaybe<ui16> verbosity; // FIXME use common function
    opts.AddCharOption('v', "Verbose").Optional().NoArgument().Handler0([&verbosity]() {
                                                                          if (!verbosity.Defined())
                                                                              verbosity.ConstructInPlace(0);
                                                                          ++*verbosity;
                                                                          });

    NLastGetopt::TOptsParseResult res{&opts, argc, argv};
    TGlobalContext globalCtx{res.GetFreeArgs()[0]};

    LOG(DEBUG) << "Config: " << globalCtx.Config().ToJson() << Endl;

    TAtomic isShutdownInitiated{false};
    TServer server{globalCtx};

    auto shutdown = [&server, &isShutdownInitiated](int) {
        AtomicSet(isShutdownInitiated, true);
        server.Shutdown();
    };
    auto rotate = [](int) {
        // TODO implement it
        // not sure that its needed
    };

    SetAsyncSignalFunction(SIGTERM, shutdown);
    SetAsyncSignalFunction(SIGINT, shutdown);
    SetAsyncSignalFunction(SIGHUP, rotate);

    {
        constexpr size_t maxTries = 10;
        for (ui32 i = 0, total = maxTries; i < total && !server.Start() && !AtomicGet(isShutdownInitiated); ++i) {
            WARNING_LOG << "Http server is starting..." << Endl;
            Sleep(TDuration::Seconds(1));
        }
        // FIXME check if server is really started
        DEBUG_LOG << "Http server is listening on " << globalCtx.Config().Server().HttpPort() << Endl;
    }

    server.Wait();

    INFO_LOG << "Http server is stoped" << Endl;

    SetAsyncSignalFunction(SIGTERM, nullptr);
    SetAsyncSignalFunction(SIGINT, nullptr);
    SetAsyncSignalFunction(SIGHUP, nullptr);

    return EXIT_SUCCESS;
}

int SessionInfo(TGlobalContext& /*globalCtx*/, TSessionControl& session) {
    if (const auto e = session.OutTo(Cout)) {
        LOG(ERROR) << e << Endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int SessionClear(TGlobalContext& /*globalCtx*/, TSessionControl& session) {
    if (const auto e = session.Clear()) {
        LOG(ERROR) << e << Endl;
        return 2;
    }

    return EXIT_SUCCESS;
}

int SessionPush(TGlobalContext& globalCtx, TSessionControl& session) {
    if (session.Push(globalCtx)) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int Session(int argc, const char** argv) {
    NLastGetopt::TOpts opts;
    opts.SetFreeArgsNum(2);
    opts.SetFreeArgTitle(0, "<action>", "what to do with session (info|upload|clear)");
    opts.SetFreeArgTitle(1, "<path/to/config>", "configuration file in json used when serevr is run");

    TString sessionId;
    opts.AddLongOption("id").Help("Session Id").Optional().StoreResult(&sessionId);

    NLastGetopt::TOptsParseResult res{&opts, argc, argv};
    static const THashMap<TStringBuf, std::function<int(TGlobalContext&, TSessionControl&)>> handlers = {
        { "info", &SessionInfo },
        { "clear", &SessionClear },
        { "push", &SessionPush },
    };

    const auto* handler = handlers.FindPtr(res.GetFreeArgs()[0]);
    if (!handler) {
        LOG(ERROR) << "Unsupported action " << res.GetFreeArgs()[0].Quote() << Endl;
        res.HandleError();
        return EXIT_FAILURE;
    }

    TGlobalContext globalCtx{res.GetFreeArgs()[1]};

    THolder<TSessionControl> session;
    if (const TStatus error = TSessionControl::Load(TSessionId{sessionId}, globalCtx.Config(), session)) {
        LOG(ERROR) << "No " << sessionId.Quote() << " session found: " << error << Endl;
        return EXIT_FAILURE;
    }

    return (*handler)(globalCtx, *session);
}

} // namespace

int main(int argc, const char** argv) {
    TLogging::InitTlsUniqId();

    DoInitGlobalLog("console", TLOG_DEBUG, false /* rotation */, false /* startAsDaemon */);

    TModChooser modChooser;
    modChooser.SetDescription("Tool for running and managing joker mocker.");
    modChooser.AddMode("server", Server, "run joker mocker server");
    modChooser.AddMode("session", Session, "cmd line access to session info");

    return modChooser.Run(argc, argv);
}

template <>
TMaybe<ui16> FromString<TMaybe<ui16>>(const TStringBuf& v) {
    return FromString<ui16>(v);
}

template <>
TMaybe<TString> FromString<TMaybe<TString>>(const TStringBuf& v) {
    return TString{v};
}

template <>
TSessionId FromString<TSessionId>(const TStringBuf& v) {
    return TSessionId{TString{v}};
}
