#include "newsdata.h"

#include <alice/bass/forms/context/context.h>
#include <alice/bass/libs/config/config.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <extsearch/geo/kernel/normalize/normalize.h>

#include <util/generic/stack.h>
#include <util/generic/maybe.h>

namespace NBASS {
namespace NNews {

namespace {

constexpr TStoragePosition INDEX_RUBRIC_ID = 0;
constexpr TStoragePosition KUREZY_RUBRIC_ID = 777;

NSc::TValue GetKurezyRubricData() {
    NSc::TValue data;
    data["id"] = KUREZY_RUBRIC_ID;
    data["alias"] = KUREZY_RUBRIC;
    data["parent_id"] = INDEX_RUBRIC_ID;

    NSc::TValue& issues = data["issues"].SetArray();
    issues.AppendAll({"ru"});

    NSc::TValue& urls = data["urls"].SetDict();
    urls["ru"] = "/kurezy.html";
    return data;
}

NSc::TValue GetSmiData() {
    struct TNewsDataSmi {
        TStringBuf aid;
        TStringBuf id;
        TStringBuf url;
        TStringBuf name;
        TVector<TStringBuf> rubrics;
        TStringBuf logo;
    };
    static const TNewsDataSmi SMI_DATA[] = {
        {"1574", "dtf", "http://www.dtf.ru/", "DTF", {COMPUTERS_RUBRIC, GAMES_RUBRIC}, "https://avatars.mds.yandex.net/get-dialogs/1530877/27fa1df8480201c6e6ab/orig"},  // ди ти ф ру
        {"1040", "gazetaru", "https://www.gazeta.ru", "Газета ru", {DEFAULT_INDEX_RUBRIC}, "https://avatars.mds.yandex.net/get-dialogs/1530877/9dc1e6d3e3341386f857/orig"},  // газета ру
        {"1048", "kommersant", "https://www.kommersant.ru/", "Коммерсантъ", {DEFAULT_INDEX_RUBRIC}, "https://avatars.mds.yandex.net/get-dialogs/399212/78acb67257c8603b888f/orig"},
        {"1227", "kp", "https://www.kp.ru", "Комсомольская правда", {}, "https://yastatic.net/s3/dialogs/smart_displays/news/smi/kp.png"},
        {"1047", "lenta", "http://lenta.ru", "Лента ru", {DEFAULT_INDEX_RUBRIC}, "https://avatars.mds.yandex.net/get-dialogs/1017510/0264b110f0adee5cf254/orig"},
        {"12694", "lifenewsru", "http://life.ru", "Life ru", {}, "https://avatars.mds.yandex.net/get-dialogs/1017510/b967725e0f952d7aeb80/orig"},  // лайф ру
        {"1429", "mk", "https://www.mk.ru", "Московский Комсомолец", {}, "https://avatars.mds.yandex.net/get-dialogs/758954/26117c573f4e07853987/orig"},
        {"254125454", "news.rambler", "http://rambler.ru/", "Рамблер", {}, "https://avatars.mds.yandex.net/get-dialogs/1530877/dea95ce15b7e465ef84b/orig"},
        {"254125230", "nplus1", "https://nplus1.ru/", "N plus 1", {SCIENCE_RUBRIC}, "https://avatars.mds.yandex.net/get-dialogs/758954/d04e76f69ebf93b5b7e5/catalogue-icon-x4"},
        {"1002", "ria", "https://www.ria.ru", "РИА Новости", {DEFAULT_INDEX_RUBRIC}, "https://avatars.mds.yandex.net/get-dialogs/1017510/9148e839952a979d8e67/orig"},
        {"1027", "rbc", "http://www.rbc.ru", "РБК", {DEFAULT_INDEX_RUBRIC}, "https://avatars.mds.yandex.net/get-dialogs/1525540/20355d1bbb8fbe8c2969/orig"},
        {"254067402", "vcru", "https://vc.ru/", "vc ru", {}, "https://yastatic.net/s3/dialogs/smart_displays/news/smi/vc.png"},  // ви си ру
    };

    NSc::TValue result;
    result.SetArray();
    for (const TNewsDataSmi& smi : SMI_DATA) {
        NSc::TValue data;
        data["aid"] = smi.aid;
        data["alias"] = smi.id;
        data["name"] = smi.name;
        auto& rubrics = data["rubrics"].SetArray();
        for (const auto& rubric : smi.rubrics) {
            rubrics.Push(rubric);
        }
        data["url"] = smi.url;
        data["logo"] = smi.logo;

        result.Push(std::move(data));
    }
    return result;
}

template <typename T>
THashSet<NGeobase::TId> ConstructIssues(const NSc::TArray& srcIssues, const TIssue::TTldMap& tld2issue, const T& objId) {
    THashSet<NGeobase::TId> issues;
    for (const NSc::TValue& i : srcIssues) {
        const NGeobase::TId* id = tld2issue.FindPtr(i.GetString());
        if (!id) {
            LOG(ERR) << "News update: region/rubric '" << objId << "' unable to find issue '" << i.GetString() << "' in export";
            continue;
        }

        issues.insert(*id);
    }

    return issues;
}

TMaybe<TNewsData> ConstructNewsData(const NSc::TValue& data) {
    // collect issues
    NNews::TIssue::TStorage issues;
    TString defaultCountry = data["default_country"].ForceString();
    TMaybe<TStoragePosition> defaultIssueIdx;
    for (const auto& kv : data["issues"].GetDict()) {
        if (kv.second["hidden"].GetBool(false)) {
            continue;
        }

        NGeobase::TId issueId = NGeobase::UNKNOWN_REGION;
        if (!TryFromString<NGeobase::TId>(kv.first, issueId) || !NAlice::IsValidId(issueId)) {
            LOG(ERR) << "NewsDataUpdate: unable to parse categ id for issue with key: " << kv.first << Endl;
            continue;
        }

        if (kv.first == defaultCountry) {
            defaultIssueIdx = issues.size();
        }
        issues.emplace_back(kv.second);
    }

    if (defaultIssueIdx) {
        return MakeMaybe<TNewsData>(std::move(issues), *defaultIssueIdx);
    }

    return Nothing();
}

struct TLanguageInfo {
    TStringBuf Language;
    TStringBuf IndexRubricFullName;
};

const TLanguageInfo LANGUAGES_INFO[] = {{"ru", "Главные новости"}, {"uk", "Головне новини"}};

} // namespace

// static
TNewsData TNewsData::Create(IGlobalContext& ctx) {
    TDummySourcesRegistryDelegate delegate;
    TSourceRequestFactory sourceRequestFactory{ctx.Sources().NewsApiScheduler, ctx.Config(), TStringBuf("/newsdata_base"),
                                               delegate};

    NHttpFetcher::TRequestPtr req = sourceRequestFactory.Request();
    if (Y_UNLIKELY(!req)) {
        ythrow yexception() << "[NewsDataUpdater] Unable to create request for NewsApi";
    }

    NHttpFetcher::THandle::TRef handle = req->Fetch();
    if (Y_UNLIKELY(!handle)) {
        ythrow yexception() << "[NewsDataUpdater] Unable to obtain handle for request NewsApi";
    }

    NHttpFetcher::TResponse::TRef resp = handle->Wait();
    if (!resp) {
        ythrow yexception() << "[NewsDataUpdater] Response is empty after request NewsApi";
    }

    if (resp->IsError()) {
        ythrow yexception() << "[NewsDataUpdater] Response error: " << resp->GetErrorText() << ", [" << resp->Code
                            << "] " << resp->Data;
    }

    NSc::TValue data = NSc::TValue::FromJson(resp->Data);
    TMaybe<TNewsData> newsDataFromJson = ConstructNewsData(data);
    if (Y_UNLIKELY(!newsDataFromJson)) {
        ythrow yexception() << "[NewsDataUpdater] Unable to update newsdata";
    }

    auto& newsData = *newsDataFromJson;
    TIssue::TTldMap tld2issue;
    for (const auto& kv : data["domain_to_issue"].GetDict()) {
        tld2issue.emplace(kv.first, kv.second.ForceIntNumber(NGeobase::UNKNOWN_REGION));
    }

    // collect regions for making urls
    for (const auto& kv : data["regions"].GetDict()) {
        NGeobase::TId id = kv.second["id"].ForceIntNumber(NGeobase::UNKNOWN_REGION);
        if (!NAlice::IsValidId(id)) {
            continue;
        }

        newsData.GeosMap.emplace(std::piecewise_construct,
                                 std::forward_as_tuple(id),
                                 std::forward_as_tuple(id, kv.second, tld2issue));
    }

    // collect rubrics and create a tree!
    struct TItemDescr {
        i64 Id;
        i64 ParentId;
        const NSc::TValue& Data;
    };
    THashMap<i64, TItemDescr> rubrics;
    THashMap<i64, TVector<i64>> tree;

    auto addRubric = [&rubrics, &tree](const NSc::TValue& data) {
        i64 id = data["id"].ForceIntNumber(-1);
        i64 parentId = data["parent_id"].ForceIntNumber(-1);

        rubrics.insert({id, {id, parentId, data}});
        if (parentId != id) {
            tree[parentId].push_back(id);
        }
    };

    for (const auto& [id, rubric] : data["rubrics"].GetDict()) {
        TStringBuf alias = rubric["alias"].GetString();
        // we don't need daily news right now, may be later
        // since quotes is not a real rubric, so we don't need it too
        if (TStringBuf("daily") == alias || TStringBuf("quotes") == alias) {
            continue;
        }
        addRubric(rubric);
    }

    // DIALOG-6276: Add 'koronavirus' rubric. It's may be temporary, so check is existed and enabled.
    for (const auto& [id, rubric] : data["special_rubrics"].GetDict()) {
        if (rubric["alias"].GetString() != COVID_RUBRIC) {
            continue;
        }
        const auto extState = rubric.TrySelect("state/external").GetString();
        if (!extState.empty() && extState != "off") {
            addRubric(rubric);
        }
        break;
    }

    // DIALOG-5992: News doesn't return "kurezy" rubric in newsdata_base. Add to HashMap and use only with exp flag.
    const NSc::TValue kurezyData = GetKurezyRubricData();
    addRubric(kurezyData);

    // this is a rubrics tree creation procedure!!
    // get current 'todo' (means list of children, by default there is only one child and it is 0 which is always
    // top) then go through all the children, add them to the rubricsStorage (vector) and then use the index from this
    // storage to connect rubricsMap (it is a rubric alias to rubric index in rubricsStorage vector), and also moves
    // all the children of current child into 'todo' stack in the end of every 'todo' we sort all children vector in
    // the created TRubric object by Order (which comes from news api)
    struct TTodo {
        TTodo() : ChildrenIds({INDEX_RUBRIC_ID}) {}
        TTodo(TStoragePosition parentIdx, TVector<i64>&& childrenIds) : ParentIdx(parentIdx), ChildrenIds(std::move(childrenIds)) {}
        TMaybe<TStoragePosition> ParentIdx;
        TVector<i64> ChildrenIds;
    };
    TStack<TTodo> todoList({ TTodo() });
    while (todoList) {
        TTodo todo = std::move(todoList.top());
        todoList.pop();

        for (i64 childId : todo.ChildrenIds) {
            const TItemDescr* child = rubrics.FindPtr(childId);
            if (!child) {
                continue;
            }

            TStoragePosition childIdx = newsData.Rubrics.size();
            TStoragePosition parentIdx = todo.ParentIdx.GetOrElse(childIdx);
            if (parentIdx != childIdx) {
                newsData.Rubrics[parentIdx].AppendChildrenIndex(childIdx);
            }
            newsData.Rubrics.emplace_back(child->Data, childIdx, parentIdx, tld2issue);
            newsData.RubricsAliasMap.emplace(newsData.Rubrics.back().Alias(), childIdx);

            auto it = tree.find(childId);
            if (tree.cend() != it) {
                if (it->second.size()) {
                    todoList.emplace(childIdx, std::move(it->second));
                }
                tree.erase(it);
            }
        }

        if (todo.ParentIdx) {
            // all children have already been inserted, so sort them!
            newsData.Rubrics[*todo.ParentIdx].SortChildrenIndexes(newsData.Rubrics);
        }
    }

    // Create smi.
    const NSc::TValue smiData = GetSmiData();
    for (const auto& smi : smiData.GetArray()) {
        const TStoragePosition idx = newsData.Smi.size();
        newsData.Smi.emplace_back(smi);
        newsData.SmiAliasMap.emplace(newsData.Smi.back().GetAlias(), idx);

        for (const auto& rubric : smi["rubrics"].GetArray()) {
            newsData.RubricToSmiMap[rubric.GetString()].push_back(newsData.Smi.back());
        }
    }

    LOG(INFO) << "[NewsDataUpdater] Updated successfully! rubrics: " << newsData.Rubrics.size()
              << ", regions: " << newsData.GeosMap.size() << Endl;
    return std::move(newsData);
}

void TNewsData::MakeSuggests(TContext& ctx, const TIssue& issue, const TRubric& rubric) const {
    rubric.AddSuggests(ctx, issue, Rubrics);
}

void TNewsData::MakeSuggests(TContext& ctx) const {
    const auto* defaultRubric = RubricByAlias(DEFAULT_INDEX_RUBRIC);
    Y_ENSURE(defaultRubric, "Index rubric must always exist");
    defaultRubric->AddSuggests(ctx, FallbackIssue(), Rubrics);
}

TString TNewsData::UrlFor(const TIssue& issue, NGeobase::TId srcGeo, bool onlyForIssue) const {
    const TGeoRegion* geo = GeosMap.FindPtr(srcGeo);
    if (!geo) {
        return TString();
    }
    // CreateUrl just take hostname from Issue for example "|news.yandex.ru|/Greece/index.html", but API
    // in 'ru' doesn't have data for Greece. When arg is true, check if requested issue in Issues list for geo.
    if (onlyForIssue && !geo->IsInIssue(issue)) {
        return TString();
    }
    return geo->CreateUrl(issue);
}

const TStoragePosition* TNewsData::GetIssueStorageIndex(const NGeobase::TLookup& geobase, NGeobase::TId geoId) const {
    for (auto id = geoId; NAlice::IsValidId(id); id = geobase.GetParentId(id)) {
        if (const auto* idx = IssuesMap.FindPtr(id)) {
            return idx;
        }
    }
    return nullptr;
}

const TIssue& TNewsData::IssueByGeo(const NGeobase::TLookup& geobase, NGeobase::TId geoId) const {
    const TStoragePosition* issue_idx = GetIssueStorageIndex(geobase, geoId);
    return issue_idx ? Issues[*issue_idx] : Issues[DefaultIssueIdx];
}

bool TNewsData::HasIssueForGeo(const NGeobase::TLookup& geobase, NGeobase::TId geoId) const {
    return GetIssueStorageIndex(geobase, geoId) != nullptr;
}

TString TNewsData::UrlFor(const TIssue& issue, TStringBuf rubric) const {
    const TStoragePosition* idx = RubricsAliasMap.FindPtr(rubric);
    if (!idx) {
        return TString();
    }

    return Rubrics[*idx].CreateUrl(issue);
}

TIssue::TIssue(const NSc::TValue& issue)
    : Id(issue["country_id"].ForceIntNumber(NGeobase::UNKNOWN_REGION))
    , Hostname(issue["hostname"].GetString())
    , Domain(issue["domain"].GetString())
{
}

TGeoRegion::TGeoRegion(NGeobase::TId geoid, const NSc::TValue& region, const TIssue::TTldMap& tld2issue)
    : UrlPath(region["urls"]["ru"].GetString()) // ru is ok
    , Issues(ConstructIssues(region["issues"].GetArray(), tld2issue, geoid))
{
}

TString TGeoRegion::CreateUrl(const TIssue& issue) const {
    return TStringBuilder() << TStringBuf("https://") << issue.Hostname << UrlPath;
}

bool TGeoRegion::IsInIssue(const TIssue& issue) const {
    return Issues.contains(issue.Id);
}

TSmi::TSmi(const NSc::TValue& data)
    : Aid(data["aid"].GetString())
    , AliasName(data["alias"].GetString())
    , Name(data["name"].GetString())
    , Url(data["url"].GetString())
    , Logo(data["logo"].GetString())
{
}

TRubric::TRubric(const NSc::TValue& data, TStoragePosition idx, TStoragePosition parentIdx,
                 const TIssue::TTldMap& tld2issue)
    : Idx(idx)
    , ParentIdx(parentIdx)
    , Id(data["id"].ForceIntNumber(-1))
    , Order(data["order"].ForceIntNumber(-1))
    , UrlPath(
          data["urls"]["ru"].GetString()) // "ru" is normal because it is main and real url is making by using issue
    , AliasName(data["alias"].GetString())
    , Issues(ConstructIssues(data["issues"].GetArray(), tld2issue, Id))
{
    if (IsSpecial()) {
        // Don't add Names for special rubrics, they are not used in suggests.
        return;
    }

    // ALICE-2825: Fill suggests text.
    auto getByKey = [&data](const TStringBuf lang, const TStringBuf suffix) {
        return data[TString::Join(lang, suffix)].GetString();
    };

    for (const auto& [lang, indexRubricName] : LANGUAGES_INFO) {
        const TString shortName(getByKey(lang, "_name"));
        // Try different keys for full name.
        TString fullName;
        // Base value for 'index' is bad, change manually.
        if (IsIndex()) {
            fullName = indexRubricName;
        } else {
            fullName = NGeosearch::CapitalizeFirstLetter(getByKey(lang, "_name_abl"));
            if (fullName.empty()) {
                fullName = getByKey(lang, "_name_genitive_full");
            }
            if (fullName.empty()) {
                fullName = TString::Join("Новости ", shortName);
            }
        }
        Names.emplace(std::piecewise_construct, std::forward_as_tuple(lang),
                      std::forward_as_tuple(TName{shortName, fullName}));
    }
}

TStringBuf TRubric::ShortName(TStringBuf lang) const {
    const TName* name = Names.FindPtr(lang);
    return name ? name->ShortName : TStringBuf();
}

TString TRubric::CreateUrl(const TIssue& issue) const {
    return TStringBuilder() << TStringBuf("https://") << issue.Hostname << UrlPath;
}

bool TRubric::IsInIssue(const TIssue& issue) const {
    return Issues.contains(issue.Id);
}

void TRubric::SortChildrenIndexes(TStorage& rubrics) {
    auto compCb = [&rubrics](TStoragePosition lhs, TStoragePosition rhs) {
        return rubrics[lhs].Order < rubrics[rhs].Order;
    };
    Sort(Children.begin(), Children.end(), compCb);
}

bool TRubric::IsSpecial() const {
    return Id == KUREZY_RUBRIC_ID || AliasName == COVID_RUBRIC;
}

void TRubric::AddSuggest(TContext& ctx, const TIssue& issue) const {
    // Don't add special rubrics to suggests.
    if (!Issues.contains(issue.Id) || IsSpecial()) {
        return;
    }

    const TName* name = Names.FindPtr(ctx.MetaLocale().Lang);
    if (!name) {
        name = Names.FindPtr(TStringBuf("ru"));
        if (!name) {
            return;
        }
    }

    NSc::TValue json;
    json["alias"].SetString(AliasName);
    // ALICE-2825: Use FullName, because voice query "Спорт" doesn't trigger news scenario.
    json["utterance"].SetString(name->FullName);
    json["caption"].SetString(name->FullName);

    TContext::TSlot topicSlot(SLOT_TOPIC, SLOT_TYPE_TOPIC);
    topicSlot.Value.SetString(AliasName);

    NSc::TValue formUpdate;
    formUpdate["name"].SetString(MAIN_FORM_NAME);
    formUpdate["slots"].SetArray().Push(topicSlot.ToJson(nullptr));
    formUpdate["resubmit"].SetBool(true);
    ctx.AddSuggest(TStringBuf("get_news__rubric"), std::move(json), std::move(formUpdate));
}

void TRubric::AddSuggests(TContext& ctx, const TIssue& issue, const TStorage& rubrics, const TStoragePosition* exceptionIdx) const {
    // always add index rubric (means global top5) in case current suggest is not for top5
    if (Idx && !exceptionIdx) {
        rubrics[0].AddSuggest(ctx, issue);
    }

    if (Children) {
        for (TStoragePosition childIdx : Children) {
            if (exceptionIdx && childIdx == *exceptionIdx) {
                continue;
            }

            rubrics[childIdx].AddSuggest(ctx, issue);
        }
    }
    else if (ParentIdx != Idx) {
        rubrics[ParentIdx].AddSuggests(ctx, issue, rubrics, &Idx);
    }
}

// static
TNewsData::TIssuesMap TNewsData::ConstructIssuesMap(const TIssue::TStorage& issues) {
    TIssuesMap im;
    for (TStoragePosition i = 0, total = issues.size(); i < total; ++i) {
        im.emplace(issues[i].Id, i);
    }
    return im;
}

TNewsData::TNewsData(TIssue::TStorage&& issues, TStoragePosition defaultIssueIdx)
    : Issues(std::move(issues))
    , IssuesMap(ConstructIssuesMap(Issues))
    , DefaultIssueIdx(defaultIssueIdx)
{
    Y_ASSERT(defaultIssueIdx < Issues.size());
}

const TIssue* TNewsData::IssueForRubric(TStringBuf alias, const TIssue& issue) const {
    const TStoragePosition* rubricIdx = RubricsAliasMap.FindPtr(alias);
    if (!rubricIdx) {
        return nullptr;
    }

    const TRubric& rubric = Rubrics[*rubricIdx];
    if (rubric.IsInIssue(issue)) {
        return &issue;
    }

    if (rubric.IsInIssue(Issues[DefaultIssueIdx])) {
        return &Issues[DefaultIssueIdx];
    }

    return nullptr;
}

const TRubric* TNewsData::RubricByAlias(TStringBuf alias) const {
    const TStoragePosition* idx = RubricsAliasMap.FindPtr(alias);
    if (idx) {
        return &Rubrics[*idx];
    }

    return nullptr;
}

const TSmi* TNewsData::SmiByAlias(TStringBuf alias) const {
    const TStoragePosition* idx = SmiAliasMap.FindPtr(alias);
    if (idx) {
        return &Smi[*idx];
    }

    return nullptr;
}

const TVector<TSmi>* TNewsData::SmisByRubric(TStringBuf rubric) const {
    return RubricToSmiMap.FindPtr(rubric);
}

} // namespace NNews
} // namespace NBASS
