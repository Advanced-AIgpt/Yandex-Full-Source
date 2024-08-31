#include "fetcher.h"
#include <alice/begemot/lib/api/experiments/flags.h>
#include <alice/begemot/lib/api/params/wizextra.h>
#include <alice/begemot/lib/locale/locale.h>
#include <alice/nlu/granet/lib/utils/string_utils.h>
#include <alice/nlu/granet/lib/utils/utils.h>
#include <dict/dictutil/dictutil.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/http/client/client.h>
#include <library/cpp/http/client/query.h>
#include <library/cpp/http/client/request.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/folder/path.h>
#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/string/join.h>
#include <util/system/env.h>

namespace NGranet {

const TString ENABLED_BEGEMOT_RULES = Join(
    ",",
    "AliceArFst",
    "AliceThesaurus",
    "AliceTranslit",
    "AliceEmbeddings",
    "AliceEmbeddingsExport",
    "AliceIot",
    "AliceIotConfigParser",
    "AliceCustomEntities",
    "AliceIotHelper",
    "AliceNonsenseTagger",
    "AliceNormalizer",
    "AliceRequest",
    "AliceSampleFeatures",
    "AliceSession",
    "AliceTokenEmbedder",
    "AliceTypeParserTime",
    "AliceUserEntities",
    "CustomEntities",
    "Date",
    "DirtyLang",
    "EntityFinder",
    "ExternalMarkup",
    "Fio",
    "FstAlbum",
    "FstArtist",
    "FstCalc",
    "FstCurrency",
    "FstDate",
    "FstDatetime",
    "FstDatetimeRange",
    "FstFilms100_750",
    "FstFilms50Filtered",
    "FstFio",
    "FstFloat",
    "FstGeo",
    "FstNum",
    "FstPoiCategoryRu",
    "FstSite",
    "FstSoft",
    "FstSwear",
    "FstTime",
    "FstTrack",
    "FstUnitsTime",
    "FstWeekdays",
    "GeoAddr",
    "Granet",
    "GranetCompiler",
    "GranetConfig",
    "IsNav",
    "Wares"
);

static TCgiParameters MakeCgiParameters(TStringBuf text, const TGranetDomain& domain,
    const TBegemotFetcherOptions& options)
{
    const EYandexSerpTld tld = NAlice::NAliceLocale::LanguageToTld(domain.Lang, YST_RU);

    TVector<TString> wizextra;
    wizextra.push_back(NAlice::WIZEXTRA_KEY_ALICE_PREPROCESSING + "=true");
    if (domain.IsPASkills) {
        wizextra.push_back(NAlice::EXP_BEGEMOT_PASKILLS);
    } else if (domain.IsWizard) {
        // Defined by shard
    } else if (domain.IsSnezhana) {
        wizextra.push_back(NAlice::EXP_BEGEMOT_SNEZHANA);
    }
    if (!options.Wizextra.empty()) {
        wizextra.push_back(options.Wizextra);
    }

    TCgiParameters cgi;
    cgi.InsertEscaped(TStringBuf("text"), text);
    cgi.InsertEscaped(TStringBuf("lr"), TStringBuf("213"));
    cgi.InsertEscaped(TStringBuf("uil"), IsoNameByLanguage(domain.Lang)); // ru, tr, kk
    cgi.InsertEscaped(TStringBuf("tld"), ToString(tld)); // ru, tr, kz
    cgi.InsertEscaped(TStringBuf("format"), TStringBuf("json"));
    cgi.InsertEscaped(TStringBuf("wizclient"), options.WizClient);
    cgi.InsertEscaped(TStringBuf("rwr"), ENABLED_BEGEMOT_RULES);
    cgi.InsertEscaped(TStringBuf("wizextra"), JoinSeq(";", wizextra));

    return cgi;
}

static TString GetBegemotUrl(const TBegemotFetcherOptions& options) {
    TString url = AddSchemePrefix(options.Url, "http");
    TString host;
    TString path;
    SplitUrlToHostAndPath(url, host, path);
    if (path == "" || path == "/") {
        url = host + "/wizard";
    }
    return url;
}

static NHttp::TFetchOptions MakeQueryOptions() {
    return {
        .Timeout = TDuration::Seconds(1),
        .RetryCount = 4,
        .RetryDelay = TDuration::MilliSeconds(200),
    };
}

static NJson::TJsonValue RequestBegemot(TStringBuf text, const TGranetDomain& domain,
    const TBegemotFetcherOptions& options)
{
    const TString url = GetBegemotUrl(options) + "?" + MakeCgiParameters(text, domain, options).Print();
    // Cerr << "Request to Begemot: " << url << Endl;

    NHttp::TFetchClient client;
    NHttp::TFetchQuery query(url, MakeQueryOptions());
    NHttpFetcher::TResultRef response = client.Fetch(query, {}).Get();

    Y_ENSURE(response, "No answer from server");
    Y_ENSURE(response->Success(), "Request failed. Error code: " << response->Code);
    Y_ENSURE(!response->Data.empty(), "Empty response");

    TString responseBody = response->Data;
    NJson::TJsonValue responseJson;
    if (ReadJsonTree(responseBody, &responseJson, false)) {
        return responseJson;
    }

    const bool isTextEmpty = responseBody.Contains("wizerror=Search+request+is+empty")
        || responseBody.Contains("wizerror=Search+request+consists+of+non-letter+characters%2C+which+are+stripped+entirely+by+normalization");
    Y_ENSURE(isTextEmpty, "Bad response: " << CropWithEllipsis(responseBody, 200));
    return {};
}

bool FetchSampleMock(TStringBuf text, const TGranetDomain& domain, TSampleMock* sampleMock,
    TEmbeddingsMock* embeddingsMock, TString* errorMessage, const TBegemotFetcherOptions& options)
{
    try {
        const NJson::TJsonValue json = RequestBegemot(text, domain, options);
        if (sampleMock != nullptr) {
            *sampleMock = TSampleMock::FromBegemotResponse(text, json);
        }
        if (embeddingsMock != nullptr) {
            *embeddingsMock = TEmbeddingsMock::FromBegemotResponse(json);
        }
    } catch (const yexception& e) {
        if (errorMessage != nullptr) {
            *errorMessage = e.what();
        }
        return false;
    }
    return true;
}

bool FetchSampleEntities(const TSample::TRef& sample, const TGranetDomain& domain,
    const TBegemotFetcherOptions& options)
{
    Y_ENSURE(sample);
    TSampleMock mock;
    if (!FetchSampleMock(sample->GetText(), domain, &mock, nullptr, nullptr, options)) {
        return false;
    }
    sample->AddEntitiesOnTokens(mock.Entities);
    return true;
}

} // namespace NGranet
