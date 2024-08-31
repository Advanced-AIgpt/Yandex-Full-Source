#include <alice/bass/libs/fetcher/neh.h>
#include <alice/bass/libs/fetcher/util.h>
#include <alice/library/network/headers.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/getopt/modchooser.h>
#include <library/cpp/neh/neh.h>
#include <library/cpp/scheme/scheme.h>
#include <library/cpp/streams/factory/factory.h>
#include <library/cpp/threading/blocking_queue/blocking_queue.h>
#include <library/cpp/threading/future/async.h>
#include <library/cpp/threading/future/future.h>

#include <util/folder/path.h>
#include <util/generic/vector.h>
#include <util/stream/file.h>
#include <util/string/builder.h>
#include <util/string/join.h>
#include <util/string/printf.h>
#include <util/string/split.h>
#include <util/thread/pool.h>

namespace {

constexpr TStringBuf DEFAULT_VINS_REQUEST = TStringBuf(R"""(
{
  "header": {
    "request_id": "ffffffff-ffff-ffff-ffff-ffffffffffff"
  },
  "request": {
     "device_state" : {
       "navigator": {
         "home": {
           "lat": 12.34,
           "lon": 56.78,
           "arrival_points": []
         }
       }
     },
     "additional_options": {
       "bass_options": {
         "client_ip": "55.77.88.44"
       }
     },
     "event": {
      "text": "",
      "type": "text_input"
    },
    "location": {
      "lat": 55.733771,
      "lon": 37.587937
    }
  },
  "application": {
    "lang": "ru-RU",
    "os_version": "5.0",
    "uuid": "ffffffffffffc8bb23c3d126604d9052",
    "platform": "android",
    "client_time": "20190522T151558",
    "timezone": "Europe/Moscow",
    "app_version": "1.2.3",
    "app_id": "com.yandex.vins.shooting"
  }
}
)""");

struct TRequestContext {
    ui64 Id = 0;
    TString RequestText;
    NHttpFetcher::THandle::TRef Handle;
};
using TRequestsQueue = NThreading::TBlockingQueue<TRequestContext>;

class TReader {
public:
    virtual ~TReader() = default;
    virtual NSc::TValue Parse(TStringBuf line) = 0;

    TReader(THolder<IInputStream> input)
        : Input(std::move(input))
    {
        Valid = static_cast<bool>(Input->ReadLine(Line));
    }

    bool IsValid() const {
        return Valid;
    }

    NSc::TValue ReadNext() {
        if (!Valid) {
            return NSc::TValue::Null();
        }
        NSc::TValue value = Parse(Line);
        Valid = static_cast<bool>(Input->ReadLine(Line));
        return value;
    }

private:
    TString Line;
    bool Valid;
    THolder<IInputStream> Input;
};

class TTextReader : public TReader {
public:
    TTextReader(THolder<IInputStream> input)
        : TReader(std::move(input))
        , DefaultRequest(NSc::TValue::FromJson(DEFAULT_VINS_REQUEST))
    {
    }

    NSc::TValue Parse(TStringBuf line) override {
        NSc::TValue request = DefaultRequest.Clone();
        request["request"]["event"]["text"] = line;
        return request;
    }

private:
    const NSc::TValue DefaultRequest;
};

class TJsonReader : public TReader {
public:
    TJsonReader(THolder<IInputStream> input)
        : TReader(std::move(input))
    {
    }

    NSc::TValue Parse(TStringBuf line) override {
        return NSc::TValue::FromJson(line);
    }
};

class IVinsResponseWriter {
public:
    virtual ~IVinsResponseWriter() = default;
    virtual void WriteSuccessfulResponse(TStringBuf request, TStringBuf response) = 0;
    virtual void WriteError(ui64 id, TStringBuf request, TStringBuf errorText, TStringBuf response) = 0;
    virtual void Flush() = 0;
};

class TSplittedVinsResponseWriter : public IVinsResponseWriter {
public:
    TSplittedVinsResponseWriter(TFsPath outDir)
    {
        outDir.MkDirs();
        OutputOkRequests.Reset(new TOFStream(outDir / "success.requests.txt"));
        OutputOkResponses.Reset(new TOFStream(outDir / "success.responses.json"));
        OutputErrors.Reset(new TOFStream(outDir / "errors.txt"));
    }

    void WriteSuccessfulResponse(TStringBuf request, TStringBuf response) override {
        *OutputOkRequests << request << '\n';
        *OutputOkResponses << NSc::TValue::FromJson(response).ToJson() << '\n';
    }

    void WriteError(ui64 id, TStringBuf request, TStringBuf errorText, TStringBuf response) override {
        *OutputErrors << id << '\t' << request << '\t' << errorText << '\t' << response << '\n';
    }

    void Flush() override {
        OutputOkRequests->Flush();
        OutputOkResponses->Flush();
        OutputErrors->Flush();
    }

private:
    THolder<IOutputStream> OutputOkRequests;
    THolder<IOutputStream> OutputOkResponses;
    THolder<IOutputStream> OutputErrors;
};

class TSimpleVinsResponseWriter : public IVinsResponseWriter {
public:
    TSimpleVinsResponseWriter(THolder<IOutputStream> output)
        : Output(std::move(output))
    {
    }

    void WriteSuccessfulResponse(TStringBuf request, TStringBuf response) override {
        *Output << "Request: " << request << "; Status: OK; Response:\n"
                << NSc::TValue::FromJson(response).ToJsonPretty() << Endl;
    }

    void WriteError(ui64 /*id*/, TStringBuf request, TStringBuf errorText, TStringBuf response) override {
        *Output << "Request: " << request << "; Status: ERROR; Reason:\n" << errorText;
        if (!response.empty())
            *Output << "\nResponse:\n" << response;
        *Output << Endl;
    }

    void Flush() override {
        Output->Flush();
    }

private:
    THolder<IOutputStream> Output;
};

class TExperimentArgParser {
public:
    explicit TExperimentArgParser(THashSet<TString>* experiments)
        : Experiments(experiments)
    {
    }

    void operator()(TStringBuf str) {
        Y_VERIFY(Experiments);
        while (str) {
            // Keep trailing commas to preserve base64 value.
            // So this string:
            //   "experiment1,experiment2,name3=base64value3,,,name4=base64value4,"
            // is parsed to:
            //   {"experiment1", "experiment2", "name1=base64value1,,", "name2=base64value2,"}
            const size_t delimiterEnd = str.find_first_not_of(',', str.find(','));
            const size_t delimiterLength = delimiterEnd < str.length() ? 1 : 0;
            const TStringBuf token = str.Head(delimiterEnd - delimiterLength);
            str.Skip(delimiterEnd);
            if (!token.empty()) {
                Experiments->emplace(token);
            }
        }
    }

private:
    THashSet<TString>* Experiments = nullptr;
};

THashMap<TStringBuf, TStringBuf> VINS_URLS = {
        { "experiments",      "http://experiments.vins-int.dev.voicetech.yandex.net/speechkit/app/pa/" },
        { "hamster",          "http://yappy_vins_hamster_0.yappy-slots.yandex-team.ru/speechkit/app/pa/" },
        { "hamster_balancer", "http://vins.hamster.alice.yandex.net/speechkit/app/pa/" },
        { "testing",          "http://vins-int.tst.voicetech.yandex.net/speechkit/app/pa/" },
        { "prestable",        "http://vins-int.voicetech.yandex.net/speechkit-prestable/app/pa/" },
        { "prod",             "http://vins-int.voicetech.yandex.net/speechkit/app/pa/" },
        { "prod_rtc",         "http://vins.alice.yandex.net/speechkit/app/pa/" }
};

TString PrintVinsUrlHelp() {
    TStringBuilder out;
    out << "You should specify either VINS url or predefined name." << Endl;
    out << "Predefined names:" << Endl;
    out << "  experiments:      " << VINS_URLS["experiments"] << Endl;
    out << "  hamster:          " << VINS_URLS["hamster"] << Endl;
    out << "  hamster_balancer: " << VINS_URLS["hamster_balancer"] << Endl;
    out << "  testing:          " << VINS_URLS["testing"] << Endl;
    out << "  prestable:        " << VINS_URLS["prestable"] << Endl;
    out << "  prod:             " << VINS_URLS["prod"] << Endl;
    out << "  prod_rtc:         " << VINS_URLS["prod_rtc"] << Endl;
    return out;
}

THashMap<TStringBuf, TStringBuf> BASS_URLS = {
        { "hamster",  "http://bass.hamster.alice.yandex.net/" },
        { "testing",  "http://bass-testing.n.yandex-team.ru/" },
        { "priemka",  "http://bass-priemka.n.yandex-team.ru/" },
        { "prod",     "http://bass-prod.yandex.net/" },
        { "rc_yappy", "http://alice-bass-rm.yappy.yandex.ru/" }
};

TString PrintBassUrlHelp() {
    TStringBuilder out;
    out << "You can specify either BASS url or predefined name." << Endl;
    out << "Omit this option to use default url (prod)" << Endl;
    out << "Predefined names:" << Endl;
    out << "  hamster:  " << VINS_URLS["hamster"] << Endl;
    out << "  testing:  " << VINS_URLS["testing"] << Endl;
    out << "  priemka:  " << VINS_URLS["priemka"] << Endl;
    out << "  prod:     " << VINS_URLS["prod"] << Endl;
    out << "  rc_yappy: " << VINS_URLS["rc_yappy"] << Endl;
    return out;
}

} // namespace anonymous

int main(int argc, const char** argv) {
    NLastGetopt::TOpts opts;

    TString vinsUrl;
    opts.AddLongOption("vins-url")
        .Help(PrintVinsUrlHelp())
        .Required()
        .RequiredArgument()
        .Handler1T<TStringBuf>(
            [&vinsUrl] (TStringBuf s) {
                vinsUrl = VINS_URLS.Value(s, s);
            }
         );
    TString bassUrl;
    opts.AddLongOption("bass-url")
        .Help(PrintBassUrlHelp())
        .Optional()
        .RequiredArgument()
        .Handler1T<TStringBuf>(
            [&bassUrl] (TStringBuf s) {
                bassUrl = BASS_URLS.Value(s, s);
            }
        );

    TString reqWizardUrl;
    opts.AddLongOption("req-wizard-url", "You can specify request wizard url. Omit this option to use default url.")
        .Optional()
        .RequiredArgument()
        .StoreResult(&reqWizardUrl);

    TString inputFile;
    opts.AddLongOption('i', "input-file")
        .DefaultValue("-").RequiredArgument().StoreResult(&inputFile);

    TString inputFormat;
    opts.AddLongOption('f', "input-format", "'text' (one request per line) or 'json' (one VINS request json per line)")
        .DefaultValue("text").RequiredArgument().StoreResult(&inputFormat);

    TFsPath outputDir;
    opts.AddLongOption('o', "output-dir", "directory for output files requests.txt, responses.txt, ")
        .DefaultValue("./vins_output").RequiredArgument().StoreResult(&outputDir);

    ui32 threads;
    opts.AddLongOption('t', "threads")
        .DefaultValue("2").RequiredArgument().StoreResult(&threads);

    bool simple = false;
    opts.AddLongOption('s', "simple", "write responses to STDOUT in simple human-readable mode")
        .NoArgument().SetFlag(&simple);

    TString patchFile;
    opts.AddLongOption("patch-file", "file with json patch to requests")
        .RequiredArgument().StoreResult(&patchFile);

    TString lang;
    opts.AddLongOption("lang", "lang for request (supported: ru-RU, tr-TR)")
        .DefaultValue("ru-RU").StoreResult(&lang);

    TString client;
    opts.AddLongOption("client", "Use predefined client profile (supported: quasar-tv,quasar-notv) or use this string as client_id (ru.yandex.yandexnavi)")
        .RequiredArgument().StoreResult(&client);

    TString contentSettings;
    opts.AddLongOption("quasar-filter", "Choose quasar content filtration: children, medium, without (medium is default)")
        .DefaultValue("medium").RequiredArgument().StoreResult(&contentSettings);

    THashSet<TString> experiments;
    opts.AddLongOption('e', "exp", "Experiment flags to be set in request")
        .RequiredArgument()
        .Handler1T<TStringBuf>(TExperimentArgParser(&experiments));

    bool noSrcrwr = false;
    opts.AddLongOption("no-srcrwr", "Send bass_url using additional_options instead of X-Srcrwr header")
        .NoArgument().SetFlag(&noSrcrwr);

    TString oauthToken;
    opts.AddLongOption("oauth", "oauth token")
        .Optional()
        .RequiredArgument().StoreResult(&oauthToken);

    NLastGetopt::TOptsParseResult optsRes(&opts, argc, argv);

    TRequestsQueue requestsQueue(threads);

    TAdaptiveThreadPool queue;
    queue.Start(0);

    THolder<IInputStream> input = OpenInput(inputFile);
    THolder<TReader> reader;
    if (inputFormat == TStringBuf("text")) {
        reader = MakeHolder<TTextReader>(std::move(input));
    } else if (inputFormat == TStringBuf("json")) {
        reader = MakeHolder<TJsonReader>(std::move(input));
    } else {
        ythrow yexception() << "Unknown format " << inputFormat;
    }

    THolder<IVinsResponseWriter> writer;
    if (simple) {
        writer.Reset(new TSimpleVinsResponseWriter(OpenOutput("-")));
    } else {
        writer.Reset(new TSplittedVinsResponseWriter(outputDir));
    }

    NSc::TValue patch;
    if (patchFile) {
        THolder<IInputStream> patchInput = OpenInput(patchFile);
        patch = NSc::TValue::FromJson(patchInput->ReadAll());
    }
    patch["application"]["lang"] = lang;
    patch["application"]["timestamp"] = ToString(TInstant::Now().Seconds());

    constexpr TStringBuf quasarClientId = "ru.yandex.quasar.services/1.0 (none none; android 6.0.1)";

    THashMap<TStringBuf, std::function<void()>> clientHandlers;
    clientHandlers[TStringBuf("quasar-tv")] = [&patch, quasarClientId, contentSettings] () {
        patch["application"]["app_id"] = quasarClientId;
        patch["request"]["device_state"]["is_tv_plugged_in"].SetBool(true);
        patch["request"]["device_state"]["device_config"]["content_settings"].SetString(contentSettings);
    };
    clientHandlers[TStringBuf("quasar-notv")] = [&patch, quasarClientId] () {
        patch["application"]["app_id"] = quasarClientId;
    };
    clientHandlers[TStringBuf("yastroka")] = [&patch] () {
        patch["application"]["app_id"] = TStringBuf("winsearchbar/1.13.0.0 (none none; Windows 6.1)");
    };
    clientHandlers[TStringBuf("quasar-sound")] = [&patch, quasarClientId, contentSettings] () {
        patch["application"]["app_id"] = quasarClientId;
        patch["request"]["device_state"]["is_tv_plugged_in"].SetBool(false);
        patch["request"]["device_state"]["sound_level"].SetIntNumber(5);
        patch["request"]["device_state"]["sound_muted"].SetBool(false);
    };

    if (client) {
        auto ptr = clientHandlers.FindPtr(client);
        if (ptr) {
            (*ptr)();
        } else {
            patch["application"]["app_id"] = client;
        }
    }

    if (experiments) {
        NSc::TValue expPatch = patch["request"]["experiments"];
        for (const TString& exp : experiments) {
            expPatch.Push(NSc::TValue(exp));
        }
    }

    if (oauthToken) {
        patch["request"]["additional_options"]["oauth_token"] = oauthToken;
    }

    TVector<NThreading::TFuture<void>> threadFutures;
    threadFutures.push_back(NThreading::Async([&requestsQueue, &reader, &vinsUrl, &bassUrl, &reqWizardUrl, &patch, &noSrcrwr] () {
        NHttpFetcher::TRequestOptions requestOptions;
        requestOptions.MaxAttempts = 5;
        requestOptions.Timeout = TDuration::MilliSeconds(10000);
        requestOptions.RetryPeriod = TDuration::MilliSeconds(2500);

        ui64 id = 0;
        while (reader->IsValid()) {
            NSc::TValue value = reader->ReadNext();
            value.MergeUpdate(patch);

            NHttpFetcher::TRequestPtr request = NHttpFetcher::Request(vinsUrl, requestOptions);
            if (bassUrl) {
                if (noSrcrwr) {
                    value["request"]["additional_options"]["bass_url"] = bassUrl;
                } else {
                    request->AddHeader(NAlice::NNetwork::HEADER_X_SRCRWR,
                                       TStringBuilder() << NHttpFetcher::HEADER_XSRCRWR_BASS_VALUE << "=" << bassUrl);
                }
            }
            if (reqWizardUrl) {
                request->AddHeader(
                    NAlice::NNetwork::HEADER_X_SRCRWR,
                    TStringBuilder() << NHttpFetcher::HEADER_XSRCRWR_REQWIZARD_VALUE << "=" << reqWizardUrl);
            }
            request->SetMethod("POST");
            request->SetBody(value.ToJson());
            request->SetContentType("application/json");

            TRequestContext ctx;
            ctx.Handle = request->Fetch();
            ctx.Id = id++;
            ctx.RequestText = value["request"]["event"]["text"].GetString();
            requestsQueue.Push(std::move(ctx));
        }
        requestsQueue.Stop();
    }, queue));

    threadFutures.push_back(NThreading::Async([&requestsQueue, &writer] () {
        ui32 count = 0;
        while (TMaybe<TRequestContext> ctx = requestsQueue.Pop()) {
            NHttpFetcher::THandle::TRef handle = ctx->Handle;
            NHttpFetcher::TResponse::TRef response = handle->Wait();

            if (response->IsError()) {
                writer->WriteError(ctx->Id, ctx->RequestText, response->GetErrorText(), response->Data);
            } else if (response->Data.empty()) {
                writer->WriteError(ctx->Id, ctx->RequestText, "empty response", response->Data);
            } else {
                writer->WriteSuccessfulResponse(ctx->RequestText, response->Data);
            }

            if (count++ >= 200) {
                writer->Flush();
            }
        }
    }, queue));

    NThreading::TFuture<void> future = NThreading::WaitExceptionOrAll(threadFutures);
    future.Wait();
    future.GetValue();
    writer->Flush();

    return 0;
}
