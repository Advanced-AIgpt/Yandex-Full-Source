#include <alice/bass/libs/fetcher/neh.h>
#include <alice/bass/libs/video_common/video_url_getter.h>

#include <alice/library/video_common/defs.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/getopt/modchooser.h>
#include <library/cpp/scheme/scheme.h>
#include <library/cpp/streams/factory/factory.h>
#include <library/cpp/threading/blocking_queue/blocking_queue.h>
#include <library/cpp/threading/future/async.h>
#include <library/cpp/threading/future/future.h>

#include <util/string/builder.h>
#include <util/string/join.h>
#include <util/thread/pool.h>

using namespace NAlice::NVideoCommon;

namespace {

struct TRequestContext {
    ui64 Id = 0;
    NSc::TValue VinsAnswer;
    TString Request;
    TVector<TString> Urls;
    TString ErrorText;
};

using TRequestsQueue = NThreading::TBlockingQueue<TRequestContext>;

class TVinsResultReader {
public:
    TVinsResultReader(IInputStream& requestsInput, IInputStream& vinsResponsesInput)
        : RequestsInput(requestsInput)
        , VinsResponsesInput(vinsResponsesInput)
    {
    }

    bool ReadNext(TString* request, NSc::TValue* vinsAnswer) {
        TString requestStr, vinsAnswerStr;
        if (RequestsInput.ReadLine(requestStr) && VinsResponsesInput.ReadLine(vinsAnswerStr)) {
            *request = requestStr;
            *vinsAnswer = NSc::TValue::FromJson(vinsAnswerStr);
        }
        return false;
    }

private:
    IInputStream& RequestsInput;
    IInputStream& VinsResponsesInput;
};

class TInputThread {
public:
    TInputThread(IInputStream& inputRequests, IInputStream& inputVins, TRequestsQueue& inputQueue)
        : Reader(inputRequests, inputVins)
        , InputQueue(inputQueue)
        , Id(0)
    {
    }

    bool ProcessLine() {
        TRequestContext ctx;
        if (Reader.ReadNext(&ctx.Request, &ctx.VinsAnswer)) {
            ctx.Id = Id++;
            InputQueue.Push(std::move(ctx));
            return true;
        }
        return false;
    }

    void Run() {
        try {
            while (ProcessLine()) {
            }
        } catch (...) {
            InputQueue.Stop();
            throw;
        }
        InputQueue.Stop();
    }

private:
    TVinsResultReader Reader;
    TRequestsQueue& InputQueue;
    ui64 Id;
};

class TWorkerThread {
public:
    TWorkerThread(TRequestsQueue& inputQueue, TRequestsQueue& resultsQueue, TRequestsQueue& errorsQueue)
        : InputQueue(inputQueue)
        , ResultsQueue(resultsQueue)
        , ErrorsQueue(errorsQueue)
        , VideoUrlGetter(NVideoCommon::TVideoUrlGetter::TParams{}) {
    }

    void Run() {
        while (TMaybe<TRequestContext> ctx = InputQueue.Pop()) {
            try {
                ProcessRequest(*ctx);

                if (ctx->Urls.empty()) {
                    throw yexception() << "Empty search result";
                }
                ResultsQueue.Push(std::move(*ctx));
            } catch (yexception& e) {
                ctx->ErrorText = e.what();
                ErrorsQueue.Push(std::move(*ctx));
            }
        }
        ResultsQueue.Stop();
        ErrorsQueue.Stop();
    }

private:
    void ProcessRequest(TRequestContext& ctx) {
        THashSet<TString> directives;
        for (const NSc::TValue& directive : ctx.VinsAnswer["response"]["directives"].GetArray()) {
            if (directive.TrySelect(TStringBuf("type")) != TStringBuf("client_action")) {
                continue;
            }
            TStringBuf name = directive.TrySelect("name");
            if (name == COMMAND_SHOW_GALLERY) {
                for (const NSc::TValue& item : directive.TrySelect("payload").TrySelect("items").GetArray()) {
                    ctx.Urls.push_back(ExtractItemUrl(item));
                }
                return;
            }
            if (name == COMMAND_SHOW_DESCRIPTION || name == COMMAND_VIDEO_PLAY || name == COMMAND_OPEN_URI) {
                const NSc::TValue& item = directive.TrySelect("payload").TrySelect("item");
                if (item.IsDict()) {
                    ctx.Urls.push_back(ExtractItemUrl(item));
                }
                return;
            }
            directives.insert(TString{name});
        }
        throw yexception() << "No video directives found in response. Text response: '" << ctx.VinsAnswer["response"]["card"]["text"].GetString() << "', directives: " << JoinSeq(", ", directives);
    }

    TString ExtractItemUrl(const NSc::TValue& item) const {
        NVideoCommon::TVideoUrlGetter::TRequest request;

        request.ProviderName = item.TrySelect("provider_name").GetString();
        request.ProviderItemId = item.TrySelect("provider_item_id");
        request.Type = item.TrySelect("type");
        request.TvShowItemId = item.TrySelect("tv_show_item_id");

        const auto url = VideoUrlGetter.Get(request);
        if (!url)
            throw yexception() << "VideoUrlGetter can not get url";
        return *url;
    }

private:
    TRequestsQueue& InputQueue;
    TRequestsQueue& ResultsQueue;
    TRequestsQueue& ErrorsQueue;

    NVideoCommon::TVideoUrlGetter VideoUrlGetter;
};

class TOutputThread {
public:
    TOutputThread(TRequestsQueue& queue, IOutputStream& output)
        : Queue(queue)
        , Output(output)
    {
    }

    virtual ~TOutputThread() = default;

    void Run() {
        while (TMaybe<TRequestContext> ctx = Queue.Pop()) {
            Output << PrintLine(*ctx) << '\n';
        }
    }

protected:
    virtual TString PrintLine(const TRequestContext& ctx) = 0;

private:
    TRequestsQueue& Queue;
    IOutputStream& Output;
};

class TOutputResultsThread : public TOutputThread {
public:
    TOutputResultsThread(TRequestsQueue& queue, IOutputStream& output)
        : TOutputThread(queue, output)
    {
    }

    TString PrintLine(const TRequestContext& ctx) override {
        return TStringBuilder() << ctx.Request << ';' << JoinSeq(";", ctx.Urls);
    }
};

class TOutputErrorsThread : public TOutputThread {
public:
    TOutputErrorsThread(TRequestsQueue& queue, IOutputStream& output)
        : TOutputThread(queue, output)
    {
    }

    TString PrintLine(const TRequestContext& ctx) override {
        return TStringBuilder() << ctx.Id << ';' << ctx.Request << ';' << ctx.ErrorText;
    }
};

} // namespace anonymous

int ExtractVideoSearchResults(int argc, const char** argv) {
    NLastGetopt::TOpts opts;

    TString inputRequestsFile;
    opts.AddLongOption("input-requests", "Text requests one per line")
        .Required().RequiredArgument().StoreResult(&inputRequestsFile);

    TString inputVinsFile;
    opts.AddLongOption("input-vins", "VINS responses in json format (one json per line)")
        .Required().RequiredArgument().StoreResult(&inputVinsFile);

    TString outputResultsFile;
    opts.AddLongOption("output-results", "results in CSV format [request;url1;url2;...]")
        .Required().RequiredArgument().StoreResult(&outputResultsFile);

    TString outputErrorsFile;
    opts.AddLongOption("output-errors", "errors in CSV format [request;error]")
        .Required().RequiredArgument().StoreResult(&outputErrorsFile);

    ui32 threads;
    opts.AddLongOption('t', "threads", "number of worker threads")
        .DefaultValue("8").RequiredArgument().StoreResult(&threads);

    opts.AddHelpOption();

    NLastGetopt::TOptsParseResult optsRes(&opts, argc, argv);

    THolder<IInputStream> inputRequests = OpenInput(inputRequestsFile);
    THolder<IInputStream> inputVins = OpenInput(inputVinsFile);
    THolder<IOutputStream> outputResults = OpenOutput(outputResultsFile);
    THolder<IOutputStream> outputErrors = OpenOutput(outputErrorsFile);

    TAdaptiveThreadPool queue;
    queue.Start(0);

    TVector<NThreading::TFuture<void>> threadFutures;

    TRequestsQueue inputQueue(1000);
    TInputThread inputThread(*inputRequests, *inputVins, inputQueue);
    threadFutures.push_back(NThreading::Async([&inputThread] () { inputThread.Run(); }, queue));

    TRequestsQueue resultsQueue(1000);
    TRequestsQueue errorsQueue(1000);

    TDeque<TWorkerThread> workerThreads;
    for (ui32 i = 0; i < threads; ++i) {
        workerThreads.emplace_back(inputQueue, resultsQueue, errorsQueue);
        TWorkerThread* workerThread = &workerThreads.back();
        threadFutures.push_back(NThreading::Async([workerThread] () { workerThread->Run(); }, queue));
    }

    TOutputResultsThread outputResultsThread(resultsQueue, *outputResults);
    TOutputErrorsThread outputErrorsThread(errorsQueue, *outputErrors);
    threadFutures.push_back(NThreading::Async([&outputResultsThread] () { outputResultsThread.Run(); }, queue));
    threadFutures.push_back(NThreading::Async([&outputErrorsThread] () { outputErrorsThread.Run(); }, queue));

    NThreading::TFuture<void> future = NThreading::WaitExceptionOrAll(threadFutures);
    future.Wait();
    future.GetValue();

    return 0;
}


int main(int argc, const char** argv) {
    TModChooser modChooser;
    modChooser.AddMode("extract-video-search-results", ExtractVideoSearchResults, "Join input requests with related VINS responses and make assessment bucket");
    return modChooser.Run(argc, argv);
}
