#pragma once

#include <alice/bass/libs/globalctx/globalctx.h>

#include <alice/library/geo/geodb.h>

#include <library/cpp/scheme/scheme.h>

#include <util/datetime/base.h>
#include <util/generic/hash_set.h>
#include <util/generic/hash.h>
#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <util/system/spinlock.h>

namespace NBASS {

class TContext;

namespace NNews {

inline constexpr TStringBuf MAIN_FORM_NAME = "personal_assistant.scenarios.get_news";
inline constexpr TStringBuf MAIN_ALT_FORM_NAME = "personal_assistant.scenarios.get_free_news";
inline constexpr TStringBuf MORE_NEWS_FORM_NAME = "personal_assistant.scenarios.get_news__more";
inline constexpr TStringBuf ELLIPSIS_NEWS_FORM_NAME = "personal_assistant.scenarios.get_news__ellipsis";
inline constexpr TStringBuf COLLECT_CARDS_FORM_NAME = "alice.centaur.collect_cards";
inline constexpr TStringBuf COLLECT_TEASERS_PREVIEW_FORM_NAME = "alice.centaur.collect_teasers_preview";
inline constexpr TStringBuf COLLECT_MAIN_SCREEN_FORM_NAME = "alice.centaur.collect_main_screen.widgets.news";
inline constexpr TStringBuf COLLECT_WIDGET_GALLERY_FORM_NAME = "alice.centaur.collect_widget_gallery";
// Rubrics.
inline constexpr TStringBuf COMPUTERS_RUBRIC = "computers";
inline constexpr TStringBuf COVID_RUBRIC = "koronavirus";
inline constexpr TStringBuf DEFAULT_INDEX_RUBRIC = "index";
inline constexpr TStringBuf GAMES_RUBRIC = "games";
inline constexpr TStringBuf KUREZY_RUBRIC = "kurezy";
inline constexpr TStringBuf SCIENCE_RUBRIC = "science";
// Slots.
inline constexpr TStringBuf SLOT_IS_DEFAULT_REQUEST = "is_default_request";
inline constexpr TStringBuf SLOT_MAX_COUNT = "max_count";
inline constexpr TStringBuf SLOT_NEWS = "news";
inline constexpr TStringBuf SLOT_NEWS_MEMENTO = "news_memento";
inline constexpr TStringBuf SLOT_RESOLVED_WHERE = "resolved_where";
inline constexpr TStringBuf SLOT_SMI = "smi";
inline constexpr TStringBuf SLOT_TOPIC = "topic";
inline constexpr TStringBuf SLOT_TYPE_TOPIC = "news_topic";
inline constexpr TStringBuf SLOT_TYPE_HW_TOPIC = "custom.news_topic";
inline constexpr TStringBuf SLOT_TYPE_MEMENTO_TOPIC = "memento.news_topic";
inline constexpr TStringBuf SLOT_WHERE = "where";
inline constexpr TStringBuf SLOT_WHERE_ID = "where_id";

inline constexpr TStringBuf AVATAR_NS = "news";
inline constexpr TStringBuf LAST_EPOCH_FIELD = "last_epoch";
inline constexpr TStringBuf NO_ICON_NAME = "No_pic";
inline constexpr TStringBuf TURBO_ICON_NAME = "Turbo_Icon";

inline constexpr TStringBuf NEWS_STRING_RUS = "новости";
inline constexpr TStringBuf PERSONAL_FEED_URL = "https://news.yandex.ru/personal_feed.html";

inline constexpr TStringBuf ERROR_NEWS_ENDED_MSG = "No more news available";
inline constexpr TStringBuf ERROR_NO_SERP_NEWS = "SERP wizard return no news";

//memento fields
constexpr TStringBuf MEMENTO_IS_ONBOARDED = "is_onboarded";
constexpr TStringBuf MEMENTO_RESULT = "result";
constexpr TStringBuf MEMENTO_RUBRIC = "rubric";
constexpr TStringBuf MEMENTO_SOURCE = "source";

constexpr TStringBuf MEMENTO_RESULT_EMPTY = "empty";
constexpr TStringBuf MEMENTO_RESULT_ERROR = "error";
constexpr TStringBuf MEMENTO_RESULT_SUCCESS = "success";

constexpr TStringBuf MEMENTO_SOURCE_DEFAULT = "default";
constexpr TStringBuf MEMENTO_RUBRIC_DEFAULT = "index";
constexpr TStringBuf MEMENTO_IS_MEMENTABLE_REQUEST_TOPIC = "is_mementable_request_topic";

// Attentions.
inline constexpr TStringBuf PERSONAL_NEWS_ATTENTION = "personal_news";
inline constexpr TStringBuf NEWS_CONTINUOUS_ATTENTION = "news_continuous_request";
inline constexpr TStringBuf NEWS_ENDED_ATTENTION = "news_ended";
inline constexpr TStringBuf TOP_NEWS_ATTENTION = "top_news";

// Experiments flags.
inline constexpr TStringBuf ALICE_DEFAULT_PERSONAL_EXP_FLAG = "alice_personal_news";
inline constexpr TStringBuf ALICE_FOREIGN_NEWS_DISABLED_EXP_FLAG = "alice_foreign_news_disabled";
inline constexpr TStringBuf ALICE_FUNNY_NEWS_EXP_FLAG = "alice_funny_news";
inline constexpr TStringBuf ALICE_MORE_NEWS_PROMO_EXP_FLAG = "alice_more_news_promo";
inline constexpr TStringBuf ALICE_NEWS_DISABLE_CHANGE_SOURCE_POSTROLL_EXP_FLAG = "alice_news_disable_change_source_postroll";
inline constexpr TStringBuf ALICE_NEWS_DISABLE_CHANGE_SOURCE_PUSH_EXP_FLAG = "alice_news_disable_change_source_push";
inline constexpr TStringBuf ALICE_NEWS_ENABLE_WIZARD = "alice_news_enable_wizard";
inline constexpr TStringBuf ALICE_NEWS_NO_CONTINOUS_ATTENTION_EXP_FLAG = "alice_news_no_continous_attention";
inline constexpr TStringBuf ALICE_NEWS_NO_GEO_WIZARD_EXP_FLAG = "alice_news_no_geo_wizard";
inline constexpr TStringBuf ALICE_NEWS_REMOVE_SEARCH_SUGGEST_EXP_FLAG = "alice_news_remove_search_suggest";
inline constexpr TStringBuf ALICE_NEWS_WIZARD_NO_FLTR_EXPR_EXP_FLAG = "alice_news_wizard_no_filter_expr";
// Remove all promo and postrolls for e2e
inline constexpr TStringBuf ALICE_NO_PROMO_EXP_FLAG = "alice_no_promo";
inline constexpr TStringBuf ALICE_SHORT_NEWS_EXP_FLAG = "alice_station_short_news";
// Similarity of title and snippet can reach 1.0 threshold for some stories. Add flag for disabling feature.
inline constexpr TStringBuf ALICE_SKIP_TITLE_EXP_DISABLED_FLAG = "alice_news_skip_title_disabled";
inline constexpr TStringBuf ALICE_SMI_NEWS_DISABLED_EXP_FLAG = "alice_smi_news_disabled";
inline constexpr TStringBuf ALICE_STANTION_NEWS_RUBRIC_PROMO_EXP_FLAG = "alice_station_news_rubric_promo";
// TODO: check if legacy flag is used.
inline constexpr TStringBuf PA_NEWS_REMOVE_FILTER_EXP_FLAG = "pa_news_rmfilt";

inline constexpr TStringBuf DEFAULT_RUBRIC_EXP_PREFIX = "alice_news_default_rubric=";
inline constexpr TStringBuf NEWS_MAX_COUNT_EXP_PREFIX = "alice_news_max_count=";
inline constexpr TStringBuf PERSONAL_ACTIONS_THRESHOLD_EXP_PREFIX = "alice_news_personal_actions_threshold=";
inline constexpr TStringBuf SKIP_TITLE_THRESHOLD_EXP_FLAG_PREFIX = "alice_news_skip_title_threshold=";
inline constexpr TStringBuf SPEAKER_SPEED_EXP_PREFIX = "alice_news_speaker_speed=";

// Promo and postroll exp prefixes
inline constexpr TStringBuf ONBOARDING_PROBA_FIRST_EXP_PREFIX = "alice_news_onboarding_first_proba=";
inline constexpr TStringBuf ONBOARDING_PROBA_DEFAULT_FULL_EXP_PREFIX = "alice_news_onboarding_default_full_proba=";
inline constexpr TStringBuf ONBOARDING_PROBA_DEFAULT_PART_EXP_PREFIX = "alice_news_onboarding_default_part_proba=";

inline constexpr TStringBuf POSTROLL_PROBA_FIRST_EXP_PREFIX = "alice_news_postroll_first_proba=";
inline constexpr TStringBuf POSTROLL_PROBA_DEFAULT_RESPONSE_EXP_PREFIX = "alice_news_postroll_default_proba=";
inline constexpr TStringBuf POSTROLL_NEW_TOPIC_RESPONSE_EXP_PREFIX = "alice_news_postroll_new_topic_proba=";
inline constexpr TStringBuf POSTROLL_PROBA_DEFAULT_AFTER_SET_EXP_PREFIX = "alice_news_postroll_after_set_proba=";

// News types in analytic_info block.
inline constexpr TStringBuf NEWS_DEFAULT_TYPE = "top";
inline constexpr TStringBuf NEWS_ENDED_TYPE = "news_ended";
inline constexpr TStringBuf NEWS_GEO_TYPE = "geo";
inline constexpr TStringBuf NEWS_KUREZY_TYPE = "kurezy";
inline constexpr TStringBuf NEWS_PERSONAL_TYPE = "personal";
inline constexpr TStringBuf NEWS_RUBRIC_TYPE = "rubric";
inline constexpr TStringBuf NEWS_SMI_TYPE = "smi";
inline constexpr TStringBuf NEWS_WIZARD_TYPE = "wizard";

inline constexpr TStringBuf POSTROLL_NEWS_CHANGE_SOURCE_MODE = "news_change_source_postroll_mode";
inline constexpr TStringBuf POSTROLL_NEWS_RADIO_NEWS_CHANGE_SOURCE_MODE = "news_radio_news_change_source_postroll_mode";
inline constexpr TStringBuf PROMO_MORE_NEWS_MODE = "more_news_promo_mode";
inline constexpr TStringBuf PROMO_MORE_NEWS_BEFORE_MODE = "more_news_promo_before_mode";
inline constexpr TStringBuf PROMO_STATION_NEWS_RUBRIC = "station_news_rubric_promo_mode";

inline constexpr TStringBuf ONBOARDING_MODE_FULL = "full";
inline constexpr TStringBuf ONBOARDING_MODE_PART = "part";

inline constexpr TStringBuf PERSONAL_RUBRIC = "personal";
// Default value of minimum actions number to have a relevant news recommendations.
inline constexpr int DEFAULT_PERSONAL_ACTIONS_THRESHOLD = 7;
// Default value for 'alice_news_skip_title_threshold' feature.
inline constexpr float DEFAULT_SKIP_TITLE_THRESHOLD = 0.5;
// How 2 news from SMI request should be similar to drop one of them
inline constexpr float SKIP_DUPS_THRESHOLD = 0.7;

// Max allowed size of exclude_ids array. 25 requests by 6 items, and 30 by 5.
inline constexpr size_t MAX_EXCLUDE_IDS_SIZE = 150;
// max number of news output
inline constexpr size_t MAX_NEWS = 5;
// DIALOG-6507: Max number of news for speakers.
inline constexpr size_t SPEAKER_MAX_NEWS = 7;
// DIALOG-6130: Skip intro and promo for a repeated news request only in 5 minutes period.
inline constexpr i64 SECONDS_TO_SKIP_INTRO = 60 * 5;

// DIALOG-7018: API request presets.
static const TString API_PRESET_GEO = "alice_geo";
static const TString API_PRESET_PERSONAL = "alice_personal";
static const TString API_PRESET_RUBRIC = "alice_rubric";
static const TString API_PRESET_SMI = "alice_smi";

// just to express that this is the index within a storage which is usually a vector
using TStoragePosition = size_t;

class TIssue {
public:
    using TStorage = TVector<TIssue>;
    using TTldMap = THashMap<TStringBuf, NGeobase::TId>;

public:
    explicit TIssue(const NSc::TValue& issue);

    const NGeobase::TId Id;
    const TString Hostname;
    const TString Domain;
};

/** It represents a geographical region object from newsdata export.
 * It contains a set of issues which have a certain region.
 */
class TGeoRegion {
public:
    TGeoRegion(NGeobase::TId id, const NSc::TValue& region, const TIssue::TTldMap& tld2issue);

    TString CreateUrl(const TIssue& issue) const;

    bool IsInIssue(const TIssue& issue) const;

private:
    const TString UrlPath;
    const THashSet<NGeobase::TId> Issues;
};

class TSmi final {
public:
    using TStorage = TVector<TSmi>;
    // alias name to index in TStorage
    using TAliasMap = THashMap<TStringBuf, TStoragePosition>;

public:
    TSmi(const NSc::TValue& data);

    const TString& GetAid() const {
        return Aid;
    }

    const TString& GetAlias() const {
        return AliasName;
    }

    const TString& GetName() const {
        return Name;
    }

    const TString& GetUrl() const {
        return Url;
    }

    const TString& GetLogo() const {
        return Logo;
    }

private:
    const TString Aid;
    const TString AliasName;
    const TString Name;
    const TString Url;
    const TString Logo;
};

class TRubric {
public:
    // just a plain vector of rubrics without hierarchy
    using TStorage = TVector<TRubric>;
    // alias name to index in TStorage
    using TAliasMap = THashMap<TStringBuf, TStoragePosition>;

public:
    /** Create a rubric with json value and main + parent index in TStorage.
     */
    TRubric(const NSc::TValue& rubric, TStoragePosition idx, TStoragePosition parentIdx, const TIssue::TTldMap& tld2issue);

    void AddSuggests(TContext& ctx, const TIssue& issue, const TStorage& rubrics, const TStoragePosition* exceptionIdx = nullptr) const;

    void SortChildrenIndexes(TStorage& rubrics);
    void AppendChildrenIndex(TStoragePosition idx) {
        Children.push_back(idx);
    }

    TString CreateUrl(const TIssue& issue) const;
    bool IsInIssue(const TIssue& issue) const;

    bool IsIndex() const {
        return DEFAULT_INDEX_RUBRIC == AliasName;
    }

    TStringBuf Alias() const {
        return AliasName;
    }

    TStringBuf ShortName(TStringBuf lang) const;

private:
    void AddSuggest(TContext& ctx, const TIssue& issue) const;
    bool IsSpecial() const;

private:
    const TStoragePosition Idx;
    const TStoragePosition ParentIdx;
    const i64 Id;
    const i64 Order;
    const TString UrlPath;
    const TString AliasName;
    const THashSet<NGeobase::TId> Issues;

    struct TName {
        const TString ShortName;
        const TString FullName;
    };
    THashMap<TStringBuf, TName> Names;
    // just a indexes to real vector's objects
    TVector<TStoragePosition> Children;
};

class TNewsData : private NNonCopyable::TMoveOnly {
private:
    // Map for regions
    using TGeosMap = THashMap<NGeobase::TId, TGeoRegion>;
    // Map for issue id (which is actually a geo id) to index in TIssue::TStorage
    using TIssuesMap = THashMap<NGeobase::TId, TStoragePosition>;
    // Map for rubric name to smi
    using TRubricToSmiMap = THashMap<TString, TVector<TSmi>>;

    // just a vector of issues
    const TIssue::TStorage Issues;
    // geoid to issue idx (in Issues)
    const TIssuesMap IssuesMap;
    // default (in according to newsdata export) Issue (index, see <Issues>)
    const TStoragePosition DefaultIssueIdx;

    // real storage for rubrics
    TRubric::TStorage Rubrics;
    // rubric alias (ie politics) to rubric idx (in Rubrics)
    TRubric::TAliasMap RubricsAliasMap;
    // Storage for smi.
    TSmi::TStorage Smi;
    // Smi alias (ie habr) to idx in |Smi|.
    TSmi::TAliasMap SmiAliasMap;
    TRubricToSmiMap RubricToSmiMap;
    // dict of geoid to region
    TGeosMap GeosMap;

private:
    const TStoragePosition* GetIssueStorageIndex(const NGeobase::TLookup& geobase, NGeobase::TId geoId) const;

    static TIssuesMap ConstructIssuesMap(const TIssue::TStorage& issues);

public:
    TNewsData(TIssue::TStorage&& issues, TStoragePosition defaultIssueIdx);
    TNewsData(TNewsData&&) = default;

    static TNewsData Create(IGlobalContext& globalCtx);

    void MakeSuggests(TContext& ctx, const TIssue& issue, const TRubric& rubric) const;
    void MakeSuggests(TContext& ctx) const;

    /** Try to find an issue for rubric by its alias.
     * First it checks if the rubric is in the given <issue>, then if it is false checks the default
     * issue. So return either the given issue or default or nullptr!
     */
    const TIssue* IssueForRubric(TStringBuf alias, const TIssue& issue) const;

    const TRubric* RubricByAlias(TStringBuf alias) const;
    const TSmi* SmiByAlias(TStringBuf alias) const;
    const TVector<TSmi>* SmisByRubric(TStringBuf rubric) const;

    const TIssue& IssueByGeo(const NGeobase::TLookup& geobase, NGeobase::TId geoId) const;
    bool HasIssueForGeo(const NGeobase::TLookup& geobase, NGeobase::TId geoId) const;

    /** Returns a default issue which is obtained from newsdata export.
     * Actually it is Russian
     */
    const TIssue& FallbackIssue() const {
        return Issues[DefaultIssueIdx];
    }

    /** Generates url for geo
     */
    TString UrlFor(const TIssue& issue, NGeobase::TId geo, bool onlyForIssue = false) const;

    /** Creates url for rubric
     */
    TString UrlFor(const TIssue& issue, TStringBuf rubric) const;
};

} // namespace NNews
} // namespace NBASS
