#include <saas/api/indexing_client/client.h>
#include <saas/api/mr_client/client.h>
#include <saas/api/indexing_client/sloth.h>
#include <saas/util/logging/tskv_log.h>
#include <saas/util/queue.h>

#include <mapreduce/lib/init.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/logger/global/global.h>
#include <library/cpp/json/json_reader.h>

#include <util/stream/file.h>
#include <library/cpp/deprecated/atomic/atomic_ops.h>
#include <util/random/random.h>
#include <util/charset/wide.h>
#include <util/digest/city.h>
#include <util/string/cast.h>

class IBuilderAgent {
public:
    virtual ~IBuilderAgent() {}
    using TPtr = TAtomicSharedPtr<IBuilderAgent>;

    virtual TAtomicSharedPtr<NSaas::TAction> Build() const = 0;
};

class TTsvFileBuilder: public IBuilderAgent {
public:
    TTsvFileBuilder(const TString& fileName)
        : Input(fileName)
    {
    }

    TAtomicSharedPtr<NSaas::TAction> Build() const override {
        TUtf16String line;
        {
            TGuard<TMutex> g(Mutex);
            if (!Input.ReadLine(line)) {
                return nullptr;
            }
        }
        TAtomicSharedPtr<NSaas::TAction> action = new NSaas::TAction();
        action->SetActionType(NSaas::TAction::atModify);
        action->SetPrefix(1);

        auto &doc = action->GetDocument();
        doc.SetRealtime(true);
        doc.SetCheckOnlyBeforeReply(false);
        doc.SetMimeType("text/html");
        doc.SetLang("ru");
        doc.SetLangDef("ru");

        TString reply = WideToUTF8(line);
        TString url = ToString(CityHash64(reply));
        doc.SetUrl(url);
        doc.SetBody(reply);
        return action;
    }

private:
    mutable TFileInput Input;
    mutable TMutex Mutex;
};

struct TSenderAgentOption {
    ui32 ReindexAttemptions = 1;
    TDuration MinSleepDuration = TDuration::MilliSeconds(10);
    TDuration MaxSleepDuration = TDuration::Seconds(10);
};

class TSenderAgent: public IObjectInQueue {
private:
    IBuilderAgent::TPtr Builder;
    NSaas::TIndexingClient& Client;
    TSenderAgentOption Option;
public:

    TSenderAgent(NSaas::TIndexingClient& client, IBuilderAgent::TPtr builder, const TSenderAgentOption &option)
        : Builder(builder)
        , Client(client)
        , Option(option)
    {
    }

    void Process(void* /*ThreadSpecificResource*/) override {
        TAtomicSharedPtr<NSaas::TAction> action;
        // We start with a zero sleep between requests, then increase it on each failure and decrease on success.
        // The requests-per-seconds rate is expected to converge around the indexer proxy throughput.
        TSlothExp sloth(TDuration::Zero(), Option.MinSleepDuration, Option.MaxSleepDuration);
        while (!!(action = Builder->Build())) {
            for (ui32 i = 0; i < 1 + Option.ReindexAttemptions; ++i) {  // one initial attempt + number of retries
                TDuration sleepedFor = sloth.Sleep();
                DEBUG_LOG << "sleeped for " << sleepedFor.MilliSeconds() << "ms" << Endl;
                NSaas::TSendResult sendResult = Client.Send(*action);
                DEBUG_LOG << sendResult.GetMessage() << Endl;
                if (sendResult.GetCode() == NSaas::TSendResult::srOK) {
                    sloth.DecDuration();
                    break;
                }
                sloth.IncDuration();
            }
        }
    }
};

void RunLocal(const NSaas::TSaasIndexerOption& ctx, const TString& fileName) {
    IBuilderAgent::TPtr builderAgent = new TTsvFileBuilder(fileName);
    TRTYMtpQueue queue;
    queue.Start(ctx.Threads);
    NSaas::TIndexingClient client(ctx.Host, ctx.Port, ctx.GetIndexUrl());

    TSenderAgentOption senderOpt { ctx.ReindexAttemptions };
    for (ui32 i = 0; i < ctx.Threads; ++i) {
        queue.SafeAddAndOwn(THolder(new TSenderAgent(client, builderAgent, senderOpt)));
    }
    queue.Stop();
}

void DropIndex(const NSaas::TSaasIndexerOption& ctx) {
    NSaas::TIndexingClient client(ctx.Host, ctx.Port, ctx.GetIndexUrl());

    NSaas::TAction action;
    action.SetActionType(NSaas::TAction::atDelete);
    action.SetPrefix(1);

    auto &doc = action.GetDocument();
    doc.SetRealtime(true);
    doc.SetUrl("*");
    doc.SetBody("query_del:$remove_all$");

    for (;;) {
        NSaas::TSendResult sendResult = client.Send(action);
        DEBUG_LOG << sendResult.GetMessage() << Endl;
        if (sendResult.GetCode() == NSaas::TSendResult::srOK) {
            DEBUG_LOG << "Indexed dropped!" << Endl;
            break;
        }
    }
}

int main(int argc, const char **argv) {
    NMR::Initialize(argc, argv);

    NLastGetopt::TOpts opts;
    opts.AddHelpOption();

    NSaas::TSaasIndexerOption ctx;
    TString fileName;
    opts.AddCharOption('h', "host").StoreResult(&ctx.Host).DefaultValue("saas-indexerproxy-prestable.yandex.net");
    opts.AddCharOption('p', "port").StoreResult(&ctx.Port).DefaultValue("80");
    opts.AddCharOption('k', "indexkey").StoreResult(&ctx.IndexKey).Required();
    opts.AddCharOption('f', "ammo filename").StoreResult(&fileName).Required();
    opts.AddCharOption('l', "log level").StoreResult(&ctx.LogLevel).DefaultValue("6");
    opts.AddCharOption('r', "number of reindex attemptions").StoreResult(&ctx.ReindexAttemptions).DefaultValue("1");
    opts.AddLongOption("prefix").StoreResult(&ctx.ProcessorOptions.Prefix);
    opts.AddLongOption("action").StoreResult(&ctx.ProcessorOptions.ActionType);

    opts.AddCharOption('t', "subqueue threads count").StoreResult(&ctx.Threads);
    opts.AddLongOption("queue-size", "subqueue max size").StoreResult(&ctx.QueueSize);
    opts.AddLongOption("compression").StoreResult(&ctx.ProcessorOptions.Compression).DefaultValue("none");
    opts.AddLongOption("rty_adapter").StoreResult(&ctx.RtyIndexAdapter).DefaultValue("json");
    opts.AddLongOption("timestamp").StoreResult(&ctx.ProcessorOptions.Timestamp);

    NLastGetopt::TOptsParseResult parseOpt(&opts, argc, argv);
    DoInitGlobalLog("console", ctx.LogLevel, false, false);

    if (!parseOpt.Has("timestamp")) {
        ctx.ProcessorOptions.Timestamp = TInstant::Now().Seconds();
    }

    if (fileName == "drop_index") {
        DropIndex(ctx);
    } else {
        RunLocal(ctx, fileName);
    }

    return 0;
}
