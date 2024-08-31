#include "happy_new_year.h"

#include <alice/bass/forms/search/search.h>

#include <alice/library/analytics/common/product_scenarios.h>

#include <util/string/split.h>

namespace NBASS {

namespace {

constexpr TStringBuf HNY_BLOGGERS_DIV_CARD = "hny_bloggers";
constexpr TStringBuf HNY_COLLECTION_DIV_CARD = "hny_collection";
constexpr TStringBuf HNY_ONBOARDING_DIV_CARD = "hny_onboarding";
constexpr TStringBuf HNY_PRESENT_DIV_CARD = "hny_present";

constexpr TStringBuf HNY_COOK = "personal_assistant.scenarios.hny.cook";
constexpr TStringBuf HNY_DECORATE = "personal_assistant.scenarios.hny.decorate";
constexpr TStringBuf HNY_GENERAL = "personal_assistant.scenarios.hny.general";
constexpr TStringBuf HNY_HAVE_FUN = "personal_assistant.scenarios.hny.have_fun";
constexpr TStringBuf HNY_LISTEN = "personal_assistant.scenarios.hny.listen";
constexpr TStringBuf HNY_OFFER = "personal_assistant.scenarios.hny.offer";
constexpr TStringBuf HNY_PRESENT = "personal_assistant.scenarios.hny.present";
constexpr TStringBuf HNY_SEE = "personal_assistant.scenarios.hny.see";
constexpr TStringBuf HNY_WEAR = "personal_assistant.scenarios.hny.wear";

constexpr TStringBuf HNY_PHRASE_SUFFIX = "_phrase";
constexpr TStringBuf HNY_PREFIX = "personal_assistant.scenarios.hny.";

const TSet<TStringBuf> HNY_UNSUPPORTED = {
    TStringBuf("personal_assistant.scenarios.hny.ivleeva"),
    TStringBuf("personal_assistant.scenarios.hny.novikov"),
};

const THashMap<TStringBuf, TVector<TStringBuf>> HNY_BLOGGERS_BY_FORMNAME = {
    {HNY_COOK, {"semenihin", "parfenon", "pokashevarim", "grilkov", "ya_cook"}},
    {HNY_DECORATE, {"trubenkova", "may", "bubenitta", "ya_decorate"}},
    {HNY_HAVE_FUN, {"varlamov", "suhov", "ya_have_fun"}},
    {HNY_LISTEN, {"jarahov", "gagarina", "dakota", "ya_listen"}},
    {HNY_OFFER, {"wylsacom", "smetana", "slivki", "ya_offer"}},
    {HNY_SEE, {"review", "bekmambetov", "badcomedian", "ya_see"}},
    {HNY_WEAR, {"anohina", "lisovec", "viskunova", "ya_wear"}},
    };

TStringBuf GetLastIntentNameAfterDot(const TStringBuf input) {
    return input.substr(input.find_last_of('.') + 1);
}

} // namespace

TResultValue THappyNewYearHandler::Do(TRequestHandler& r) {
    r.Ctx().GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::PROMO);
    TStringBuf formName = r.Ctx().FormName();

    if (HNY_UNSUPPORTED.contains(formName)) {
        if (TSearchFormHandler::SetAsResponse(r.Ctx(), false)) {
            return r.Ctx().RunResponseFormHandler();
        }
        return TResultValue();
    }

    NSc::TValue data;
    if (formName == HNY_GENERAL) {
        for (const auto& hnyCase: HNY_BLOGGERS_BY_FORMNAME) {
            NSc::TValue item;
            item["name"] = GetLastIntentNameAfterDot(hnyCase.first);
            data.Push(item);
        }
        r.Ctx().AddTextCardBlock(TStringBuilder() << HNY_ONBOARDING_DIV_CARD << HNY_PHRASE_SUFFIX);
        if (r.Ctx().ClientFeatures().SupportsDivCards()) {
            r.Ctx().AddDivCardBlock(HNY_ONBOARDING_DIV_CARD, data);
        }
    } else if (formName == HNY_PRESENT) {
        if (r.Ctx().ClientFeatures().SupportsDivCards()) {
            r.Ctx().AddDivCardBlock(HNY_PRESENT_DIV_CARD, data);
        }
        r.Ctx().AddTextCardBlock(TStringBuilder() << HNY_PRESENT_DIV_CARD << HNY_PHRASE_SUFFIX, data);
    } else if (HNY_BLOGGERS_BY_FORMNAME.contains(formName)) {
        for (const auto& hnyBlogger: HNY_BLOGGERS_BY_FORMNAME.at(formName)) {
            NSc::TValue item;
            item["name"] = hnyBlogger;
            data.Push(item);
        }
        r.Ctx().AddTextCardBlock(TStringBuilder() << HNY_BLOGGERS_DIV_CARD << "_" << GetLastIntentNameAfterDot(formName) << HNY_PHRASE_SUFFIX);
        if (r.Ctx().ClientFeatures().SupportsDivCards()) {
            r.Ctx().AddDivCardBlock(HNY_BLOGGERS_DIV_CARD, data);
        }
    } else {
        data["name"] = GetLastIntentNameAfterDot(formName);
        if (r.Ctx().ClientFeatures().SupportsDivCards()) {
            r.Ctx().AddDivCardBlock(HNY_COLLECTION_DIV_CARD, data);
        }
        r.Ctx().AddTextCardBlock(TStringBuilder() << HNY_COLLECTION_DIV_CARD << HNY_PHRASE_SUFFIX, data);
    }
    return TResultValue();
}

void THappyNewYearHandler::Register(THandlersMap* handlers) {
    auto cbHappyNewYearForm = []() {
        return MakeHolder<THappyNewYearHandler>();
    };
    handlers->emplace(HNY_GENERAL, cbHappyNewYearForm);
    handlers->emplace(HNY_PRESENT, cbHappyNewYearForm);
    for (const auto& hnyCase: HNY_BLOGGERS_BY_FORMNAME) {
        handlers->emplace(hnyCase.first, cbHappyNewYearForm);
        for (const auto& hnyBlogger: hnyCase.second) {
            TStringBuilder bloggerIntent;
            bloggerIntent << HNY_PREFIX << hnyBlogger;
            handlers->emplace(bloggerIntent, cbHappyNewYearForm);
        }
    }
    // fix for removed bloggers:
    for (const auto& unsupportedName: HNY_UNSUPPORTED) {
        handlers->emplace(unsupportedName, cbHappyNewYearForm);
    }
}

} // namespace NBASS
