#include "news.h"

#include <alice/bass/forms/common/personal_data.h>
#include <alice/bass/forms/directives.h>
#include <alice/bass/forms/player/player.h>
#include <alice/bass/forms/remember_address.h>
#include <alice/bass/forms/search/serp.h>
#include <alice/bass/forms/urls_builder.h>
#include <alice/bass/libs/avatars/avatars.h>
#include <alice/bass/libs/config/config.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/scheduler/scheduler.h>

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/experiments/flags.h>

#include <kernel/geodb/countries.h>
#include <kernel/lemmer/core/language.h>

#include <library/cpp/neh/neh.h>
#include <library/cpp/neh/http_common.h>
#include <library/cpp/resource/resource.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/string_utils/quote/quote.h>

#include <util/charset/utf8.h>
#include <util/datetime/base.h>
#include <util/generic/algorithm.h>
#include <util/generic/ptr.h>
#include <util/generic/serialized_enum.h>
#include <util/generic/vector.h>
#include <util/stream/file.h>
#include <util/string/join.h>
#include <util/system/fs.h>
#include <util/system/hp_timer.h>
#include <util/system/tempfile.h>

#include <regex>

namespace NBASS::NNews {
namespace {

constexpr float ONBOARDING_PROBA_FIRST = 1;
constexpr float ONBOARDING_PROBA_DEFAULT_FULL = 0.5;
constexpr float ONBOARDING_PROBA_DEFAULT_PART = 0.1;

constexpr float POSTROLL_PROBA_FIRST = 1;
constexpr float POSTROLL_PROBA_DEFAULT_RESPONSE = 0.05;
constexpr float POSTROLL_NEW_TOPIC_RESPONSE = 0.1;
constexpr float POSTROLL_PROBA_DEFAULT_AFTER_SET = 0.0;

const std::regex IMAGE_SIZE_REGEX("[0-9]+x[0-9]+");

const TRubric* GetRubricFromExperiments(const TContext& ctx, const TNewsData& newsData) {
    auto customRubric = ctx.GetValueFromExpPrefix(DEFAULT_RUBRIC_EXP_PREFIX);
    if (customRubric && !customRubric->empty()) {
        return newsData.RubricByAlias(*customRubric);
    }
    return nullptr;
}

bool IsCustomTopicSlot(const TContext::TSlot* slotTopic) {
    return !IsSlotEmpty(slotTopic) && slotTopic->Type == "string";
}

bool IsForeignRequest(TContext& ctx) {
    const auto& geobase = ctx.GlobalCtx().GeobaseLookup();
    return !geobase.IsIdInRegion(ctx.UserRegion(), NGeoDB::RUSSIA_ID);
}

bool IsRubricSlot(const TContext::TSlot* slotTopic, TMaybe<TStringBuf> rubricName = Nothing()) {
    if (IsSlotEmpty(slotTopic) ||
        !EqualToOneOf(slotTopic->Type, SLOT_TYPE_TOPIC, SLOT_TYPE_HW_TOPIC, SLOT_TYPE_MEMENTO_TOPIC))
    {
        return false;
    }
    if (rubricName) {
        return slotTopic->Value.GetString() == *rubricName;
    }
    return true;
}

bool IsAllowedRubric(const TContext& ctx, const TContext::TSlot* slotTopic) {
    if (!IsRubricSlot(slotTopic)) {
        return false;
    }
    // DIALOG-5992: Added "kurezy" to tagger and newsdata, but return only with exp flag.
    // DIALOG-6276: "koronavirus" is temporary, added to tagger but return only with exp flag.
    const TStringBuf value = slotTopic->Value.GetString();
    if (value == KUREZY_RUBRIC) {
        return ctx.HasExpFlag(ALICE_FUNNY_NEWS_EXP_FLAG);
    } else if (value == PERSONAL_RUBRIC) {
        // Personal news processed before rubric logic, because of special request in TPersonalRequest class. If this
        // condition reached - exp disabled or user is cold, return false to answer default rubric.
        return false;
    }
    return true;
}

const NNews::TIssue& ConstructIssue(TContext& ctx) {
    const auto& newsData = ctx.GlobalCtx().NewsData();
    return newsData.IssueByGeo(ctx.GlobalCtx().GeobaseLookup(), ctx.UserRegion());
}

const TRubric& ConstructRubric(TContext& ctx, const TContext::TSlot* slotTopic) {
    const auto& newsData = ctx.GlobalCtx().NewsData();
    if (IsAllowedRubric(ctx, slotTopic)) {
        // If rubric is tagged, but missed in newsdata - return default.
        if (const auto* rubric = newsData.RubricByAlias(slotTopic->Value.GetString())) {
            return *rubric;
        }
    }

    // Check experiments for custom default rubric feature.
    if (const auto* rubric = GetRubricFromExperiments(ctx, newsData)) {
        return *rubric;
    }
    const auto* defaultRubric = newsData.RubricByAlias(DEFAULT_INDEX_RUBRIC);
    Y_ENSURE(defaultRubric, "Expect newsdata always has 'index' rubric");
    return *defaultRubric;
}

NSc::TValue ConstructMementoValue(const TContext& ctx) {
    auto mementoSlot = ctx.GetSlot(NNews::SLOT_NEWS_MEMENTO, "news_memento");
    NSc::TValue mementoValue;
    if (mementoSlot != nullptr) {
        NSc::TValue::FromJson(mementoValue, mementoSlot->Value.GetString());
    } else {
        LOG(WARNING) << "empty memento value" << Endl;
    }
    return mementoValue;
}

TMaybe<float> GetSkipTitleExpThreshold(const TContext& ctx) {
    // Don't compare if return only titles or the feature is disabled.
    if (ctx.HasExpFlag(ALICE_SHORT_NEWS_EXP_FLAG) || ctx.HasExpFlag(ALICE_SKIP_TITLE_EXP_DISABLED_FLAG)) {
        return Nothing();
    }
    // Remove repeatable title threshold.
    auto thresholdFlagValue = ctx.GetValueFromExpPrefix(SKIP_TITLE_THRESHOLD_EXP_FLAG_PREFIX);
    float threshold = 0.0;
    // Validate value. Expect float in range (0:1].
    if (thresholdFlagValue && !thresholdFlagValue->empty() && TryFromString<float>(*thresholdFlagValue, threshold) &&
        threshold > 0.0 && threshold <= 1.0) {
        return threshold;
    }
    return DEFAULT_SKIP_TITLE_THRESHOLD;
}

void AddAdditionalSuggests(TContext& ctx) {
    // DIALOG-6340: For exp, try to remove search suggest from first position.
    if (!ctx.HasExpFlag(ALICE_NEWS_REMOVE_SEARCH_SUGGEST_EXP_FLAG)) {
        ctx.AddSearchSuggest();
    }
    ctx.AddOnboardingSuggest();
}

TString CreateLogoUrl(TStringBuf logo) {
    if (logo.empty() && !logo.Contains("/")) {
        LOG(WARNING) << "Cannot create news agency logo url" << Endl;
        return "";
    }
    TStringBuf left;
    TStringBuf lastTok;
    logo.RSplit('/', left, lastTok);
    if (!lastTok.empty() && std::regex_match(lastTok.data(), IMAGE_SIZE_REGEX)) {
        logo.RNextTok('/');
    }
    return TStringBuilder{} << TStringBuf("https:") << logo << "/orig";
}

class TScaleSize {
public:
    bool operator<(const TScaleSize& r) const {
        return Scale < r.Scale;
    }

    static void CreateImage(double scaleFactor, const NSc::TValue& image,
        NSc::TValue* dst, TStringBuf dstName = TStringBuf("image"))
    {
        TStringBuf imageUrl = image["url"].GetString();
        auto it = std::lower_bound(Scales.cbegin(), Scales.cend() - 1, TScaleSize(scaleFactor));
        const size_t size = it->Scale * MagicMultiplierForImages;

        imageUrl.RNextTok('/');
        TStringBuilder newImageUrl;
        if (!imageUrl.StartsWith("http")) {
            newImageUrl << TStringBuf("https:");
        }
        newImageUrl << imageUrl << '/' << size << 'x' << size;

        NSc::TValue imageJson = (*dst)[dstName];
        imageJson["src"].SetString(newImageUrl);
        imageJson["height"].SetIntNumber(size);
        imageJson["width"].SetIntNumber(size);
        imageJson["orig_height"].SetIntNumber(image["height"].GetIntNumber());
        imageJson["orig_width"].SetIntNumber(image["width"].GetIntNumber());
    }

private:
    TScaleSize(double scale)
        : Scale(scale)
    {
    }

private:
    const double Scale;
    static const TVector<TScaleSize> Scales;
    // https://st.yandex-team.ru/HOME-40931#1512743993000
    static constexpr size_t MagicMultiplierForImages = 60;
};

const TVector<TScaleSize> TScaleSize::Scales{ 1.0, 1.5, 2, 3, 3.5, 4.0 };

} // anonymous namespace

// ---------------------------------------------------------------- TRequestType

TRequestType::TRequestType(TContext& ctx)
    : Ctx(ctx)
    , NewsData(ctx.GlobalCtx().NewsData())
    , Issue(ConstructIssue(ctx))
    , Rubric(ConstructRubric(ctx, ctx.GetSlot(SLOT_TOPIC)))
    , MaxNewsCount(GetMaxNewsCount())
{
}

void TRequestType::AddTurboIcon(NSc::TValue* dst) const {
    const TAvatar* avatar = Ctx.Avatar(AVATAR_NS, TURBO_ICON_NAME);
    if (avatar) {
        NSc::TValue imageJson = (*dst)["turbo"];
        imageJson["src"].SetString(avatar->Https);
    }
}

bool TRequestType::IsContinuousRequest() const {
    // News ended attention means better start from begin (will do different logic later).
    return !Ctx.HasAttention(NEWS_ENDED_ATTENTION) && Ctx.FormName() == MORE_NEWS_FORM_NAME;
}

void TRequestType::SetNoIcon(NSc::TValue* dst) const {
    const TAvatar* avatar = Ctx.Avatar(AVATAR_NS, NO_ICON_NAME);
    if (avatar) {
        NSc::TValue imageJson = (*dst)["image"];
        imageJson["src"].SetString(avatar->Https);
    }
}

void TRequestType::SaveRequestTime(NSc::TValue& dst) const {
    if (!Ctx.Meta().HasEpoch()) {
        return;
    }
    const i64 epoch = Ctx.Meta().Epoch();
    if (epoch > 0) {
        dst[LAST_EPOCH_FIELD] = epoch;
    }
}

TMaybe<TInstant> TryParseTime(TStringBuf timeStr) {
    TInstant resultTimestamp;
    if (TInstant::TryParseIso8601(timeStr, resultTimestamp)) {
        return resultTimestamp;
    }

    LOG(ERR) << "Cannot parse time: '" << timeStr << '\'' << Endl;
    return Nothing();
}

size_t TRequestType::GetMaxNewsCount() const {
    size_t maxCount = 0;
    if (const TContext::TSlot* slot = Ctx.GetSlot(SLOT_MAX_COUNT)) {
        if (!IsSlotEmpty(slot) && slot->Type == "num") {
            if (slot->Value.IsString() && TryFromString<size_t>(slot->Value.GetString(), maxCount)) {
                return maxCount;
            }
            return slot->Value.GetIntNumber(1);
        }
    }
    auto maxCountFlagValue = Ctx.GetValueFromExpPrefix(NEWS_MAX_COUNT_EXP_PREFIX);
    if (maxCountFlagValue && !maxCountFlagValue->empty() && TryFromString<size_t>(*maxCountFlagValue, maxCount) &&
        maxCount > 0) {
        return maxCount;
    }
    return Ctx.MetaClientInfo().IsSmartSpeaker() ? SPEAKER_MAX_NEWS : MAX_NEWS;
}

void TRequestType::AddDivCard() {
    if (!Ctx.ClientFeatures().SupportsDivCards()) {
        return;
    }

    const TContext::TSlot* slot = Ctx.GetSlot(SLOT_NEWS, "news");
    if (Y_UNLIKELY(!slot)) {
        LOG(ERR) << "no news output slot found... no divcard" << Endl;
        return;
    }

    Ctx.AddDivCardBlock("news", slot->Value);
}

float TRequestType::GetExpValueByPrefix(TStringBuf prefix, float def) const {
    float value = 0;
    auto flagValue = Ctx.GetValueFromExpPrefix(prefix);
    if (flagValue && !flagValue->empty() && TryFromString<float>(*flagValue, value)) {
        return value;
    }
    return def;
}

bool TRequestType::IsPostrollableRequest(const NSc::TValue& mementoValue) const
{
    TContext::TSlot* topicSlot = Ctx.GetSlot(NNews::SLOT_TOPIC);
    TContext::TSlot* whereSlot = Ctx.GetSlot(NNews::SLOT_WHERE);

    bool hasTopic = !IsSlotEmpty(topicSlot) &&
        topicSlot->Type != SLOT_TYPE_MEMENTO_TOPIC;
    bool hasWhere = !IsSlotEmpty(whereSlot);

    bool isDefault = !hasTopic && !hasWhere;

    //If need to send push but it is forbidden, don't send
    if (Ctx.HasExpFlag(ALICE_NEWS_DISABLE_CHANGE_SOURCE_PUSH_EXP_FLAG) && !mementoValue[MEMENTO_IS_MEMENTABLE_REQUEST_TOPIC].GetBool()) {
        return false;
    }

    bool isSameTopic = hasTopic &&
        mementoValue[MEMENTO_RESULT] == MEMENTO_RESULT_SUCCESS &&
        (topicSlot->Value.GetString() == mementoValue[MEMENTO_SOURCE]
            || topicSlot->Value.GetString() == mementoValue[MEMENTO_RUBRIC]);
    return (Ctx.MetaClientInfo().IsSmartSpeaker() || Ctx.MetaClientInfo().IsTvDevice()) &&
        !Ctx.HasExpFlag(ALICE_NEWS_DISABLE_CHANGE_SOURCE_POSTROLL_EXP_FLAG) &&
        mementoValue[MEMENTO_RESULT] != MEMENTO_RESULT_ERROR &&
        !hasWhere &&
        (isDefault || !isSameTopic);
}

bool IsMementoDefault(NSc::TValue mementoValue) {
    return  mementoValue[MEMENTO_SOURCE] == MEMENTO_SOURCE_DEFAULT &&
            (mementoValue[MEMENTO_RUBRIC].IsNull() || mementoValue[MEMENTO_RUBRIC] == MEMENTO_RUBRIC_DEFAULT);
}

void TRequestType::AddPostNewsPhraseId() {
    if (Ctx.HasExpFlag(ALICE_NO_PROMO_EXP_FLAG) ||
        Ctx.HasAttention(NEWS_CONTINUOUS_ATTENTION) && !Ctx.HasExpFlag(NAlice::NExperiments::EXP_NEWS_DISABLE_RUBRIC_API))
    {
        return;
    }

    TContext::TSlot* slot = Ctx.GetSlot(NNews::SLOT_NEWS, "news");

    if (Y_UNLIKELY(!slot)) {
        LOG(ERR) << "no news output slot found... no post message" << Endl;
        return;
    }

    TContext::TSlot* mementoSlot = Ctx.GetSlot(NNews::SLOT_NEWS_MEMENTO, "news_memento");
    NSc::TValue mementoValue;
    if (Y_UNLIKELY(!mementoSlot)) {
        LOG(ERR) << "no memento output slot found... no post message" << Endl;
        return;
    }
    NSc::TValue::FromJson(mementoValue, mementoSlot->Value.GetString());

    TContext::TSlot* isDefaultRequestSlot = Ctx.GetSlot(NNews::SLOT_IS_DEFAULT_REQUEST, "is_default_request");
    const bool isDefaultRequest = !IsSlotEmpty(isDefaultRequestSlot) && isDefaultRequestSlot->Value.IsTrue();

    TContext::TSlot* topicSlot = Ctx.GetSlot(NNews::SLOT_TOPIC);
    const bool isMainNewsRequest = !IsSlotEmpty(topicSlot) && topicSlot->Value.GetString() == DEFAULT_INDEX_RUBRIC;

    if (Ctx.HasExpFlag(NAlice::NExperiments::EXP_NEWS_DISABLE_RUBRIC_API) && (isDefaultRequest || isMainNewsRequest) && GetNewsType() == NEWS_SMI_TYPE) {
        const auto randValue = Ctx.GetRng().RandomDouble();

        float onboardingProba = 0;
        const auto onboardingMode = mementoValue[MEMENTO_RESULT] == MEMENTO_RESULT_EMPTY || mementoValue[MEMENTO_SOURCE] == MEMENTO_SOURCE_DEFAULT
            ? ONBOARDING_MODE_FULL
            : ONBOARDING_MODE_PART;
        if (!mementoValue[MEMENTO_IS_ONBOARDED].IsTrue() && onboardingMode == ONBOARDING_MODE_FULL) {
            onboardingProba = GetExpValueByPrefix(ONBOARDING_PROBA_FIRST_EXP_PREFIX, ONBOARDING_PROBA_FIRST);
        } else if (onboardingMode == ONBOARDING_MODE_FULL) {
            onboardingProba = GetExpValueByPrefix(ONBOARDING_PROBA_DEFAULT_FULL_EXP_PREFIX, ONBOARDING_PROBA_DEFAULT_FULL);
        } else if (isDefaultRequest) {
            onboardingProba = GetExpValueByPrefix(ONBOARDING_PROBA_DEFAULT_PART_EXP_PREFIX, ONBOARDING_PROBA_DEFAULT_PART);
        }

        if (randValue < onboardingProba) {
            slot->Value["onboarding_mode"] = onboardingMode;
            // Onboarding and POSTROLL_NEWS_CHANGE_SOURCE_MODE should not go together
            return;
        }
    }

    if (Ctx.HasAttention(NEWS_CONTINUOUS_ATTENTION)) {
        return;
    }

    const auto randValue = Ctx.GetRng().RandomDouble();

    float postrollProba = 0;
    if (IsPostrollableRequest(mementoValue))
    {
        if (mementoValue[MEMENTO_RESULT] == MEMENTO_RESULT_EMPTY) {
            postrollProba = GetExpValueByPrefix(POSTROLL_PROBA_FIRST_EXP_PREFIX, POSTROLL_PROBA_FIRST);
        } else if (mementoValue[MEMENTO_IS_MEMENTABLE_REQUEST_TOPIC].GetBool()) {
            postrollProba = GetExpValueByPrefix(POSTROLL_NEW_TOPIC_RESPONSE_EXP_PREFIX, POSTROLL_NEW_TOPIC_RESPONSE);
        } else if (IsMementoDefault(mementoValue)) {
            postrollProba = GetExpValueByPrefix(POSTROLL_PROBA_DEFAULT_RESPONSE_EXP_PREFIX, POSTROLL_PROBA_DEFAULT_RESPONSE);
        } else {
            postrollProba = GetExpValueByPrefix(POSTROLL_PROBA_DEFAULT_AFTER_SET_EXP_PREFIX, POSTROLL_PROBA_DEFAULT_AFTER_SET);
        }
    }

    if (randValue < postrollProba) {
        slot->Value["post_news_mode"] = POSTROLL_NEWS_CHANGE_SOURCE_MODE;
    }
}

void TRequestType::AddAnalyticsNewsType(const TString& errorType) {
    TString type;
    if (!errorType.empty()) {
        // News ended is nonews error too, but will be processed with other text in NLG. Also need a different
        // analytics type to better analyze responses.
        type = Ctx.HasAttention(NEWS_ENDED_ATTENTION) ? NEWS_ENDED_TYPE : errorType;
    } else {
        type = GetNewsType();
    }
    Y_ENSURE(!type.empty(), "News type must be initialized");
    Ctx.GetAnalyticsInfoBuilder().AddSelectedSourceEvent(Ctx.GetRequestStartTime(), type);
}

void TRequestType::CreateSlot(NSc::TValue json) {
    Ctx.CreateSlot(SLOT_NEWS, "news", true, std::move(json));
}

void TRequestType::AddContinuousAttention() const {
    if (Ctx.HasExpFlag(ALICE_NEWS_NO_CONTINOUS_ATTENTION_EXP_FLAG)) {
        return;
    }

    bool addAttention = true;

    const auto* newsSlot = Ctx.GetSlot(SLOT_NEWS);
    // If previous "news" slot has field "last_epoch", compare it with current "epoch".
    if (Ctx.Meta().HasEpoch() && !IsSlotEmpty(newsSlot) && newsSlot->Value.Has(LAST_EPOCH_FIELD)) {
        const i64 lastEpoch = newsSlot->Value[LAST_EPOCH_FIELD].GetIntNumber();
        const i64 currentEpoch = Ctx.Meta().Epoch();
        // Epoch is client_time, it may be invalid, have changed NTP, etc. If current timespent is lower then previous,
        // substraction result is below zero (attention is added by default).
        // If diff is more than X (seconds) to skip attention.
        if (currentEpoch - lastEpoch > SECONDS_TO_SKIP_INTRO) {
            addAttention = false;
        }
    }
    if (addAttention) {
        // Means skip intro and promo in VINS nlg.
        Ctx.AddAttention(NEWS_CONTINUOUS_ATTENTION);
    }
}

void TRequestType::AddSuggests() const {
    Ctx.AddSuggest("get_news__settings");
    Ctx.AddSuggest("get_news__more");
    Ctx.AddSuggest("get_news__details");
    AddAdditionalSuggests(Ctx);
    NewsData.MakeSuggests(Ctx, Issue, Rubric);
}


// -------------------------------------------------------------- TSearchRequest

TSearchRequest::TSearchRequest(TContext& ctx, TStringBuf text)
    : TRequestType(ctx)
    , Text(text)
    , Offset(GetWizardNewsOffset())
    , NewsCount(Offset + MaxNewsCount)
{
}

const NSc::TValue& FindDocsInSearchResponse(const NSc::TValue& news) {
    if (const auto& docs = news["story"]["docs"]; !docs.ArrayEmpty()) {
        return docs;
    }
    for (const auto& tab : news["story"]["tabs"].GetArray()) {
        if (tab.Get("is_active").GetBool()) {
            return tab["docs"];
        }
    }
    return NSc::Null();
}

size_t TSearchRequest::GetWizardNewsOffset() const {
    const TContext::TSlot* slotPrevNews = Ctx.GetSlot(SLOT_NEWS);
    if (IsSlotEmpty(slotPrevNews) || !IsContinuousRequest()) {
        return 0;
    }
    return slotPrevNews->Value.TrySelect("search_wizard_offset").GetIntNumber();
}

TResultValue TSearchRequest::MakeWebRequest(NSc::TValue & result) const {
    // these cgi params to prevent news titles cutting
    TCgiParameters cgi{
        {"rearr", "scheme_Local/NewsFromQuickMiddle/MakeFirstTitleOptions/PixelCut=0"},
        {"rearr", "scheme_Local/NewsFromQuickMiddle/MakeFirstTitleOptions/MaxLen=1000"},
        {"rearr", "scheme_Local/NewsFromQuickMiddle/MakeTitleOptions/PixelCut=0"},
        {"rearr", "scheme_Local/NewsFromQuickMiddle/MakeTitleOptions/MaxLen=1000"},
        // ALICE-6490: To have valid 'touch_url' add these two rearr params.
        {"rearr", "scheme_Local/NewsFromQuickMiddle/ShowSingleStory=0"},
        {"rearr", "scheme_Local/NewsFromQuickMiddle/NoneAsSingleOpts/Enabled=0"},
        {"snip", "max-title-length=10000"},
    };
    if (Ctx.HasExpFlag(PA_NEWS_REMOVE_FILTER_EXP_FLAG)) {
        // sets the same filtering as in desktop (touch and desktop news wizards have different behaviour)
        cgi.InsertUnescaped("rearr", "scheme_Local/NewsFromQuickMiddle/MobileNewsHostsList=news_hosts");
    }
    // DIALOG-6128: Enable more news for wizard requests. Check intent. "get_news__more" means repeated request, slots
    // are imported from previous = add offset. "get_news" means it is new query with slot clearing. Cannot compare
    // current and previous slots - not sure it's the same request about same topic = do not increase count.
    if (IsContinuousRequest() && Offset > 0) {
        // Increase request count and add attention to skip intro.
        AddContinuousAttention();
    }
    TString cgiCount = IntToString<10, size_t>(NewsCount);
    cgi.InsertUnescaped("rearr", TString::Join("scheme_Local/NewsFromQuickMiddle/WizardDocumentsCount=", cgiCount));
    cgi.InsertUnescaped("rearr", TString::Join("scheme_Local/NewsFromQuickUpper/WizardDocumentsCount=", cgiCount));
    if (Ctx.HasExpFlag(ALICE_NEWS_WIZARD_NO_FLTR_EXPR_EXP_FLAG)) {
        // EXPERIMENTS-41722: Exp have decreased docs count, flag disables behaviour.
        cgi.InsertUnescaped("rearr", "scheme_Local/NewsFromQuickMiddle/FilterDocByExpr/Enabled=0");
    }

    if (TResultValue ret =
            NSerp::MakeRequest(Text, Ctx, cgi, &result, NAlice::TWebSearchBuilder::EService::BassNews)) {
        const TString message = TString::Join("SERP request failed with error: ", ret->Msg);
        return TError(TError::EType::NONEWS, message);
    }
    return TResultValue();
}

TResultValue TSearchRequest::Process() {
    if (!Ctx.HasExpFlag(ALICE_NEWS_ENABLE_WIZARD)) {
        return TError(TError::EType::NONEWS, "wizard response is disabled");
    }

    NSc::TValue serpResult;

    TString searchDocsKey = "searchdata";
    if (Ctx.Meta().ForbidWebSearch()) {
        const auto& dataSources = Ctx.DataSources();
        if (dataSources.empty() || !dataSources.contains(WEB_SEARCH_DOCS_TYPE)) {
            // We are restricted to make web request, return irrelevant answer nonews.
            return TError(TError::EType::SYSTEM, "Expect SearchWebDocs in request");
        }
        serpResult = dataSources.at(WEB_SEARCH_DOCS_TYPE);
        searchDocsKey = "web_search_docs";
    } else if (auto err = MakeWebRequest(serpResult)) {
        return err;
    }

    auto newsSnippetFilter = [](const NSc::TValue& src) { return src["type"].GetString() == "news"; };
    NSerp::TSnippetIterator it(serpResult, newsSnippetFilter, searchDocsKey);
    if (!it) {
        return TError(TError::EType::NONEWS, ERROR_NO_SERP_NEWS);
    }

    NSc::TValue jsonNewsList;
    const NSc::TArray& storiesArray = FindDocsInSearchResponse(*it).GetArray();
    for (size_t index = Offset; index < storiesArray.size(); ++index) {
        const NSc::TValue& story = storiesArray[index];
        NSc::TValue& jsonStory = jsonNewsList.Push();
        jsonStory["url"].SetString(GenerateNewsUri(Ctx.MetaClientInfo(), story["url"].GetString()));
        jsonStory["text"] = NBASS::NSerpSnippets::RemoveHiLight(story["title"].GetString());
        jsonStory["snippet"] = NBASS::NSerpSnippets::RemoveHiLight(story["snippet"].GetString());
        jsonStory["agency"] = NBASS::NSerpSnippets::RemoveHiLight(story["agency"].GetString());

        auto pubDate = TryParseTime(story["time"].GetString());
        if (pubDate) {
            jsonStory["date"] = pubDate.Get()->Seconds();
        }

        if (story["use_agency_logo_as_picture"].ForceIntNumber(0)) {
            if (const NSc::TValue& src = story.TrySelect("/desktop_picture/src"); !src.IsNull()) {
                TString logoUrl = CreateLogoUrl(src.GetString());
                if (!logoUrl.empty()) {
                    jsonStory["logo"] = logoUrl;
                }
            }
            SetNoIcon(&jsonStory);
        } else {
            if (const NSc::TValue& src = story.TrySelect("/desktop_logo/src"); !src.IsNull()) {
                TString logoUrl = CreateLogoUrl(src.GetString());
                if (!logoUrl.empty()) {
                  jsonStory["logo"] = logoUrl;
                }
            }
            TScaleSize::CreateImage(Ctx.MatchScreenScaleFactor(ALLOWED_SCREEN_SCALE_FACTORS_DEFAULT),
                                    story["touch_retina_picture"]["src"].GetString(), &jsonStory);
        }

        // it means turbo
        if (story["can_show_in_sideblock"].ForceNumber(0)) {
            AddTurboIcon(&jsonStory);
        }

        if (jsonNewsList.ArraySize() >= MaxNewsCount) {
            break;
        }
    }
    if (jsonNewsList.IsNull()) {
        // Offset means continuous request.
        if (Offset > 0 && IsContinuousRequest()) {
            Ctx.AddAttention(NEWS_ENDED_ATTENTION);
            return TError(TError::EType::NONEWS, ERROR_NEWS_ENDED_MSG);
        } else {
            return TError(TError::EType::NONEWS, ERROR_NO_SERP_NEWS);
        }
    }

    NSc::TValue json;
    json["news"] = std::move(jsonNewsList);

    const TStringBuf url = it->Get("touch_url").GetString();
    json["url"].SetString(GenerateNewsUri(Ctx.MetaClientInfo(), url));

    // Save requested count to get new stories next time.
    json["search_wizard_offset"] = NewsCount;
    SaveRequestTime(json);

    CreateSlot(std::move(json));
    AddSuggests();
    Ctx.AddAttention("wizard_response");

    return TResultValue();
}


// ------------------------------------------------------------- TNewsApiRequest

TNewsApiRequest::TNewsApiRequest(TContext& ctx)
    : TNewsApiRequest(ctx, "/rubric", API_PRESET_RUBRIC)
{
}

TNewsApiRequest::TNewsApiRequest(TContext& ctx, const TString& apiPreset)
    : TNewsApiRequest(ctx, "/rubric", apiPreset)
{
}

TNewsApiRequest::TNewsApiRequest(TContext& ctx, const TString& method, const TString& apiPreset)
    : TRequestType(ctx)
    , ApiPreset(apiPreset)
    , Method(method)
{
}

TVector<TString> TNewsApiRequest::GetExcludeIdsFromState() const {
    TVector<TString> result;

    const TContext::TSlot* slotPrevNews = Ctx.GetSlot(SLOT_NEWS);
    if (IsSlotEmpty(slotPrevNews)) {
        return result;
    }
    if (slotPrevNews->Value.IsString()) {
        NSc::TValue newsSlotValue;
        NSc::TValue::FromJson(newsSlotValue, slotPrevNews->Value.GetString());
        for (auto id : newsSlotValue["exclude_ids"].GetArray()) {
            result.push_back(TString(id.GetString()));
        }
        return result;
    }

    const NSc::TArray& excludeIds = slotPrevNews->Value["exclude_ids"].GetArray();

    for (auto id : excludeIds) {
        result.push_back(TString(id.GetString()));
    }
    return result;
}

void TNewsApiRequest::AddExcludeNewsIds(NSc::TValue& news) const {
    NSc::TValue& excludeIds = news["exclude_ids"].SetArray();

    // Append ids from previous requests.
    TVector<TString> prevExcludeIds = GetExcludeIdsFromState();
    if (!prevExcludeIds.empty() && prevExcludeIds.size() <= MAX_EXCLUDE_IDS_SIZE) {
        for (auto id : prevExcludeIds) {
            excludeIds.Push(id);
        }
    }

    // Save ids to exclude this document in "more" request.
    for (const NSc::TValue& item : news["news"].GetArray()) {
        excludeIds.Push(item["id"].GetString());
    }
}

TResultValue TNewsApiRequest::AddAuxData(const NSc::TValue& newsResponse, NSc::TValue& newsResult) const {
    const TStringBuf rubric = Rubric.IsIndex() ? Rubric.Alias() : newsResponse.TrySelect("rubric/alias").GetString();
    const TString urlForRubric = NewsData.UrlFor(Issue, rubric);
    if (!urlForRubric) {
        return TError(TError::EType::SYSTEM, TString::Join("Unable to find a url for rubric: ", rubric));
    }
    newsResult["url"] = GenerateNewsUri(Ctx.MetaClientInfo(), urlForRubric);

    // DIALOG-6766: Add for nlg. Text depends on rubric.
    if (Rubric.IsIndex()) {
        Ctx.AddAttention(TOP_NEWS_ATTENTION);
    }

    return TResultValue();
}

TString TNewsApiRequest::BuildExcludeIdsCgi() const {
    TVector<TString> excludeIds = GetExcludeIdsFromState();
    return JoinSeq(",", excludeIds);
}

TResultValue TNewsApiRequest::PrepareRequest(TCgiParameters& cgi) const {
    // Check news API know about |Rubric| in detected |Issue|.
    if (!Rubric.IsInIssue(Issue) && !Rubric.IsInIssue(NewsData.FallbackIssue())) {
        return TError(TError::EType::NONEWS, "Bad rubric or issue");
    }

    cgi.InsertUnescaped("rubric", Rubric.Alias());
    return TResultValue();
}

TResultValue TNewsApiRequest::ParseNewsJson(const NSc::TValue& newsResponse, NSc::TValue& result) const {
    const NSc::TValue& items = newsResponse.TrySelect("items");
    if (items.ArrayEmpty()) {
        if (IsContinuousRequest()) {
            Ctx.AddAttention(NEWS_ENDED_ATTENTION);
            return TError(TError::EType::NONEWS, ERROR_NEWS_ENDED_MSG);
        } else {
            return TError(TError::EType::NONEWS, "API return no news");
        }
    }

    NSc::TValue& newsList = result["news"].SetArray();
    for (const NSc::TValue& src : items.GetArray()) {
        NSc::TValue dst;

        const NSc::TValue& obj = src["obj"];
        dst["id"] = obj["persistent_id"];
        dst["text"] = obj["title"];
        dst["url"].SetString(GenerateNewsUri(Ctx.MetaClientInfo(), obj["url"].GetString()));

        const NSc::TValue& annotation = obj.TrySelect("annot_docs[0]");
        if (!annotation.DictEmpty()) {
            dst["date"] = annotation["date"];
            dst["agency"] = annotation["source_name"];
            if (const NSc::TValue& logo = annotation.TrySelect("/logo_square"); !logo.IsNull()) {
                TString logoUrl = CreateLogoUrl(logo.GetString());
                if (!logoUrl.empty()) {
                    dst["logo"] = logoUrl;
                }
            }
            dst["snippet"] = annotation["snippet"];
        }

        const NSc::TValue& image = obj.TrySelect("pictures/items[0]");
        if (!image.IsNull()) {
            TScaleSize::CreateImage(Ctx.MatchScreenScaleFactor(ALLOWED_SCREEN_SCALE_FACTORS_DEFAULT),
                                    image, &dst);
        } else {
            SetNoIcon(&dst);
        }

        const NSc::TValue& summary = obj.TrySelect("summary");
        if (!summary.DictEmpty()) {
            const NSc::TValue& points = summary.TrySelect("points");
            NSc::TValue& pointsList = dst["extended_news"].SetArray();
            for (const NSc::TValue& point : points.GetArray()) { 
                NSc::TValue res;
                res["text"] = point["text"];
                res["url"].SetString(GenerateNewsUri(Ctx.MetaClientInfo(), point["url"].GetString()));
                res["agency"] = point["agency"]["actual_name"];
                pointsList.Push(std::move(res));
            }
        }
        
        newsList.Push(std::move(dst));
        if (newsList.ArraySize() >= MaxNewsCount) {
            break;
        }
    }
    return TResultValue();
}

TStringBuf TNewsApiRequest::GetNewsType() const {
    if (Rubric.IsIndex()) {
        return NEWS_DEFAULT_TYPE;
    }
    return Rubric.Alias() == KUREZY_RUBRIC ? NEWS_KUREZY_TYPE : NEWS_RUBRIC_TYPE;
}

TResultValue TNewsApiRequest::Process() {
    if (GetNewsType() != NEWS_SMI_TYPE) {
        Ctx.AddAttention("rubric_response");
        return TError(TError::EType::NONEWS, "rubric response is disabled");
    }

    THPTimer watcher;
    TCgiParameters cgi{
        {"count", IntToString<10, size_t>(MaxNewsCount)},
        {"preset", ApiPreset},
        {"add_summary", "1"}
    };
    if (Issue.Id != NGeoDB::RUSSIA_ID) {
        // Ask different issue, but only in russian language.
        cgi.InsertUnescaped("issue", Issue.Domain);
        cgi.InsertUnescaped("doclang", "ru");
    }
    if (Ctx.ClientFeatures().SupportsDivCardsRendering()) {
        cgi.InsertUnescaped("picture_sizes", "100.100.1.orig");
    }
    // Format cgi string for request.
    if (auto err = PrepareRequest(cgi)) {
        return err;
    }

    TString param = BuildExcludeIdsCgi();
    if (!param.empty()) {
        cgi.InsertUnescaped("exclude", param);
        // Non-empty param means continuous request.
        AddContinuousAttention();
    }
    LOG(INFO) << "DEFAULT. Before request inited: " << watcher.PassedReset() << Endl;

    auto request = Ctx.GetSources().NewsApi(Method).Request();
    request->AddCgiParams(cgi);
    const auto response = request->Fetch()->WaitFor(Ctx.GetConfig()->Vins().NewsApi().Timeout());
    if (!response || response->IsError()) {
        return TError(TError::EType::SYSTEM,
                      TStringBuilder() << "Fetching from news api error: "
                                       << (response ? response->GetErrorText() : "no response"));
    }
    LOG(INFO) << "DEFAULT. After response: " << watcher.PassedReset() << Endl;

    NSc::TValue newsResponse;
    if (!NSc::TValue::FromJson(newsResponse, response->Data) || !newsResponse.IsDict()) {
        return TError(TError::EType::SYSTEM, TStringBuilder() << "Cannot parse news result: " << response->Data);
    }
    // Check Request-specific conditions.
    if (auto err = OnFetched(newsResponse)) {
        return err;
    }
    // Parse response json and create "news" slot structure.
    NSc::TValue news;
    if (auto err = ParseNewsJson(newsResponse, news)) {
        return err;
    }
    // Add Request-specific data to answer.
    if (auto err = AddAuxData(newsResponse, news)) {
        return err;
    }
    LOG(INFO) << "DEFAULT. Before postprocessing: " << watcher.PassedReset() << Endl;

    // Remember news ids to not repeat.
    AddExcludeNewsIds(news);
    SaveRequestTime(news);

    CreateSlot(std::move(news));
    AddSuggests();
    LOG(INFO) << "DEFAULT. End: " << watcher.PassedReset() << Endl;
    return TResultValue();
}

// -------------------------------------------------------------- TGeoApiRequest

TGeoApiRequest::TGeoApiRequest(TContext& ctx, TRequestedGeo geo)
    : TNewsApiRequest(ctx, API_PRESET_GEO)
    , Geo(geo) {
    const auto& geobase = ctx.GlobalCtx().GeobaseLookup();
    if (geobase.IsIdInRegion(Geo.GetId(), NGeoDB::RUSSIA_ID)) {
        // Up to city, because API dont know about lower types.
        Geo.ConvertTo(NGeobase::ERegionType::CITY);
    } else {
        // API dont return news for foreign cities, but for countries.
        // Request Czech Republic for Prague and Greece for Athens. DIALOG-6027.
        Geo.ConvertTo(NGeobase::ERegionType::COUNTRY);
    }
}

TResultValue TGeoApiRequest::AddAuxData(const NSc::TValue&, NSc::TValue& newsResult) const {
    Y_ENSURE(NAlice::IsValidId(Geo.GetId()), "GeoId must be valid, checked in OnFetched()");

    TString url = NewsData.UrlFor(Issue, Geo.GetId());
    if (!url) {
        const TString msg = TString::Join("Unable to find an url for geo: ", ToString(Geo.GetId()));
        return TError(TError::EType::SYSTEM, msg);
    }
    newsResult["url"].SetString(GenerateNewsUri(Ctx.MetaClientInfo(), url));
    return Geo.CreateResolvedMeta(Ctx, SLOT_RESOLVED_WHERE);
}

TResultValue TGeoApiRequest::PrepareRequest(TCgiParameters& cgi) const {
    if (TResultValue error = Geo.GetError()) {
        return error;
    }
    cgi.InsertUnescaped("region_id", ToString(Geo.GetId()));
    cgi.InsertUnescaped("country", Issue.Domain);
    // Without fallback API return Moscow for unknown countries (Greece, Portugal, etc).
    cgi.InsertUnescaped("do_not_fallback_region", "1");
    return TResultValue();
}

TResultValue TGeoApiRequest::OnFetched(const NSc::TValue& newsResponse) {
    // Select first region id to specify geo position.
    const NSc::TValue& newsGeo = newsResponse.TrySelect("items/0/obj/regions/0");
    // News for foreign geo objects return empty regions.
    if (newsGeo.IsNull()) {
        return TResultValue();
    }

    NGeobase::TId geoId = newsGeo.ForceIntNumber(NGeobase::UNKNOWN_REGION);
    if (!NAlice::IsValidId(geoId)) {
        return TError(TError::EType::INVALIDPARAM, "Invalid json response from news API");
    }
    Geo.ConvertTo(geoId);

    return TResultValue();
}

// ------------------------------------------------------------ TPersonalRequest

TPersonalRequest::TPersonalRequest(TContext& ctx, const TString& yandexUID)
    : TNewsApiRequest(ctx, API_PRESET_PERSONAL)
    , YandexUID(yandexUID)
{
}

TResultValue TPersonalRequest::AddAuxData(const NSc::TValue&, NSc::TValue& newsResult) const {
    newsResult["url"].SetString(PERSONAL_FEED_URL);

    // DIALOG-6455: Mark request as personal for NLG processing.
    Ctx.AddAttention(PERSONAL_NEWS_ATTENTION);

    return TResultValue();
}

TResultValue TPersonalRequest::PrepareRequest(TCgiParameters& cgi) const {
    cgi.InsertUnescaped("rubric", "personal_feed");
    cgi.InsertUnescaped("personalize", "personal_feed");
    cgi.InsertUnescaped("userid", YandexUID);
    return TResultValue();
}

// ----------------------------------------------------------------- TSmiRequest

TSmiRequest::TSmiRequest(TContext& ctx, const TSmi smi)
    : TNewsApiRequest(ctx, "/smi_documents", API_PRESET_SMI)
    , Smi(smi)
{
}

TResultValue TSmiRequest::Process() {
    if (auto err = TNewsApiRequest::Process()) {
        return err;
    }
    TContext::TSlot* newsSlot = Ctx.GetSlot(SLOT_NEWS);
    Y_ENSURE(!IsSlotEmpty(newsSlot), "Must be called with filled news");
    // Forward name to nlg for corrent introduction.
    newsSlot->Value["smi"].SetString(Smi.GetName());
    return TResultValue();
}

TResultValue TSmiRequest::AddAuxData(const NSc::TValue&, NSc::TValue& newsResult) const {
    newsResult["url"].SetString(Smi.GetUrl());
    return TResultValue();
}

TResultValue TSmiRequest::PrepareRequest(TCgiParameters& cgi) const {
    cgi.InsertUnescaped("aid", Smi.GetAid());
    cgi.InsertUnescaped("without_articles", "1");
    return TResultValue();
}

TResultValue TSmiRequest::ParseNewsJson(const NSc::TValue& newsResponse, NSc::TValue& result) const {
    const NSc::TValue& items = newsResponse.TrySelect("data/documents");
    if (items.ArrayEmpty()) {
        if (IsContinuousRequest()) {
            Ctx.AddAttention(NEWS_ENDED_ATTENTION);
            return TError(TError::EType::NONEWS, ERROR_NEWS_ENDED_MSG);
        } else {
            return TError(TError::EType::NONEWS, "API smi return no news");
        }
    }

    NSc::TValue& newsList = result["news"].SetArray();
    for (const NSc::TValue& src : items.GetArray()) {
        NSc::TValue dst;

        // By default int is signed.
        const ui64 uid = src["uid"];
        dst["id"] = ToString(uid);
        dst["text"] = src["title"];
        dst["url"] = src["url"];
        dst["snippet"] = src["text"];
        dst["agency"] = Smi.GetName();
        dst["date"] = src["pub_date"];
        dst["logo"] = Smi.GetLogo();

        newsList.Push(std::move(dst));
        if (newsList.ArraySize() >= MaxNewsCount) {
            break;
        }
    }
    return TResultValue();
}

// ---------------------------------------------------------- TNewsHandlerHelper

TNewsHandlerHelper::TNewsHandlerHelper(TContext& ctx)
    : Ctx(ctx)
    , SlotSmi(ctx.GetSlot(SLOT_SMI))
    , SlotTopic(ctx.GetSlot(SLOT_TOPIC))
    , SlotWhere(ctx.GetSlot(SLOT_WHERE))
    , NewsData(ctx.GlobalCtx().NewsData())
    , Issue(ConstructIssue(ctx))
    , Rubric(ConstructRubric(ctx, SlotTopic))
    , MementoValue(ConstructMementoValue(ctx))
    , ForeignRequest(IsForeignRequest(ctx))
    , IsSpeaker(ctx.MetaClientInfo().IsSmartSpeaker())
{
    // Personal news need yandexuid.
    if (Ctx.Meta().HasYandexUID()) {
        YandexUID = Ctx.Meta().YandexUID();
    } else {
        // Otherwise, send request to blackbox or reuse data source.
        TPersonalDataHelper personalDataHelper(Ctx);
        if (!personalDataHelper.GetUid(YandexUID)) {
            LOG(ERR) << "TPersonalDataHelper::GetUid failed." << Endl;
        }
    }

    if (IsSlotEmpty(SlotTopic)) {
        return;
    }
    // Extract query string from SlotTopic.
    if (IsRubricSlot(SlotTopic)) {
        const auto& value = SlotTopic->Value.GetString();
        if (const auto* rubric = NewsData.RubricByAlias(value)) {
            // XXX could be empty if rubric doesn't have such language
            TopicValue = rubric->ShortName(Ctx.MetaLocale().Lang);
        } else if (const auto* smi = NewsData.SmiByAlias(value)) {
            TopicValue = smi->GetName();
        }
    }

    // SourceText is deprecated, hollywood news service doesn't return it
    // By default use SourceText as original query value.
    if (TopicValue.empty() && !SlotTopic->SourceText.IsNull()) {
        TopicValue = SlotTopic->SourceText.GetString();
    }

    // By default in HW use Value as original query value.
    if (TopicValue.empty()) {
        TopicValue = SlotTopic->Value.GetString();
    }
}

THolder<TSearchRequest> TNewsHandlerHelper::MakeSearchRequest() const {
    TStringBuilder query;
    query << NEWS_STRING_RUS;
    if (!IsSlotEmpty(SlotTopic)) {
        query << ' ' << GetTopicValue();
    }
    if (!IsSlotEmpty(SlotWhere)) {
        query << ' ' << SlotWhere->Value.GetString();
    }
    return MakeHolder<TSearchRequest>(Ctx, query);
}

THolder<TSmiRequest> TNewsHandlerHelper::MakeSmiRequest() const {
    Y_ENSURE(!Ctx.HasExpFlag(ALICE_SMI_NEWS_DISABLED_EXP_FLAG), "Reversed exp is processed with wizard");

    // If request is from HW and slots are known
    if (!IsSlotEmpty(SlotSmi)) {
        NSc::TValue smiProto;
        NSc::TValue::FromJson(smiProto, SlotSmi->Value.GetString());
        return MakeHolder<TSmiRequest>(Ctx, TSmi(smiProto));
    }

    Y_ENSURE(!IsSlotEmpty(SlotTopic), "Valid slot with smi alias is expected");
    const auto* smi = NewsData.SmiByAlias(SlotTopic->Value.GetString());
    Y_ENSURE(smi, "Expect valid TSmi pointer");
    return MakeHolder<TSmiRequest>(Ctx, *smi);
}

TSmi TNewsHandlerHelper::ChooseSmi(const TVector<TSmi>& smis) const {
    if (!MementoValue.IsNull()) {
        for (const auto& smi : smis) {
            if (smi.GetAlias() == MementoValue[MEMENTO_SOURCE]) {
                return smi;
            }
        }
    }
    return smis[Ctx.GetRng().RandomInteger(smis.size())];
}

THolder<TSmiRequest> TNewsHandlerHelper::MakeSmiRubricRequest() const {
    if (Ctx.HasExpFlag(ALICE_SMI_NEWS_DISABLED_EXP_FLAG)) {
        LOG(ERR) << "Reversed exp is processed with wizard";
        return nullptr;
    }

    const auto rubric = IsSlotEmpty(SlotTopic) ? DEFAULT_INDEX_RUBRIC : SlotTopic->Value.GetString();
    const auto smis = NewsData.SmisByRubric(rubric);
    if (smis == nullptr || smis->empty()) {
        LOG(ERR) << "Expect non-empty rubric-to-smi mapping";
        return nullptr;
    }
    const auto smi = ChooseSmi(*smis);
    return MakeHolder<TSmiRequest>(Ctx, smi);
}

bool TNewsHandlerHelper::HasGeoInfoInAPI(const TRequestedGeo& geo) const {
    if (Ctx.HasExpFlag(ALICE_FOREIGN_NEWS_DISABLED_EXP_FLAG)) {
        return false;
    }
    const auto& geobase = Ctx.GlobalCtx().GeobaseLookup();
    // Believe that API know everything about Russia.
    if (geobase.IsIdInRegion(geo.GetId(), NGeoDB::RUSSIA_ID)) {
        return true;
    }
    // For foreign geo check existance in news API.
    const auto countryId = geo.GetParentIdByType(NGeobase::ERegionType::COUNTRY);
    // Try get url for geo. If nothing returns it's useless to ask API.
    const TString url = NewsData.UrlFor(Issue, countryId, true /* onlyForIssue */);
    return !url.empty();
}

bool TNewsHandlerHelper::HasRubricInfoInAPI() const {
    if (Ctx.HasExpFlag(ALICE_FOREIGN_NEWS_DISABLED_EXP_FLAG)) {
        return false;
    }
    // Construct rubric and transfer request to wizard if API can't answer |Rubric| for user's |Issue|.
    return Rubric.IsInIssue(Issue);
}

bool TNewsHandlerHelper::IsSmiRequest() const {
    return !IsSlotEmpty(SlotSmi) ||
        (!IsSlotEmpty(SlotTopic) && NewsData.SmiByAlias(SlotTopic->Value.GetString()) != nullptr);
}

bool TNewsHandlerHelper::IsSmiRubricRequest() const {
    return IsSlotEmpty(SlotTopic) || NewsData.SmisByRubric(SlotTopic->Value.GetString()) != nullptr;
}

THolder<TRequestType> TNewsHandlerHelper::ProcessGeoRequest(TRequestedGeo geo) const {
    Y_UNUSED(geo);
    // Geo API is not available anymore
    return MakeSearchRequest();
}

int TNewsHandlerHelper::GetPersonalActionsThreshold() const {
    auto thresholdValue = Ctx.GetValueFromExpPrefix(PERSONAL_ACTIONS_THRESHOLD_EXP_PREFIX);
    int threshold = 0;
    // Validate threshold. Actions is number in range [0;100].
    if (!thresholdValue || thresholdValue->empty() || !TryFromString<int>(*thresholdValue, threshold) ||
        threshold < 0 || threshold > 100) {
        return DEFAULT_PERSONAL_ACTIONS_THRESHOLD;
    }
    return threshold;
}

bool TNewsHandlerHelper::IsPersonalNewsAllowed() const {
    // DIALOG-6529: If tagger get 'personal' topic, return user's personal_feed.
    if (IsRubricSlot(SlotTopic, PERSONAL_RUBRIC)) {
        return true;
    }
    // Otherwise return personal for empty topic and default exp enabled.
    return Ctx.HasExpFlag(ALICE_DEFAULT_PERSONAL_EXP_FLAG) && IsSlotEmpty(SlotTopic);
}

bool TNewsHandlerHelper::IsUserWarm() const {
    if (YandexUID.empty()) {
        LOG(DEBUG) << "YandexUID is empty, cannot check for personal news" << Endl;
        return false;
    }

    TCgiParameters cgi{{"userid", YandexUID}};

    auto request = Ctx.GetSources().NewsApi("/user_coldness").Request();
    request->AddCgiParams(cgi);
    const auto response = request->Fetch()->WaitFor(Ctx.GetConfig()->Vins().NewsApi().Timeout());
    if (!response || response->IsError()) {
        LOG(ERR) << (response ? response->GetErrorText() : "no response");
        return false;
    }

    NSc::TValue coldness;
    if (!NSc::TValue::FromJson(coldness, response->Data) || !coldness.IsDict()) {
        LOG(ERR) << "Cannot parse user_coldness json: " << response->Data;
        return false;
    }

    if (coldness["cold"].GetIntNumber() != 0) {
        return false;
    }
    const int actionsThreshold = GetPersonalActionsThreshold();
    return coldness["number_of_actions"].GetIntNumber() >= actionsThreshold;
}

THolder<TRequestType> TNewsHandlerHelper::MakeDefaultRequest() const {
    // For the case when API turns off coronavirus rubric in newsdata.
    if (ShouldAskWizardForCovidNews()) {
        // People don't pronounce it clearly, replace query with right one.
        return MakeHolder<TSearchRequest>(Ctx, "Новости коронавирус");
    }
    /* Cases when response with wizard info:
     * 1. topic has 'string' type - tagger didn't find rubric or smi.
     * 2. special rubrics (kurezy, smi): tagged as 'news_topic' but exps are disabled.
     */
    if (IsCustomTopicSlot(SlotTopic) || ShouldAskWizardForFunnyNews() || ShouldAskWizardForSmiNews()) {
        return MakeSearchRequest();
    }
    // DIALOG-6606: Request /smi_documents. Must be called after smi_wizard check.
    // DIALOG-6968: API Smi must be checked before foreign, because SMI is international.
    if (IsSmiRequest()) {
        return MakeSmiRequest();
    }
    // Requests smi instead of rubrics according to a mapping
    if (Ctx.HasExpFlag(NAlice::NExperiments::EXP_NEWS_DISABLE_RUBRIC_API) && IsSmiRubricRequest()) {
        if (auto request = MakeSmiRubricRequest()) {
            return request;
        }
    }
    // Transfer to wizard unknown issues (America for example)
    // and known issue (not Russia) but API hasn't info for requested rubric.
    if (IsUnknownUserRegion() || (ForeignRequest && !HasRubricInfoInAPI())) {
        return MakeSearchRequest();
    }
    // If exp is enabled and |slotTopic| is empty, then check coldness for personality.
    // If user warm, create TPersonalRequest instance. For cold - continue to default API request.
    if (IsPersonalNewsAllowed() && IsUserWarm()) {
        return MakeHolder<TPersonalRequest>(Ctx, YandexUID);
    }
    // Now we sure can answer with requested rubric for user country.
    return MakeHolder<TNewsApiRequest>(Ctx);
}

void TNewsHandlerHelper::CheckSpeakerSpeedDirective() const {
    TContext::TSlot* newsSlot = Ctx.GetSlot(SLOT_NEWS);
    Y_ENSURE(!IsSlotEmpty(newsSlot), "Must be called with filled news");

    float speed = 0.0;
    auto speedFlagValue = Ctx.GetValueFromExpPrefix(SPEAKER_SPEED_EXP_PREFIX);
    if (speedFlagValue && !speedFlagValue->empty() && TryFromString<float>(*speedFlagValue, speed)) {
        newsSlot->Value["speaker_speed"].SetNumber(speed);
    }
}

void TNewsHandlerHelper::SkipRepeatedNews() {
    TContext::TSlot* newsSlot = Ctx.GetSlot(SLOT_NEWS);
    if (IsSlotEmpty(newsSlot) || !newsSlot->Value.PathExists("news")) {
        return;
    }
    NSc::TArray& newsItems = newsSlot->Value["news"].GetArrayMutable();
    if (newsItems.empty()) {
        return;
    }
    TTextComparer comparer;
    THashSet<size_t> toSkip;
    toSkip.reserve(newsItems.size());
    for (size_t i = 0; i < newsItems.size(); i++) {
        if (toSkip.contains(i)) {
            continue;
        }
        for (size_t j = i + 1; j < newsItems.size(); j++) {
            if (comparer.GetSimilarity(newsItems[i]["snippet"].GetString(), newsItems[j]["snippet"].GetString()) >= SKIP_DUPS_THRESHOLD) {
                LOG(DEBUG) << "Skipping repeated news: " << newsItems[i]["text"].GetString() << "  -------  " << newsItems[j]["text"].GetString() << Endl;
                toSkip.insert(j);
            }
        }
    }

    NSc::TValue withoutDups;
    NSc::TArray& withoutDupsArray = withoutDups.GetArrayMutable();
    for (size_t i = 0; i < newsItems.size(); i++) {
        if (!toSkip.contains(i)) {
            withoutDupsArray.push_back(newsItems[i]);
        }
    }
    newsSlot->Value["news"] = withoutDups;
}

void TNewsHandlerHelper::SkipRepeatableNewsTitles(float threshold) {
    TContext::TSlot* newsSlot = Ctx.GetSlot(SLOT_NEWS);
    if (IsSlotEmpty(newsSlot) || !newsSlot->Value.PathExists("news")) {
        return;
    }
    NSc::TArray& newsItems = newsSlot->Value["news"].GetArrayMutable();
    if (newsItems.empty()) {
        return;
    }

    TTextComparer comparer;
    for (NSc::TValue& story : newsItems) {
        const TStringBuf title = story["text"].GetString();
        const TStringBuf snippet = story["snippet"].GetString();
        if (title.empty() || snippet.empty()) {
            continue;
        }
        const float similarity = comparer.GetSimilarity(title, comparer.GetFirstSentence(snippet));
        if (similarity >= threshold) {
            story["skip_title"].SetBool(true);
        }
    }
}

bool TNewsHandlerHelper::IsUnknownUserRegion() const {
    return !NewsData.HasIssueForGeo(Ctx.GlobalCtx().GeobaseLookup(), Ctx.UserRegion());
}

bool TNewsHandlerHelper::ShouldAskWizardForCovidNews() const {
    // DIALOG-6298: If 'koronavirus' missed in |NewsData|, it's disabled. Ask wizard.
    return IsRubricSlot(SlotTopic, COVID_RUBRIC) && !NewsData.RubricByAlias(COVID_RUBRIC);
}

bool TNewsHandlerHelper::ShouldAskWizardForFunnyNews() const {
    return IsRubricSlot(SlotTopic, KUREZY_RUBRIC) && !Ctx.HasExpFlag(ALICE_FUNNY_NEWS_EXP_FLAG);
}

bool TNewsHandlerHelper::ShouldAskWizardForSmiNews() const {
    return Ctx.HasExpFlag(ALICE_SMI_NEWS_DISABLED_EXP_FLAG) && IsSmiRequest();
}

void TNewsHandlerHelper::ProcessResult() {
    // DIALOG-6201: Change Alice speech speed.
    CheckSpeakerSpeedDirective();
    // DIALOG-5996: Remove first snippet sentence which repeat title.
    if (auto threshold = GetSkipTitleExpThreshold(Ctx)) {
        SkipRepeatableNewsTitles(*threshold);
    }

    if (IsSmiRequest()) {
        SkipRepeatedNews();
    }
}

// --------------------------------------------------------------- TTextComparer

TTextComparer::TTextComparer() :
    Tokenizer(NTextProcessing::NTokenizer::TTokenizerOptions{
        .Lowercasing = true,
        .SeparatorType = NTextProcessing::NTokenizer::ESeparatorType::BySense,
        .SubTokensPolicy = NTextProcessing::NTokenizer::ESubTokensPolicy::SeveralTokens
    })
{
}

const NDict::NLemmerProxy::TLemmerProxy* InitLemmer() {
    static bool inited = false;
    if (!inited) {
        TTempFileHandle file;
        TOFStream output(file.GetName());
        output << NResource::Find("news_lemmer.bin");
        output.Flush();
        NDict::NLemmerProxy::TLemmerProxy::Load(ELanguage::LANG_RUS, file.GetName());
        inited = true;
    }
    return NDict::NLemmerProxy::TLemmerProxy::Get(ELanguage::LANG_RUS);
}

void TTextComparer::SplitToWords(TStringBuf text, TVector<TString>& words) const {
    words = Tokenizer.Tokenize(text);
    EraseIf(words, [](const TString& word) { return GetNumberOfUTF8Chars(word) <= 2; });

    // Y_ENSURE(Lemmer, "llemmer");
    // Lemmatize  TODO: add other languages
    ForEach(words.begin(), words.end(), [](TString& word){
        const auto w = UTF8ToWide(word);
        TWLemmaArray lemmas;
        NLemmer::AnalyzeWord(w.data(), w.size(), lemmas, TLangMask(LANG_RUS));
        word = WideToUTF8(lemmas.front().GetText());
    });

    Sort(words);
}

// static
TStringBuf TTextComparer::GetFirstSentence(TStringBuf text) {
    TStringBuf firstSentence, other;
    text.Split(". ", firstSentence, other);
    return firstSentence;
};

float TTextComparer::GetSimilarity(TStringBuf title, TStringBuf text) const {
    TVector<TString> titleWords, textWords, intersection;
    SplitToWords(title, titleWords);
    if (titleWords.empty()) {
        return 0;
    }

    THPTimer watcher;
    SplitToWords(text, textWords);
    SetIntersection(titleWords.begin(), titleWords.end(), textWords.begin(), textWords.end(),
                    std::back_inserter(intersection));
    return static_cast<float>(intersection.size()) / titleWords.size();
}

// ------------------------------------------------------------ TNewsFormHandler

TResultValue TNewsFormHandler::Do(TRequestHandler& r) {
    class TErrorWriter {
    public:
        explicit TErrorWriter(TContext& ctx)
            : Ctx(ctx)
            , NewsData(ctx.GlobalCtx().NewsData()) {
        }
        ~TErrorWriter() {
            if (!Value) {
                return;
            }

            Ctx.AddErrorBlock(*Value);
            AddAdditionalSuggests(Ctx);
            NewsData.MakeSuggests(Ctx);
        }

        TResultValue SetResult(TResultValue rv) {
            Value = std::move(rv);
            return TResultValue();
        }

        TString GetErrorType() const {
            if (Value) {
                return ToString(Value->Type);
            }
            return TString();
        }

    private:
        TContext& Ctx;
        const TNewsData& NewsData;
        TResultValue Value;
    };

    THPTimer watcher;
    TContext& ctx = r.Ctx();
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::GET_NEWS);
    const TContext::TSlot* slotWhere = ctx.GetSlot(SLOT_WHERE);
    const TContext::TSlot* slotWhereId = ctx.GetSlot(SLOT_WHERE_ID);

    // Step by step all methods will be moved to helper class.
    TNewsHandlerHelper helper(ctx);
    LOG(INFO) << "request inited: " << watcher.PassedReset() << Endl;

    THolder<TRequestType> request;
    if (!IsSlotEmpty(slotWhere) || !IsSlotEmpty(slotWhereId)) {
        TRequestedGeo geo(ctx, slotWhereId, true);
        if (geo.HasError()) {
            LOG(INFO) << "falling back from where_id slot to where" << Endl;
            geo = TRequestedGeo(ctx, slotWhere, true);
        }
        LOG(INFO) << "geo constructor: " << watcher.PassedReset() << Endl;
        // Dirty hack - process request as different scenario.
        if (geo.HasError() && *geo.GetError() == TError::EType::NOSAVEDADDRESS) {
            LOG(INFO) << "geo error: " << watcher.PassedReset() << Endl;
            return TSaveAddressHandler::SetAsResponse(ctx, geo.GetSpecialLocation());
        }
        request = helper.ProcessGeoRequest(std::move(geo));
        LOG(INFO) << "geo request inited: " << watcher.PassedReset() << Endl;
    } else {
        request = helper.MakeDefaultRequest();
        LOG(INFO) << "default request inited: " << watcher.PassedReset() << Endl;
    }
    Y_ENSURE(request, "TRequestType instance must be created");

    TErrorWriter errorWriter(ctx);
    auto err = request->Process();
    LOG(INFO) << request->GetNewsType() << " processed: " << watcher.PassedReset() << Endl;

    if (err.Defined() && request->GetNewsType() != NEWS_WIZARD_TYPE && !request->IsContinuousRequest()) {
        LOG(INFO) << "Switching to websearch answer due to API request fail: " << err->Msg << Endl;
        err = helper.MakeSearchRequest()->Process();
        LOG(INFO) << "Search reprocessed: " << watcher.PassedReset() << Endl;
    }

    errorWriter.SetResult(err);

    // Use |errorType| string to log possible 'system' error and 'nonews' value from enum declaration.
    const TString errorType = errorWriter.GetErrorType();
    if (errorType.empty()) {
        request->AddPostNewsPhraseId();
        request->AddDivCard();
        LOG(INFO) << "divcard added: " << watcher.PassedReset() << Endl;
        helper.ProcessResult();
        LOG(INFO) << "request postprocessed: " << watcher.PassedReset() << Endl;
    }
    // DIALOG-6057: Log answer type.
    request->AddAnalyticsNewsType(errorType);

    if (ctx.MetaClientInfo().IsYaAuto()) {
        ctx.AddStopListeningBlock();
    }
    if ((ctx.MetaClientInfo().IsSmartSpeaker() || ctx.MetaClientInfo().IsTvDevice()) &&
        !NPlayer::AreAudioPlayersPaused(ctx))
    {
        ctx.AddCommand<TPlayerPauseDirective>("player_pause", NSc::TValue().SetDict(), true /*beforeTts*/);
        ctx.AddCommand<TClearQueueDirective>("clear_queue", NSc::TValue().SetDict(), true /*beforeTts*/);
    }

    LOG(INFO) << "scenario end: " << watcher.PassedReset() << Endl;
    return TResultValue();
}

void TNewsFormHandler::Register(THandlersMap* handlers) {
    auto cb = []() {
        return MakeHolder<TNewsFormHandler>();
    };

    handlers->RegisterFormHandler(MAIN_FORM_NAME, cb);
    handlers->RegisterFormHandler(MAIN_ALT_FORM_NAME, cb);
    handlers->RegisterFormHandler(MORE_NEWS_FORM_NAME, cb);
    handlers->RegisterFormHandler(ELLIPSIS_NEWS_FORM_NAME, cb);
    handlers->RegisterFormHandler(COLLECT_CARDS_FORM_NAME, cb);
    handlers->RegisterFormHandler(COLLECT_TEASERS_PREVIEW_FORM_NAME, cb);
    handlers->RegisterFormHandler(COLLECT_MAIN_SCREEN_FORM_NAME, cb);
    handlers->RegisterFormHandler(COLLECT_WIDGET_GALLERY_FORM_NAME, cb);
}

} // namespace NBASS::NNews
