#include "navigation.h"
#include "bno_apps.h"

#include <alice/bass/forms/automotive/open_site_or_app.h>
#include <alice/bass/forms/computer_vision/handler.h>
#include <alice/bass/forms/directives.h>
#include <alice/bass/forms/search/serp.h>
#include <alice/bass/forms/urls_builder.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/metrics/metrics.h>

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/app_navigation/navigation.h>

#include <library/cpp/string_utils/quote/quote.h>
#include <alice/library/datetime/datetime.h>
#include <util/string/cast.h>
#include <util/string/join.h>


namespace NBASS {

void TNavigationFormHandler::Init() {
    TNavigationFixList::Instance();
    TBnoApps::Instance();
}

///// Navigation Form Hanlder //////////////////////////////////////////////////////////////////////////////////////////
namespace {
static constexpr TStringBuf FORM_NAME = "personal_assistant.scenarios.open_site_or_app";
static constexpr TStringBuf FORM_NAME_DO_OPEN = "personal_assistant.scenarios.open_site_or_app__open";
static constexpr TStringBuf FORM_NAME_ELLIPSIS = "personal_assistant.scenarios.open_site_or_app__ellipsis";

class TExitHandler {
public:
    TExitHandler(std::function<void()> fn)
            : Fn(fn) {
    }
    ~TExitHandler() {
        Fn();
    }
private:
    std::function<void()> Fn;
};
}

using namespace NSerpSnippets;

struct TNavigationFormHandler::TFixListResult {
    bool HasResult = false;
    TResultValue ResultValue = {};
};

TResultValue TNavigationFormHandler::Do(TRequestHandler& r) {
    auto& ctx = r.Ctx();
    const auto& clientInfo = ctx.MetaClientInfo();

    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::NAV_URL);

    TContext::TSlot* slotTarget = ctx.GetSlot(TStringBuf("target"));
    if (IsSlotEmpty(slotTarget)) {
        return TError(TError::EType::INVALIDPARAM, "target: missing value");
    }

    Target = slotTarget->Value.GetString();

    Context = &ctx;
    IsYaAuto = clientInfo.IsYaAuto();
    IsAndroid = clientInfo.IsAndroid();
    IsIos = clientInfo.IsIOS();
    IsTouch = clientInfo.IsTouch();
    IsWindows = clientInfo.IsWindows();
    IsPornoSerp = false;

    Answer = ctx.CreateSlot(TStringBuf("navigation_results"), TStringBuf("navigation_results"));

    if (IsYaAuto) {
        return NAutomotive::HandleOpenSiteOrApp(*Context, Target);
    }

    TExitHandler atExit([this] { Postprocess(); });

    if (slotTarget->Type == TStringBuf("string")) {
        TContext::TSlot* slotTargetType = ctx.GetSlot(TStringBuf("target_type"));
        if (IsSlotEmpty(slotTargetType)) {
            TargetType = ETargetType::ETT_NONE;
        } else {
            TStringBuf targetTypeStr = slotTargetType->Value.GetString();
            if (targetTypeStr == TStringBuf("app"))
                TargetType = ETT_APPLICATION;
            else if (targetTypeStr == TStringBuf("site"))
                TargetType = ETT_SITE;
            else
                TargetType = ETT_NONE;
        }
    } else if (slotTarget->Type == TStringBuf("default_app")) {
        if (!ctx.ClientFeatures().SupportsOpenLink()) {
            ctx.AddAttention("nav_not_supported_on_device");
            return TResultValue();
        }
        return OpenNativeApp(Target);
    } else {
        return TError(TError::EType::INVALIDPARAM, "target: unknown type");
    }

    // 1. ищем по фикслисту
    TFixListResult result = AddFixList(ctx);
    if (result.HasResult) {
        Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "nav_fixlist");
        AddWindowsSoft();
        return result.ResultValue;
    }

    // 2. для Яндекс.Строки (или Алисы из браузера на десктопе) ещё и по списку приложений
    if (auto winApp = AddWindowsSoft()) {
        AddResult(winApp->Title, "", "", winApp->Url, false /* addSearchSuggest */, false /* addUriSuggest */);
        if (!winApp->Native) {
            Context->AddSearchSuggest(winApp->Title ? winApp->Title : winApp->NormName);
        }
        return TResultValue();
    }

    TCgiParameters cgi;
    cgi.InsertUnescaped("noreask", "1");

    bool regularSerp = true;

    if ((IsAndroid || IsIos) && TargetType == ETargetType::ETT_APPLICATION) {
        // 3. запрос "приложение XXX" ищём в вебе с ограничением по хосту (gplay/itunes)
        if (IsAndroid) {
            Query = TStringBuilder() << TStringBuf("site:play.google.com ") << Target;
            regularSerp = false;
        } else if (IsIos) {
            Query = TStringBuilder() << TStringBuf("site:itunes.apple.com ") << Target;
            regularSerp = false;
        } else {
            Query = Target;
        }
    } else {
        cgi.InsertUnescaped("rearr", "scheme_Local/BigNavAnswerUpper/InWhiteList=1"); // force BNO
        if (TargetType == ETargetType::ETT_SITE) {
            Query = TStringBuilder() << TStringBuf("сайт ") << Target;
        } else if (Query.empty()) {
            Query = Target;
        }
    }

    NSc::TValue searchResult;
    if (TResultValue error = NSerp::MakeRequest(Query, ctx, cgi, &searchResult, NAlice::TWebSearchBuilder::EService::BassNavigation)) {
        Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "nav_fetch_error");
        return error;
    }

    IsPornoSerp = NSerp::IsPornoSerp(searchResult);
    if (IsPornoSerp) {
        Context->AddAttention(TStringBuf("porno"), NSc::Null());
        Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "nav_porno");
        if (Context->GetContentRestrictionLevel() == EContentRestrictionLevel::Children) {
            Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "nav_nothing_found");
            return TResultValue();
        }
    }

    NSc::TArray& docs = searchResult["searchdata"]["docs"].GetArrayMutable();
    if (docs.empty()) {
        Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "nav_nothing_found");
        return TResultValue();
    }

    if (regularSerp) {
        // 4. BNO
        if (AddBno(docs[0]))
            return TResultValue();

        // 5. первый урл
        if (AddFirstUrl(searchResult))
            return TResultValue();
    } else {
        // 4. поиск по GPlay/iTunes
        if (AddAppFirstUrl(searchResult))
            return TResultValue();
    }

    Context->AddSearchSuggest(Query);
    Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "nav_nothing_found");
    return TResultValue();
}

TNavigationFormHandler::TFixListResult TNavigationFormHandler::AddFixList(TContext& ctx) {
    NSc::TValue data = TNavigationFixList::Instance()->Find(Target, *Context);
    if (!ctx.ClientFeatures().SupportsOpenLink()) {
        const TStringBuf changeForm = data["nav"]["app"]["quasar"]["change_form"].GetString();
        if (!changeForm.empty()) {
            ctx.SetResponseForm(changeForm, false /* setCurrentFormAsCallback */);
            return TFixListResult{.HasResult = true, .ResultValue = ctx.RunResponseFormHandler()};
        }
        ctx.AddAttention("nav_not_supported_on_device");
        return TFixListResult{.HasResult = true, .ResultValue = TResultValue()};
    }
    NSc::TValue nav = NAlice::CreateNavBlockImpl(data, Context->ClientFeatures());
    if (nav.IsNull())
        return TFixListResult{.HasResult = false, .ResultValue = TResultValue()};
    if (nav["turboapp"].GetBool() && ctx.ClientFeatures().SupportsOpenLinkTurboApp()) {
        ctx.GetAnalyticsInfoBuilder().AddAction(TString("open_turboapp_") + nav["url"].GetString(),
                                                "open_turboapp",
                                                TString("Открывается турбоапп ") + nav["text"].GetString());
        LOG(INFO) << "Opening turboapp" << Endl;
        AddResult(nav["text"].GetString(), nav["tts"].GetString(), TStringBuf(""), nav["url"].GetString());
    }
    if (TargetType == ETargetType::ETT_APPLICATION) {
        TString url = TString{nav["url"].GetString()};
        if (nav["app"].GetString()) {
            if (IsAndroid)
                url = NAlice::CreateGooglePlayAppUrl(nav["app"].GetString());
            else if (IsIos)
                url = NAlice::CreateITunesAppUrl(nav["app"].GetString());
        }
        AddResult(nav["text"].GetString(), nav["tts"].GetString(), nav["app"].GetString(), url);
    } else if (TargetType == ETargetType::ETT_SITE) {
        AddResult(nav["text"].GetString(), nav["tts"].GetString(), TStringBuf(""), nav["url"].GetString());
    } else {
        AddResult(nav["text"].GetString(), nav["tts"].GetString(), nav["app"].GetString(), nav["url"].GetString());
    }
    return TFixListResult{.HasResult = true, .ResultValue = TResultValue()};
}

TMaybe<NAlice::TNavigationFixList::TWindowsApp> TNavigationFormHandler::AddWindowsSoft() {
    if (IsWindows && TargetType != ETargetType::ETT_SITE) {
        if (auto winApp = TNavigationFixList::Instance()->FindWindowsApp(Target)) {
            NSc::TValue soft;
            soft["soft"] = winApp->NormName;
            if (winApp->Url)
                soft["url"] = winApp->Url;
            Context->AddCommand<TNavigationOpenSoftDirective>(TStringBuf("open_soft"), soft);

            if (winApp->Native) {
                NSc::TValue attention;
                attention["app"] = winApp->NormName;
                Context->AddAttention(TStringBuf("default_desktop_app"), attention);
            }

            Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "nav_winsoft");
            return winApp;
        }
    }
    return Nothing();
}

bool TNavigationFormHandler::AddBno(NSc::TValue& doc) {
    const NSc::TValue& bno = FindSnippet(doc, TStringBuf("bno"), ESS_CONSTRUCT);
    if (bno.IsNull())
        return false;

    TString text = RemoveHiLight(doc["doctitle"].GetString());
    if (text.empty()) {
        const NSc::TValue& generic = FindSnippet(doc, TStringBuf("generic"), ESS_SNIPPETS_MAIN);
        if (!generic.IsNull())
            text = RemoveHiLight(generic["headline"].GetString());
        if (!generic.IsNull() && text.empty())
            text = RemoveHiLight(generic.TrySelect("passages[0]").GetString());
    }

    if (text.empty()) {
        LOG(ERR) << "Cannot find text for bno: " << doc << Endl;
        return false;
    }

    TStringBuf url = doc["url"].GetString();
    TString app;

    if (TargetType != ETargetType::ETT_SITE) {
        if (IsAndroid)
            app = bno.TrySelect(TStringBuf("/mobile_apps/gplay/id")).GetString();
        else if (IsIos)
            app = bno.TrySelect(TStringBuf("/mobile_apps/itunes/id")).GetString();
        if (app) {
            Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "nav_bnoapp");
            LOG(DEBUG) << "bnoapp: " << app << Endl;
        }
        if (!app && (IsAndroid || IsIos)) {
            if (TStringBuf docId = doc["docid"].GetString()) {
                TStringBuf id = docId.RNextTok('-');
                if (auto bnoApp = TBnoApps::Instance()->Find(id, *Context)) {
                    app = *bnoApp;
                }
            }
        }
    }

    AddResult(text, TStringBuf(""), app, url);

    Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "nav_bno");
    return true;
}

bool TNavigationFormHandler::AddFirstUrl(NSc::TValue& searchResult) {
    NSc::TArray& docs = searchResult["searchdata"]["docs"].GetArrayMutable();

    for (size_t i = 0; i < docs.size(); ++i) {
        NSc::TValue& doc = docs[i];

        if (NSerp::IsInfectedDoc(doc)) {
            Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "nav_infected");
            LOG(WARNING) << "Skip infected document " << doc["url"] << Endl;
            continue;
        }

        TStringBuf serverDescr = doc[TStringBuf("server_descr")].GetString();

        if (serverDescr == TStringBuf("WEB")) {
            if (AddFirstUrlAppWiz(doc) || AddFirstUrlGeneric(doc, i, docs))
                return true;
            continue;
        }
        else if (serverDescr == TStringBuf("FAKE")) {
            TStringBuf reqid = searchResult.TrySelect("/reqdata/reqid").GetString();
            if (AddFirstUrlVideoWiz(doc) || AddFirstUrlImageWiz(doc, reqid) ||
                AddFirstUrlMapsWiz(doc)  || AddFirstUrlCompaniesWiz(doc) ||
                AddFirstUrlTvOnline(doc) || AddFirstUrlRasp(doc) ||
                AddFirstUrlYaStation(doc))
                return true;
        }
        else if (serverDescr == TStringBuf("TRANSLATE_PROXY")) {
            if (AddFirstUrlTranslate(doc))
                return true;
        } else if (serverDescr == TStringBuf("ENTITYSEARCH")) {
            if (AddFirstUrlObject(doc))
                return true;
            continue;
        } else if (serverDescr == TStringBuf("AUTO2")) {
            if (AddFirstUrlAutoRu(doc))
                return true;
        } else if (serverDescr == TStringBuf("MARKET")) {
            if (AddFirstUrlMarket(doc))
                return true;
        } else if (serverDescr == TStringBuf("WIZREPORTMISC")) {
            if (AddFirstUrlPanoramas(doc))
                return true;
        } else if (serverDescr == TStringBuf("REALTY")) {
            if (AddFirstUrlRealty(doc))
                return true;
        }

        LOG(ERR) << "Cant process document: " << doc << Endl;
        Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "nav_unknown_doc");
    }

    return false;
}

bool TNavigationFormHandler::AddFirstUrlGeneric(NSc::TValue& doc, size_t index, NSc::TArray& docs) {
    Y_ASSERT(doc["server_descr"].GetString() == TStringBuf("WEB"));

    struct TDocDescr {
        TStringBuf ServerDescr;
        TStringBuf Host;
        TStringBuf Glue;
        TStringBuf RedirectTo;
        TString Title;
        TStringBuf Url;
        TString AppId;
        double RelevPrediction = 0;
        double Relevance = 0;
    };

    static auto RedirectToFn = [](NSc::TValue& doc) {
        const NSc::TValue& redirect = FindSnippet(doc, TStringBuf("redir_snippet"), ESS_CONSTRUCT);
        return redirect.IsNull() ? TStringBuf("") : redirect["hostname_to"].GetString();
    };

    auto FillDocDescr = [this](NSc::TValue& doc) {
        TDocDescr descr;
        descr.ServerDescr = doc["server_descr"].GetString();
        descr.Host = doc["host"].GetString();
        descr.Glue = doc["markers"]["Glue"].GetString();
        descr.RedirectTo = RedirectToFn(doc);
        descr.Title = RemoveHiLight(doc["doctitle"].GetString());
        descr.Url = doc["url"].GetString();
        descr.AppId = TargetType == ETargetType::ETT_SITE ? TString() : ExtractAppId(descr.Url);
        descr.Relevance = doc["relevance"].ForceNumber();
        descr.RelevPrediction = doc["markers"]["RelevPrediction"].ForceNumber();
        return descr;
    };

    auto MakeResult = [this](const TDocDescr& doc, TStringBuf log) {
        LOG(DEBUG) << "[1st_url] " << log << Endl;
        AddResult(doc.Title, "", doc.AppId, doc.Url);
        Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "nav_first_doc");
        return true;
    };

    TDocDescr descr0 = FillDocDescr(doc);

    if (index >= docs.size()) {
        return MakeResult(descr0, "single answer");
    }

    TDocDescr descr1 = FillDocDescr(docs[index + 1]);

    if (descr0.RelevPrediction < 0.1 && descr1.RelevPrediction < 0.1) {
        LOG(DEBUG) << "[1st_url] too low relevs: " << descr0.RelevPrediction << ", " << descr1.RelevPrediction << Endl;
        return true;
    }

    if (descr0.RelevPrediction * 0.8 > descr1.RelevPrediction) {
        return MakeResult(descr0, "good answer");
    }
    if (descr1.RelevPrediction * 0.8 > descr0.RelevPrediction) {
        return MakeResult(descr1, "good answer");
    }

    if (descr1.ServerDescr != TStringBuf("WEB")) {
        return MakeResult(descr0, "2nd doc is not WEB");
    }

    if (descr0.RedirectTo == descr1.Host) {
        return MakeResult(descr0, "redirect");
    }

    if (descr0.Glue && descr0.Glue == descr1.Glue) {
        return MakeResult(descr0, "same host");
    }

    // doc[0] - official site
    // doc[1] - public group on vk, fb, youtube, twitter, ok
    static constexpr TStringBuf PGHOSTS[] = {TStringBuf("www.youtube.com"), TStringBuf("vk.com"),
                                             TStringBuf("vk.ru"), TStringBuf("twitter.com"),
                                             TStringBuf("ok.ru"), TStringBuf("www.facebook.com")};
    for (TStringBuf host : PGHOSTS) {
        if (descr0.Host != host && descr1.Host == host) {
            return MakeResult(descr0, "pg");
        }
        if (descr1.Host != host && descr0.Host == host) {
            return MakeResult(descr1, "pg");
        }
    }

    LOG(DEBUG) << "[1st_url] no good answer" << Endl;
    Context->AddAttention("nav_no_good_answer");

    if (Y_UNLIKELY(Context->HasExpFlag(TStringBuf("nav_debug")))) {
        TContext::TBlock* block = Context->Block();
        (*block)["name"] = "nav_debug";
        (*block)["type"] = "nav_debug";
        NSc::TValue relevs = (*block)["data"];
        for (const auto& d : {descr0, descr1}) {
            NSc::TValue& v = relevs.Push();
            v["title"] = d.Title;
            v["server_descr"] = d.ServerDescr;
            v["url"] = d.Url;
            v["host"] = d.Host;
            v["glue"] = d.Glue;
            v["relev"] = d.RelevPrediction;
            v["redir"] = d.RedirectTo;
        }
    }

    return true;
}

bool TNavigationFormHandler::AddFirstUrlAppWiz(NSc::TValue& doc) {
    const NSc::TValue& snippet = FindSnippet(doc, TStringBuf("app_search_view"), ESS_SNIPPETS_FULL);
    if (snippet.IsNull())
        return false;

    TStringBuf url = doc["url"].GetString();
    TString appId = (TargetType != ETargetType::ETT_SITE) ? ExtractAppId(url) : TString();

    const NSc::TValue& badge = snippet.TrySelect("/viewport/appbadgeCard/badge");
    TStringBuf text = badge.TrySelect("/title/text").GetString();
    AddResult(text, TStringBuf(""), appId, url);
    Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "nav_appwiz");
    return true;
}

bool TNavigationFormHandler::AddFirstUrlVideoWiz(NSc::TValue& doc) {
    static TVector<TStringBuf> snippetTypes = {TStringBuf("videowiz"), TStringBuf("video")};
    const NSc::TValue& snippet = FindAnySnippet(doc, snippetTypes, ESS_CONSTRUCT | ESS_SNIPPETS_FULL);
    if (snippet.IsNull())
        return false;
    const NSc::TValue& clip = snippet.TrySelect("clips[0]");
    if (Y_UNLIKELY(clip.IsNull())) {
        LOG(ERR) << "clips is empty: " << snippet << Endl;
        return false;
    }
    TString text = RemoveHiLight(clip["title"].GetString(doc["doctitle"].GetString()));
    if (Y_UNLIKELY(text.empty())) {
        LOG(ERR) << "Cant fetch title from " << doc << Endl;
        return false;
    }
    TStringBuf url = clip["clip_href"].GetString();
    if (Y_UNLIKELY(url.empty())) {
        LOG(ERR) << "Cant fetch url from " << doc << Endl;
        return false;
    }
    AddResult(text, TStringBuf(""), TStringBuf(""), url);
    Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "nav_video_wiz");
    return true;
}

bool TNavigationFormHandler::AddFirstUrlImageWiz(NSc::TValue& doc, TStringBuf reqId) {
    const NSc::TValue& snippet = FindSnippet(doc, TStringBuf("images"), ESS_SNIPPETS_FULL);
    if (snippet.IsNull())
        return false;
    TString title = RemoveHiLight(doc["doctitle"].GetString(Target));
    TString url = TStringBuilder() << ("https://yandex.ru/images/") << (IsTouch ? "touch/" : "")
                                   << "search?text=" << CGIEscapeRet(Target)
                                   << "&parent-reqid=" << reqId << "&p=0&source=wiz";
    AddResult(title, TStringBuf(""), TStringBuf(""), url);
    Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "nav_images_wiz");
    return true;
}

bool TNavigationFormHandler::AddFirstUrlMapsWiz(NSc::TValue& doc) {
    const NSc::TValue& snippet = FindSnippet(doc, TStringBuf("maps"), ESS_SNIPPETS_FULL);
    if (snippet.IsNull())
        return false;
    TStringBuf text = snippet["text"].GetString();
    TStringBuf url = doc["url"].GetString();
    if (!text.empty() && !url.empty()) {
        AddResult(text, TStringBuf(""), TStringBuf(""), url);
        Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "nav_maps_wiz");
        return true;
    }
    LOG(ERR) << "Cant parse snippet maps: " << snippet << Endl;
    return false;
}

bool TNavigationFormHandler::AddFirstUrlCompaniesWiz(NSc::TValue& doc) {
    const NSc::TValue& snippet = FindSnippet(doc, TStringBuf("companies"), ESS_SNIPPETS_FULL);
    if (snippet.IsNull())
        return false;
    const NSc::TValue& company = snippet.TrySelect("data/GeoMetaSearchData/features[0]/properties/CompanyMetaData");
    TStringBuf text = company["name"].GetString();
    TStringBuf url = company["url"].GetString();
    if (!text.empty() && !url.empty()) {
        AddResult(text, TStringBuf(""), TStringBuf(""), url);
        Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "nav_companies_wiz");
        return true;
    }
    LOG(ERR) << "Cant parse snippet companies: " << snippet << Endl;
    return false;
}

bool TNavigationFormHandler::AddFirstUrlTvOnline(NSc::TValue& doc) {
    {
        const NSc::TValue& snippet = FindSnippet(doc, TStringBuf("tv-online-channel"), ESS_CONSTRUCT);
        if (!snippet.IsNull()) {
            TStringBuf text = snippet["channel"]["title"].GetString();
            TStringBuf url = snippet["channel"]["url"].GetString();
            if (text && url) {
                AddResult(text, TStringBuf(""), TStringBuf(""), url);
                Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "nav_tv_online");
                return true;
            }
            LOG(ERR) << "Cant parse snippet tv-online-channel: " << snippet << Endl;
            return false;
        }
    }
    {
        const NSc::TValue& snippet = FindSnippet(doc, TStringBuf("tv-online-main"), ESS_CONSTRUCT);
        if (!snippet.IsNull()) {
            TStringBuf text = snippet["title"].GetString();
            TStringBuf url = doc["url"].GetString();
            if (text && url) {
                TString urlFull = url.StartsWith("//") ? TStringBuilder() << "https:" << url : TString{url};
                AddResult(text, TStringBuf(""), TStringBuf(""), urlFull);
                Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "nav_tv_online");
                return true;
            }
            LOG(ERR) << "Cant parse snippet tv-online-main: " << snippet << Endl;
            return false;
        }
    }
    return false;
}

bool TNavigationFormHandler::AddFirstUrlTranslate(NSc::TValue& doc) {
    const NSc::TValue& snippet = FindSnippet(doc, TStringBuf("translate"), ESS_SNIPPETS_FULL);
    if (snippet.IsNull())
        return false;
    TStringBuf text = doc["doctitle"].GetString();
    TString url = TString(doc["url"].GetString());
    if (url.find("translate.yandex") != TString::npos) {
        url = GenerateTranslateUri(*Context, "", "");
    }
    // todo: add deep link to the translate app
    AddResult(text, TStringBuf(""), TStringBuf(""), url);
    Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "nav_translate_wiz");
    return true;
}

bool TNavigationFormHandler::AddFirstUrlObject(NSc::TValue& doc) {
    const NSc::TValue& snippet = FindSnippet(doc, TStringBuf("entity_search"), ESS_SNIPPETS_FULL);
    if (snippet.IsNull())
        return false;

    const NSc::TValue& baseInfo = snippet.TrySelect("data/base_info");
    const NSc::TArray& subtypes = baseInfo["wsubtype"].GetArray();
    for (const auto& subtype : subtypes) {
        if (subtype.GetString() == TStringBuf("VideoGame@on")) {
            TStringBuf text = doc["doctitle"].GetString();
            TStringBuf tts = NSerp::GetVoiceTTS(snippet, "ru");
            TStringBuf appId;
            if (IsAndroid) {
                appId = baseInfo.TrySelect("ids/google_play").GetString();
            } else if (IsIos) {
                appId = baseInfo.TrySelect("ids/itunes").GetString();
            }
            TCgiParameters cgi;
            cgi.emplace(TString("entref"), TString{baseInfo["entref"].GetString()});
            TString url = GenerateSearchUri(Context, baseInfo["search_request"].GetString(text), cgi);
            AddResult(text, tts, appId, url);
            Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "search_oo_videogame");
            return true;
        }
    }

    return false;
}

bool TNavigationFormHandler::AddFirstUrlAutoRu(NSc::TValue& doc) {
    static TVector<TStringBuf> snippetTypes = {TStringBuf("autoru/thumbs-text"), TStringBuf("autoru/thumbs-price"),
                                               TStringBuf("autoru/links")};
    const NSc::TValue& snippet = FindAnySnippet(doc, snippetTypes, ESS_CONSTRUCT);
    if (snippet.IsNull())
        return false;
    // autoru/thumbs-price
    TString text = RemoveHiLight(snippet["title"]["text"].GetString());
    TStringBuf url = snippet["title"]["url"].GetString();
    if (text && url) {
        AddResult(text, TStringBuf(""), TStringBuf(""), url);
        Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "nav_autoru");
        return true;
    }
    LOG(ERR) << "Cant parse snippet autoru/xxx: " << snippet << Endl;
    return false;
}

bool TNavigationFormHandler::AddFirstUrlMarket(NSc::TValue& doc) {
    const NSc::TValue& snippet = FindSnippet(doc, TStringBuf("market_constr"), ESS_CONSTRUCT);
    if (snippet.IsNull())
        return false;
    TString text = RemoveHiLight(snippet["title"].GetString());
    TStringBuf url = IsTouch ? snippet["urlTouch"].GetString() : snippet["url"].GetString();
    if (text && url) {
        AddResult(text, "", "", url);
        Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "nav_market");
        return true;
    }
    LOG(ERR) << "Cant parse snippet market_constr: " << snippet << Endl;
    return false;
}

bool TNavigationFormHandler::AddFirstUrlPanoramas(NSc::TValue& doc) {
    const NSc::TValue& snippet = FindSnippet(doc, TStringBuf("panoramas"), ESS_SNIPPETS_FULL);
    if (snippet.IsNull())
        return false;
    TString text = RemoveHiLight(snippet["rasp"]["title"].GetString());
    TStringBuf url = IsTouch ? snippet["rasp"]["mobile_url"].GetString() : snippet["rasp"]["url"].GetString();
    if (text && url) {
        AddResult(text, "", "", url);
        Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "nav_panoramas");
        return true;
    }
    LOG(ERR) << "Cant parse snippet panoramas: " << snippet << Endl;
    return false;
}

bool TNavigationFormHandler::AddFirstUrlRasp(NSc::TValue& doc) {
    const NSc::TValue& snippet = FindSnippet(doc, TStringBuf("rasp"), ESS_CONSTRUCT);
    if (snippet.IsNull())
        return false;
    TString text = RemoveHiLight(doc["doctitle"].GetString());
    TStringBuf url = doc["url"].GetString();
    if (text && url) {
        AddResult(text, "", "", url);
        Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "nav_rasp");
        return true;
    }
    LOG(ERR) << "Cant parse snippet rasp: " << snippet << Endl;
    return false;
}

bool TNavigationFormHandler::AddFirstUrlRealty(NSc::TValue& doc) {
    const NSc::TValue& snippet = FindSnippet(doc, TStringBuf("realty/offers"), ESS_CONSTRUCT);
    if (snippet.IsNull())
        return false;
    TString text = RemoveHiLight(snippet["title"]["text"].GetString());
    TStringBuf url = snippet["title"]["url"].GetString();
    if (text && url) {
        TString urlFull = url.StartsWith("//") ? TStringBuilder() << "https:" << url : TString{url};
        AddResult(text, "", "", urlFull);
        Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "nav_realty");
        return true;
    }
    LOG(ERR) << "Cant parse snippet realty/offers: " << snippet << Endl;
    return false;
}

// обработка результата поиска по сайту GPlay/iTunes
bool TNavigationFormHandler::AddAppFirstUrl(NSc::TValue& searchResult) {
    NSc::TArray& docs = searchResult["searchdata"]["docs"].GetArrayMutable();

    if (docs.size() > 1) {
        TString appId0 = ExtractAppId(docs[0]["url"].GetString());
        if (appId0.empty()) {
            LOG(ERR) << "Cant extract app id from url " << docs[0]["url"] << Endl;
            return false;
        }

        TString appId1 = ExtractAppId(docs[1]["url"].GetString());
        if (appId0 == appId1) {
            TString text = RemoveHiLight(docs[0]["doctitle"].GetString());
            AddResult(text, TStringBuf(""), appId0, docs[0]["url"].GetString());
            Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "nav_appsearch");
            return true;
        }

        double relev0 = docs[0]["markers"]["RelevPrediction"].ForceNumber();
        double relev1 = docs[1]["markers"]["RelevPrediction"].ForceNumber();

        if (relev0 < 0.1 && relev1 < 0.1) {
            LOG(DEBUG) << "relevs is too low" << Endl;
            Context->AddAttention("nav_no_good_answer");
            return true;
        }

        if (relev0 * 0.8 > relev1) {
            TString text = RemoveHiLight(docs[0]["doctitle"].GetString());
            AddResult(text, TStringBuf(""), appId0, docs[0]["url"].GetString());
            LOG(DEBUG) << "[appsearch] prefer (" << appId0 << ", " << relev0 << ") to "
                       << "(" << appId1 << ", " << relev1 << ")" << Endl;
            Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "nav_appsearch");
            return true;
        }

        if (relev1 * 0.8 > relev0) {
            TString text = RemoveHiLight(docs[1]["doctitle"].GetString());
            AddResult(text, TStringBuf(""), appId1, docs[1]["url"].GetString());
            LOG(DEBUG) << "[appsearch] prefer (" << appId1 << ", " << relev1 << ") to "
                       << "(" << appId0 << ", " << relev0 << ")" << Endl;
            Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "nav_appsearch");
            return true;
        }

        // релевантности документов очень похожи, поэтому отправляем в поиск
        LOG(DEBUG) << "[appsearch] no good answer, "
                   << "(" << appId0 << ", " << relev0 << ") - "
                   << "(" << appId1 << ", " << relev1 << ")" << Endl;
        Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "nav_appsearch_approx_relevs");
        Context->AddAttention("nav_no_good_answer");
        return true;
    }

    if (docs.size() == 1) {
        double relev0 = docs[0]["markers"]["RelevPrediction"].ForceNumber();
        if (relev0 < 0.1) {
            LOG(DEBUG) << "relev0 is too low: " << relev0 << Endl;
            return false;
        }
        TString text = RemoveHiLight(docs[0]["doctitle"].GetString());
        TString appId = ExtractAppId(docs[0]["url"].GetString());
        AddResult(text, TStringBuf(""), appId, docs[0]["url"].GetString());
        LOG(DEBUG) << "[appsearch] single document, appId=" << appId << Endl;
        Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "nav_appsearch");
        return true;
    }

    return false;
}

bool TNavigationFormHandler::AddFirstUrlYaStation(NSc::TValue& doc) {
    TStringBuf host = doc.TrySelect("url_parts/hostname");
    if (host != TStringBuf("station.yandex.ru")) {
        return false;
    }
    TString text = RemoveHiLight(doc["doctitle"].GetString());
    TStringBuf url = doc["url"].GetString();
    if (text && url) {
        AddResult(text, TStringBuf(""), TStringBuf(""), url);
        Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "nav_yastation");
        return true;
    }
    return false;
}

void TNavigationFormHandler::AddResult(TStringBuf text, TStringBuf tts, TStringBuf app, TStringBuf url,
                                       bool addSearchSuggest, bool addUriSuggest)
{
    NSc::TValue appUri;
    appUri["uri"] = app.empty() ? url : GenerateApplicationUri(*Context, app, url);

    NSc::TValue result;
    if (url) {
        NUri::TUri uri;
        if (uri.Parse(url, NUri::TUri::TFeature::FeaturesRecommended) == NUri::TUri::ParsedOK) {
            result["text"] = TStringBuilder() << text << "\\n" << uri.GetHost();
        } else {
            result["text"] = text;
        }
    } else {
        result["text"] = text;
    }
    result["tts"] = tts;
    if (IsPornoSerp) {
        result["url"] = url;
    }
    Answer->Value.Swap(result);

    if (addUriSuggest && !IsPornoSerp) {
        if (app.empty()) {
            Context->AddCommand<TNavigationOpenSiteDirective>(TStringBuf("open_uri"), appUri);
        } else {
            Context->AddCommand<TNavigationOpenAppOrSiteDirective>(TStringBuf("open_uri"), appUri);
        }
        Context->AddSuggest(TStringBuf("open_site_or_app__open"), appUri.Clone());
    }

    if (addSearchSuggest)
        Context->AddSearchSuggest(Query ? Query : Target);
}

TResultValue TNavigationFormHandler::OpenNativeApp(TString name) {
    Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "nav_native_app");

    TMaybe<NAlice::TNavigationFixList::TNativeApp> app = TNavigationFixList::Instance()->FindNativeApp(name);
    if (app && !app->Empty(Context->MetaClientInfo())) {
        if (IsAndroid) {
            AddResult(app->AndroidTitle, TStringBuf(""), TStringBuf(""), app->AndroidUri, false);
            return {};
        }
        if (IsIos) {
            AddResult(app->IosTitle, TStringBuf(""), TStringBuf(""), app->IosUri, false);
            return {};
        }
    }

    // https://st.yandex-team.ru/DIALOG-2208
    if (name == "camera" && TComputerVisionMainHandler::IsSupported(*Context)) {
        Context->AddIrrelevantAttention(
            /* relevantIntent= */ TStringBuf("personal_assistant.scenarios.image_what_is_this"),
            /* reason= */ TStringBuf("https://st.yandex-team.ru/DIALOG-2208"));
        return {};
    }

    NSc::TValue attention;
    attention["app"] = name;
    Context->AddAttention(TStringBuf("unknown_app"), attention);

    return {};
}

TString TNavigationFormHandler::ExtractAppId(TStringBuf url) const {
    NUri::TUri uri;
    NUri::TState::EParsed ps = uri.Parse(url, NUri::TFeature::FeaturesDefault | NUri::TFeature::FeatureSchemeKnown);
    if (ps != NUri::TState::EParsed::ParsedOK) {
        LOG(ERR) << "Cant parse app url " << url << Endl;
        return TString();
    }

    if (IsAndroid) {
        // https://play.google.com/store/apps/details?id=com.whatsapp
        TCgiParameters cgi(uri.GetField(NUri::TField::FieldQuery));
        return cgi.Get("id");
    }

    if (IsIos) {
        // https://itunes.apple.com/ru/app/telegram-messenger/id686449807?mt=8
        TStringBuf path(uri.GetField(NUri::TField::FieldPath));
        return ToString(path.RNextTok('/'));
    }

    return TString();
}

void TNavigationFormHandler::Postprocess() {
    if (!Answer)
        return;
    if (Context->ClientFeatures().SupportsNavigator() && Context->FormName() != FORM_NAME_DO_OPEN) {
        // do not open anything without confirmation in Navigator
        Context->DeleteCommand(TStringBuf("open_uri"));
        Context->DeleteCommand(TStringBuf("open_soft"));
        Context->AddAttention("ask_confirmation");
        return;
    }

    if (TContext::TBlock* openSoftCmd = Context->GetCommand(TStringBuf("open_soft"))) {
        if (TContext::TBlock* openUriCmd = Context->GetCommand(TStringBuf("open_uri"))) {
            if ((*openSoftCmd)["data"]["url"].IsNull()) {
                // добавляем урл из поиска только если нет своего урла из данных
                (*openSoftCmd)["data"]["url"].Swap((*openUriCmd)["data"]["uri"]);
            }
            Context->DeleteCommand(TStringBuf("open_uri"));
        }
        Context->DeleteSuggest(TStringBuf("open_site_or_app__open"));
   }
}

// static
TContext::TPtr TNavigationFormHandler::SetAsResponse(TContext& ctx, bool callbackSlot, TStringBuf text) {
    TContext::TPtr newCtx = ctx.SetResponseForm(FORM_NAME, callbackSlot);
    Y_ENSURE(newCtx);
    if (!text) {
        text = newCtx->Meta().Utterance();
    }
    newCtx->CreateSlot("target", "string", true, NSc::TValue(text));
    return newCtx;
}

void TNavigationFormHandler::Register(THandlersMap* handlers) {
    auto cb = []() {
        return MakeHolder<TNavigationFormHandler>();
    };

    handlers->emplace(FORM_NAME, cb);
    handlers->emplace(FORM_NAME_DO_OPEN, cb);
    handlers->emplace(FORM_NAME_ELLIPSIS, cb);
}

}
