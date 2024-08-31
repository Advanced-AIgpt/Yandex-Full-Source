#include <alice/bass/libs/fetcher/neh.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/ydb_kv/kv.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/http/misc/parsed_request.h>
#include <library/cpp/http/server/http.h>
#include <library/cpp/http/server/response.h>
#include <library/cpp/scheme/scheme.h>
#include <library/cpp/sighandler/async_signals_handler.h>
#include <library/cpp/threading/future/async.h>
#include <library/cpp/threading/future/future.h>

#include <util/generic/string.h>
#include <util/stream/file.h>
#include <util/system/env.h>
#include <util/thread/pool.h>


const TString TESTENV_URL = "https://testenv.yandex-team.ru/handlers/release/pre_release";

const TString TE_DB = "bass-trunk";
const TString TE_DB_TEST = "bassrm-trunk";
const TString YDB_DB = "/ru/alice/prod/games";
const TString YDB_TABLE_PATH = "release_bot/users";
const TString YDB_ENDPOINT = "ydb-ru.yandex.net:2135";

constexpr TStringBuf NEW_BRANCH = "Отведи ветку";
constexpr TStringBuf STATUS = "Статус";

static const auto YDB_RETRY = NYdb::NTable::TRetryOperationSettings()
    .SlowBackoffSettings(
        NYdb::NTable::TRetryOperationSettings::DefaultSlowBackoffSettings()
            .SlotDuration(TDuration::MilliSeconds(150))
            .Ceiling(1)
    );

const TDuration TIMEOUT = TDuration::Seconds(10);

TVector<TString> NewBranchUtterance = {
    "Отводи",
    "отводи",
    "Отведи",
    "отведи",
    "Отведи ветку",
    "отведи ветку",
    "Отводи ветку",
    "отводи ветку",
    "Новая ветка",
    "новая ветка",
};

TVector<TString> StatusUtterance = {
    "Где ветка",
    "где ветка",
    "Как дела",
    "как дела",
    "Статус",
    "статус",
    "Что с веткой",
    "что с веткой",
};

struct TBranch {
    explicit TBranch()
        : Created(TInstant::Now())
    {
    }

    TInstant Created;
    bool IsReady = false;
    bool Ok = false;
    TString Message;
};

void ReplyWithCode(HttpCodes code, TStringBuf content, THttpOutput& output) {
    LOG(INFO) << "Response " << HttpCodeStr(code) << ": " << content << Endl;

    THttpResponse httpResponse(code);
    httpResponse.SetContentType("application/json");
    output << httpResponse << content;
    output.Flush();
}

void SetButton(NSc::TValue& response, const TVector<TStringBuf>& buttons) {
    for (auto name : buttons) {
        NSc::TValue& v = response["response"]["buttons"].Push();
        v["title"] = name;
        v["hide"] = true;
    }
}

TString GetReqId(const THttpHeaders& httpHeaders, TStringBuf defaultPrefix) {
    THashMap<TString, TString> headers;
    for (auto h : httpHeaders) {
        headers.insert({h.Name(), h.Value()});
    }

    if (auto id = headers.FindPtr("X-Request-Id")) {
        return *id;
    }
    if (auto id = headers.FindPtr("X-Req-Id")) {
        return *id;
    }
    return TStringBuilder() << defaultPrefix << TInstant::Now().MicroSeconds();
}

class SuperBot : public TRequestReplier {
public:
    SuperBot(const TString& teDb,
             const TString& ydbDb,
             THashMap<TString,
             TVector<TBranch>>* branches,
             THashMap<TString,
             TVector<NThreading::TFuture<void>>>* branchRequests)
        : TeDb(teDb)
        , ThreadPool_(new TAdaptiveThreadPool())
        , Branches(branches)
        , BranchRequests(branchRequests)
        , YdbClient(NYdb::TDriver(
            NYdb::TDriverConfig().SetEndpoint(YDB_ENDPOINT).SetDatabase(ydbDb).SetAuthToken(GetEnv("YDB_TOKEN"))))
        , KV(YdbClient, {ydbDb, YDB_TABLE_PATH}, YDB_RETRY)
    {
        ThreadPool_->Start(0);
        if (!KV.Exists().IsSuccess()) {
            LOG(INFO) << "No table" << Endl;
            KV.Create();
            LOG(INFO) << "Table created" << Endl;
        }
    }

    bool DoReply(const TReplyParams& params) override {
        TParsedHttpFull request(params.Input.FirstLine());

        if (request.Path == "/release_bot") {
            return Answer(params);
        }
        LOG(ERR) << "bad http path: " << request.Path << Endl;
        params.Output << THttpResponse(HTTP_BAD_REQUEST);
        params.Output.Flush();
        return true;
    }

private:
    void UpdateUsers() {
        auto status = KV.GetAll();
        if (status.IsSuccess()) {
            Users.clear();
            auto kvs = status.GetKeyValues();
            for (const auto& t : kvs) {
                Users[t.GetKey()] = t.GetValue();
                LOG(INFO) << "Name: " << t.GetValue() << " Id: " << t.GetKey() << Endl;
            }
        }
    }

    std::pair<bool, TString> CreateBranch(TString token, TString name) {
        std::system("svn info svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia | grep \"Last Changed Rev\" > revision.txt");

        TFileInput file("revision.txt");
        TString s;
        file.ReadLine(s);
        int revision = FromString<int>(TStringBuf(s).RAfter(':'));

        TString url = TStringBuilder() << TESTENV_URL << "?database=" << TeDb << "&revision=" << revision;
        LOG(INFO) << "TestEnv request url: " << url << Endl;
        NHttpFetcher::TRequestOptions options;
        options.MaxAttempts = 1;
        options.EnableFastReconnect = true;
        options.Timeout = TIMEOUT;
        NHttpFetcher::TRequestPtr r = NHttpFetcher::Request(url, options);

        r->AddHeader(TStringBuf("Authorization"), TStringBuilder() << "OAuth " << token);

        NHttpFetcher::TResponse::TRef resp = r->Fetch()->WaitFor(TIMEOUT);

        if (resp->IsHttpOk()) {
            LOG(INFO) << "OK, New branch has been created by " << name << Endl;
            return {true, ""};
        }
        TString error = TStringBuilder{} << "Failed to created a new branch, Error: " << resp->GetErrorText();
        LOG(ERR) << error << ", Data: " << resp->Data << Endl;
        return {false, error};
    }

    bool Answer(const TReplyParams& params) {
        UpdateUsers();

        TLogging::ReqInfo.Get().Update(GetReqId(params.Input.Headers(), "local_"));

        const TString inputStr = params.Input.ReadAll();
        LOG(INFO) << "Request: " << inputStr << Endl;

        NSc::TValue request = NSc::TValue::FromJson(inputStr);
        if (!request.IsDict() ||
            request["request"]["command"].IsNull() ||
            request["session"]["user_id"].IsNull())
        {
            ReplyWithCode(HTTP_BAD_REQUEST, "Bad input json?", params.Output);
            return true;
        }

        NSc::TValue response;
        response["version"] = request["version"];
        response["session"] = request["session"];

        TString userId = TString{request["session"]["user_id"].GetString()};
        auto& branchList = (*Branches)[userId];

        if (Users.find(userId) == Users.end()) {
            AnswerUnauthorizedUser(response);
        } else if (request["session"]["new"].GetBool()) {
            AnswerNewSession(userId, response);
        } else if (Find(NewBranchUtterance, request["request"]["command"].GetString()) != NewBranchUtterance.end()) {
            AnswerNewBranch(userId, branchList, response);
        } else if (Find(StatusUtterance, request["request"]["command"].GetString()) != StatusUtterance.end()) {
            AnswerStatus(userId, branchList, response);
        } else {
            AnswerNoIdea(response);
        }
        ReplyWithCode(HTTP_OK, response.ToJson(), params.Output);
        return true;
    }

    void AnswerUnauthorizedUser(NSc::TValue& response) {
        LOG(INFO) << "Unauthorized user" << Endl;
        response["response"]["text"].SetString("Извините, но Вы не достойны быть пользователем этого навыка! Ха-ха!");
        response["response"]["end_session"].SetBool(true);
    }

    void AnswerNewSession(const TString& userId, NSc::TValue& response) {
        LOG(INFO) << "New session" << Endl;
        response["response"]["text"].SetString(TStringBuilder() << "Привет, " << Users[userId] << "! Это супер крутой скилл, который пока умеет только отводить ветки Басса. Если хотите добавить новую функциональность -- Welcome to <#tts аркадия элис басс тулз б+асрел+изб+от#><#text arcadia/alice/bass/tools/bass_release_bot#>\nЧтобы отвести ветку, просто скажите \"Отведи ветку\"");
        SetButton(response, {NEW_BRANCH});
    }

    void AnswerNewBranch(const TString& userId, TVector<TBranch>& branchList, NSc::TValue& response) {
        if (branchList.empty() || branchList.back().IsReady) {
            branchList.emplace_back();
            LOG(INFO) << "Creating request for a new branch" << Endl;
            (*BranchRequests)[userId].push_back(NThreading::Async(
                [userId, this]() {
                    auto resp = CreateBranch(GetEnv("TESTENV_TOKEN"), Users[userId]);
                    (*Branches)[userId].back().Ok = resp.first;
                    (*Branches)[userId].back().Message = resp.second;
                }, *ThreadPool_));
            response["response"]["text"].SetString(
                "Я отправила запрос на отведение новой ветки, чтобы узнать его судьбу, скажите: <#tts-#><#text#>\"Статус\"");
        } else {
            LOG(INFO) << "Last branch result hasn't been checked, yet" << Endl;
            response["response"]["text"].SetString(
                "Вы еще не посмотрели результат своего последнего запроса, чтобы сделать это, скажите: <#tts-#><#text#>\"Статус\"");
        }
        SetButton(response, {STATUS});
    }

    void AnswerStatus(const TString& userId, TVector<TBranch>& branchList, NSc::TValue& response) {
        if (branchList.empty()) {
            LOG(INFO) << "No requests to show status for" << Endl;
            response["response"]["text"].SetString("Вы еще не отводили ни одной ветки. Чтобы отвести ветку, просто скажите <#tts-#><#text#>\"Отведи ветку\"");
            SetButton(response, {NEW_BRANCH});
        } else {
            if (!branchList.back().IsReady && !(*BranchRequests)[userId].empty() && !(*BranchRequests)[userId].back().Wait(TDuration::Seconds(1))) {
                LOG(INFO) << "No information about the last request" << Endl;
                response["response"]["text"].SetString("Пока нет информации<#tts#><#text,#> подождите ещё<#tts#><#text,#> пожалуйста");
                SetButton(response, {STATUS});
            } else {
                if(!branchList.back().IsReady) {
                    (*Branches)[userId].back().IsReady = true;
                    (*BranchRequests)[userId].pop_back();
                }

                LOG(INFO) << "Branch status: " << branchList.back().Message << Endl;
                int minutes = ((TInstant::Now() - branchList.back().Created).Seconds() - 1) / 60 + 1;
                TString lastLetter;
                int lastDigit = minutes % 10;
                int lastTwoDigits = minutes % 100;
                if (lastDigit == 1 && lastTwoDigits != 11) {
                    lastLetter = "у";
                } else if (lastDigit >= 2 && lastDigit <= 4 && (lastTwoDigits < 12 || lastTwoDigits > 14)) {
                    lastLetter = "ы";
                }
                if (branchList.back().Ok) {
                    response["response"]["text"].SetString(TStringBuilder() << "Запрос на отведение ветки, отправленный " << minutes << " минут" << lastLetter << " назад, был успешно доставлен.");
                } else {
                    response["response"]["text"].SetString(TStringBuilder() << "Запрос на отведение ветки, отправленный " << minutes << " минут" << lastLetter << " назад, вернулся с ошибкой: " << branchList.back().Message);
                }
                SetButton(response, {NEW_BRANCH});
            }
        }
    }

    void AnswerNoIdea(NSc::TValue& response) {
        LOG(INFO) << "Don't know what to do" << Endl;
        response["response"]["text"].SetString("Что!? Либо я тупая, либо Вы несёте какой-то бред.");
        SetButton(response, {NEW_BRANCH, STATUS});
    }

private:
    TString TeDb;

    THashMap<TString, TString> Users;

    THolder<IThreadPool> ThreadPool_;

    THashMap<TString, TVector<TBranch>>* Branches;
    THashMap<TString, TVector<NThreading::TFuture<void>>>* BranchRequests;

    NYdb::NTable::TTableClient YdbClient;

    NYdbKV::TKV KV;

};

class THttpCallback: public THttpServer::ICallBack {
public:
    THttpCallback(const TString& teDb, const TString& ydbDb)
        : TeDb(teDb)
        , YdbDb(ydbDb)
    {
    }

    TClientRequest* CreateClient() override {
        return new SuperBot(TeDb, YdbDb, &Branches, &BranchRequests);
    }

private:
    TString TeDb;
    TString YdbDb;

    THashMap<TString, TVector<TBranch>> Branches;
    THashMap<TString, TVector<NThreading::TFuture<void>>> BranchRequests;
};


int main(int argc, const char** argv) {
    NLastGetopt::TOpts opts;

    int httpPort = 0;
    opts.AddLongOption("port").Help("HTTP port").DefaultValue(17890).StoreResult(&httpPort);
    bool prod;
    opts.AddLongOption("prod").Help("If set, bot will use bass-trunk database, otherwise bassrm-trunk").DefaultValue(false).SetFlag(&prod);
    ELogPriority logLevel;
    opts.AddLongOption("loglevel").Help("Log level").DefaultValue("DEBUG").StoreResultT<ELogPriority>(&logLevel);
    TString logDir;
    opts.AddLongOption("logdir").Help("Logdir or console").DefaultValue("console").StoreResult(&logDir);

    NLastGetopt::TOptsParseResult res(&opts, argc, argv);

    TLogging::Configure(logDir, httpPort, logLevel);

    TString testenvDatabase = TE_DB_TEST;
    if (prod) {
        testenvDatabase = TE_DB;
    }

    THttpCallback cb(testenvDatabase, YDB_DB);

    THttpServer::TOptions options(httpPort);
    options.SetThreads(10);
    options.EnableKeepAlive(true);
    options.EnableCompression(true);
    THttpServer server(&cb, options);

    auto shutdown = [&server](int) {
        LOG(INFO) << "Shutdown initiated" << Endl;
        server.Shutdown();
    };
    SetAsyncSignalFunction(SIGTERM, shutdown);
    SetAsyncSignalFunction(SIGINT, shutdown);

    auto rotate = [](int) {
        LOG(INFO) << "Logs rotated" << Endl;
        TLogging::Rotate();
    };
    SetAsyncSignalFunction(SIGHUP, rotate);

    while (!server.Start()) {
        LOG(INFO) << "Starting HTTP server..." << Endl;
        Sleep(TDuration::Seconds(1));
    }
    LOG(INFO) << "HTTP server started on port " << httpPort << Endl;
    server.Wait();
    LOG(INFO) << "HTTP server stopped" << Endl;

    SetAsyncSignalFunction(SIGTERM, nullptr);
    SetAsyncSignalFunction(SIGINT, nullptr);
    SetAsyncSignalFunction(SIGHUP, nullptr);
}
