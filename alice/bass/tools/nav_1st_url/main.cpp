#include <alice/bass/libs/fetcher/neh.h>

#include <library/cpp/getopt/opt.h>
#include <library/cpp/scheme/scheme.h>

#include <mapreduce/yt/client/init.h>
#include <mapreduce/yt/interface/client.h>

#include <util/string/builder.h>


const NSc::TValue& FindBlock(TStringBuf blockName, const NSc::TArray& blocks) {
    for (const NSc::TValue& block : blocks) {
        if (block["name"].GetString() == blockName)
            return block;
    }
    return NSc::Null();
}

void ProcessResponse(TStringBuf reqid, TStringBuf utterance, TStringBuf clientId, NSc::TValue&& response) {
    // Cout << response.ToJsonPretty() << Endl;

    const NSc::TValue& block = FindBlock(TStringBuf("nav_debug"), response["blocks"].GetArray());
    if (block.IsNull()) {
        Cerr << reqid << ": missing block 'nav_debug'" << Endl;
        return;
    }

    const NSc::TArray& data = block["data"].GetArray();
    if (data.empty()) {
        Cerr << reqid << ": 'nav_debug' block is empty" << Endl;
        return;
    }

    Cout << utterance << '\t' << clientId.NextTok('/') << '\t';

    auto fn = [](const NSc::TValue& doc) {
        Cout << doc["server_descr"].GetString("-") << '\t'
             << doc["title"].GetString("-") << '\t'
             << doc["relev"].ForceNumber(0) << '\t';
    };
    fn(data[0]);
    if (data.size() > 1) {
        fn(data[1]);
    }

    Cout << Endl;
}

int main(int argc, const char** argv) {
    NYT::Initialize(argc, argv);

    NLastGetopt::TOpts opts;
    opts.SetFreeArgsNum(0);

    TString ytpath;
    opts.AddLongOption("ytpath")
            .Help("YT table path in format 'serverName://tableName'")
            .Required()
            .DefaultValue("hahn://home/sup/production/nav-1st_url-queries_2018-04-25")
            .StoreResult(&ytpath);

    TString bassUrl;
    opts.AddLongOption("bass")
            .Help("BASS url or an alias from list [prod, priemka, testing, fuzzy, local]")
            .Required()
            .DefaultValue("local")
            .Handler1T<TStringBuf>([&bassUrl](TStringBuf arg) {
                if (arg == "prod")
                    bassUrl = "http://bass-prod.yandex.net/";
                else if (arg == "priemka")
                    bassUrl = "http://bass-priemka.n.yandex-team.ru/";
                else if (arg == "testing")
                    bassUrl = "http://bass-testing.n.yandex-team.ru/";
                else if (arg == "fuzzy")
                    bassUrl = "http://fuzzy.search.yandex.net:12345/";
                else if (arg == "local")
                    bassUrl = "http://localhost:12345/";
                else
                    bassUrl = arg;
                bassUrl += "/vins";
            });

    ui32 limit;
    opts.AddLongOption("limit")
            .Help("Max rows to process, 0 - unlimited")
            .Optional()
            .DefaultValue(1)
            .StoreResult(&limit);

    NLastGetopt::TOptsParseResult res(&opts, argc, argv);

    TString serverName, tableName;
    try {
        Split(ytpath, ':', serverName, tableName);
    } catch (const yexception& e) {
        Cerr << "Invalid table path format" << Endl;
        return -1;
    }
    if (serverName.empty() || tableName.empty()) {
        Cerr << "Invalid <table-path> value" << Endl;
        return -1;
    }

    if (limit == 0)
        limit = std::numeric_limits<ui32>::max();

    NYT::IClientPtr client = NYT::CreateClient(serverName);
    if (!client->Exists(tableName)) {
        Cerr << "table '" << tableName << "' does not exist" << Endl;
        return -2;
    }

    NHttpFetcher::TRequestOptions bassRequestOptions;
    bassRequestOptions.MaxAttempts = 5;
    bassRequestOptions.Timeout = TDuration::MilliSeconds(10000);
    bassRequestOptions.RetryPeriod = TDuration::MilliSeconds(2500);

    auto reader = client->CreateTableReader<NYT::TNode>(tableName);
    ui32 numRows = 0;
    for (; reader->IsValid() && numRows < limit; reader->Next(), ++numRows) {
        const NYT::TNode::TMapType& row = reader->GetRow().AsMap();
        const TString& reqid = row.find("reqid")->second.AsString();
        const TString& clientId = row.find("client_id")->second.AsString();
        const TString& utterance = row.find("utterance")->second.AsString();
        const TString& requestData = row.find("request")->second.AsString();

        NSc::TValue requestJson = NSc::TValue::FromJsonThrow(requestData);
        requestJson["meta"]["experiments"].Push(NSc::TValue("nav_debug"));

        NHttpFetcher::TRequestPtr request = NHttpFetcher::Request(bassUrl, bassRequestOptions);
        request->SetMethod("POST");
        request->SetBody(requestJson.ToJson());
        request->SetContentType("application/json");

        NHttpFetcher::TResponse::TRef response = request->Fetch()->Wait();
        if (response->IsHttpOk()) {
            ProcessResponse(reqid, utterance, clientId, NSc::TValue::FromJsonThrow(response->Data));
        } else {
            Cerr << reqid << " failure: " << response->GetErrorText() << Endl;
        }
    }

    return 0;
}
