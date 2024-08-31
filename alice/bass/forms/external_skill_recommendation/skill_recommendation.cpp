#include "skill_recommendation.h"

#include <alice/megamind/protos/scenarios/response.pb.h>

#include <alice/bass/forms/external_skill_recommendation/card_block.sc.h>
#include <alice/bass/forms/external_skill_recommendation/service_response.sc.h>

#include <alice/bass/forms/external_skill/dj_entry_point.h>
#include <alice/bass/forms/external_skill/fwd.h>
#include <alice/bass/forms/external_skill/skill.h>

#include <alice/bass/forms/common/data_sync_api.h>
#include <alice/bass/forms/directives.h>
#include <alice/bass/forms/games_onboarding.h>
#include <alice/bass/forms/onboarding.h>
#include <alice/bass/forms/session_start.h>
#include <alice/bass/forms/whats_new.h>

#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/metrics/metrics.h>

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/proto/protobuf.h>

#include <alice/megamind/protos/scenarios/directives.pb.h>

#include <library/cpp/scheme/scheme.h>

#include <util/generic/guid.h>
#include <util/string/cast.h>

namespace NBASS::NExternalSkill {
namespace {

constexpr TStringBuf SKILL_RECOMMENDATION_FORM_NAME = "personal_assistant.scenarios.skill_recommendation";
constexpr TStringBuf SKILL_RECOMMENDATION_NEXT_FORM_NAME = "personal_assistant.scenarios.skill_recommendation__next";
constexpr TStringBuf SKILL_RECOMMENDATION_CARD_ID = "skill_recommendation";
constexpr TStringBuf SHOW_BUTTONS_DIRECTIVE = "show_buttons";
constexpr TStringBuf FILL_CLOUD_UI_DIRECTIVE = "fill_cloud_ui";
constexpr TStringBuf CLOUD_UI_HARDCODED_TEXT = "Чем могу помочь?";
constexpr size_t TEXTANSWER_GAMES_ONBOARDING_DISPLAYED_SKILLS_COUNT = 1;
constexpr size_t TEXTANSWER_DEFAULT_DISPLAYED_SKILLS_COUNT = 3;
constexpr size_t TEXTANSWER_MAX_SUGGESTED_SKILLS_COUNT = 7;
constexpr size_t CARDANSWER_MAX_NUMBER_OF_CARDS_TO_VOICE = 2;

using TServiceResponseItem = NBASSSkill::TServiceResponse<TSchemeTraits>::TItemConst;
using TCardBlockRef = NBASSSkill::TCardBlock<TSchemeTraits>;
using TCardBlockItemRef = NBASSSkill::TCardBlock<TSchemeTraits>::TCase;

TStringBuf GetFormNameForUpdate(EServiceRequestCard cardName) {
    switch (cardName) {
        case EServiceRequestCard::GamesOnboarding: {
            return TGamesOnboardingHandler::GameOnboardingFormName;
        }
        case EServiceRequestCard::WhatsNew: {
            return TWhatsNewHandler::WhatsNewFormName;
        }
        default: {
            return TOnboardingHandler::OnboardingFormName;
        }
    }
}

size_t GetTextAnswersCount(EServiceRequestCard cardName) {
    switch (cardName) {
        case EServiceRequestCard::GamesOnboarding: {
            return TEXTANSWER_GAMES_ONBOARDING_DISPLAYED_SKILLS_COUNT;
        }
        case EServiceRequestCard::GetGreetings: {
            return 0;
        }
        default: {
            return TEXTANSWER_DEFAULT_DISPLAYED_SKILLS_COUNT;
        }
    }
}

TSchemeTraits::TStringType GetStoreUrl(EServiceRequestCard cardName, TContext& ctx) {
    auto config = ctx.GetConfig().Vins().ExternalSkillsRecommender();
    if (ctx.HasExpFlag(ToString(ESkillRecommendationExperiment::OnboardingUrl))) {
        return cardName == EServiceRequestCard::GamesOnboarding
               ? config.StoreOnboardingGamesUrl()
               : config.StoreOnboardingUrl();
    } else {
        return cardName == EServiceRequestCard::GamesOnboarding
               ? config.OnboardingGamesUrl()
               : config.OnboardingUrl();
    }
}

void RenderItem(TCardBlockItemRef caseRef, const TServiceResponseItem& rec, const TServiceResponse& response, TContext& ctx) {
    caseRef->Idx() = rec->Id();
    caseRef->Description() = rec->Description();
    caseRef->Activation() = rec->Activation();
    caseRef->RecommendationType() = response->RecommendationType();
    caseRef->RecommendationSource() = response->RecommendationSource();
    caseRef->Look() = rec->Look();
    caseRef->Logo() = ConstructLogoUrl(rec->Look(), rec->LogoPrefix(), rec->LogoAvatarId(), ctx);
    caseRef->Name() = rec->Name();
    if (rec->HasLogoAmelieBgUrl()) {
        caseRef->LogoAmelieBgUrl() = rec->LogoAmelieBgUrl();
    }
    if (rec->HasLogoAmelieBgWideUrl()) {
        caseRef->LogoAmelieBgWideUrl() = rec->LogoAmelieBgWideUrl();
    }
    if (rec->HasLogoAmelieFgUrl()) {
        caseRef->LogoAmelieFgUrl() = rec->LogoAmelieFgUrl();
    }
    if (rec->HasLogoFgImageId()) {
        caseRef->LogoFgImage() = TSkillDescription::CreateImageUrl(ctx, rec->LogoFgImageId(), IMAGE_TYPE_LOGO_FG_IMG, AVATAR_NAMESPACE_SKILL_LOGO);
    }
    if (rec->HasLogoBgImageId()) {
        caseRef->LogoBgImage() = TSkillDescription::CreateImageUrl(ctx, rec->LogoBgImageId(), IMAGE_TYPE_LOGO_BG_IMG, AVATAR_NAMESPACE_SKILL_LOGO);
    }
    caseRef->LogoBgColor() = rec->LogoBgColor();
}

void RenderResponseAsDivCard(EServiceRequestCard cardName, const TServiceResponse& response, TContext& ctx) {
    NSc::TValue voiceValue;
    NSc::TValue cardValue;
    TCardBlockRef cardRef(&cardValue);
    cardRef->StoreUrl() = GetStoreUrl(cardName, ctx);

    size_t i = 0;
    for (const TServiceResponseItem& rec : response->Items()) {
        TCardBlockItemRef caseRef = cardRef->Cases().Add();
        RenderItem(caseRef, rec, response, ctx);

        if (i++ < CARDANSWER_MAX_NUMBER_OF_CARDS_TO_VOICE) {
            voiceValue["cases"].GetArrayMutable().push_back(*caseRef.GetRawValue());
        }
    }

    if (cardName != EServiceRequestCard::GetGreetings) {
        ctx.AddTextCardBlock("onboarding__set_message", std::move(voiceValue));
    }

    ctx.AddDivCardBlock(SKILL_RECOMMENDATION_CARD_ID, std::move(cardValue));

    if ((ctx.HasExpFlag(EXPERIMENTAL_FLAG_ONBOARDING_MULTICOLUMN_CARD) && cardName == EServiceRequestCard::Onboarding) ||
        (ctx.HasExpFlag(EXPERIMENTAL_FLAG_GAMES_ONBOARDING_MULTICOLUMN_CARD) && cardName == EServiceRequestCard::GamesOnboarding)) {
        NSc::TValue btnValue;
        TCardBlockRef btnRef(&btnValue);
        btnRef->StoreUrl() = GetStoreUrl(cardName, ctx);
        ctx.AddDivCardBlock("skill_recommendation_skills_store_button", std::move(btnValue));
    }

    if (cardName == EServiceRequestCard::WhatsNew) {
        ctx.AddSuggest("onboarding__news");
    }

    if (ctx.HasExpFlag(EXPERIMENTAL_FLAG_SKILLREC_SUGGEST_ELLIPSIS)
        && (cardName == EServiceRequestCard::Onboarding || cardName == EServiceRequestCard::GamesOnboarding))
    {
        ctx.AddSuggest("skillrec__next");
    } else {
        NSc::TValue formUpdate;
        formUpdate["name"] = GetFormNameForUpdate(cardName == EServiceRequestCard::WhatsNew ? EServiceRequestCard::Onboarding : cardName);
        formUpdate["resubmit"].SetBool(true);
        ctx.AddSuggest("onboarding__next", NSc::Null(), std::move(formUpdate));
    }
}

void FillFrameActionTypeTextDirective(NAlice::NScenarios::TTypeTextDirective& typeTextDirective, const TServiceResponseItem& item) {
    typeTextDirective.SetText(TStringBuf(item->Activation()).data());
}

void FillFrameActionExternalSourceCallback(NAlice::NScenarios::TCallbackDirective& externalSourceActionDirective, const TServiceResponse& response) {
    externalSourceActionDirective.SetName("external_source_action");
    externalSourceActionDirective.SetIgnoreAnswer(true);
    *externalSourceActionDirective.MutablePayload() = NAlice::TProtoStructBuilder()
            .Set("utm_source", "Yandex_Alisa")
            .Set("utm_campaign", "")
            .Set("utm_term", "")
            .Set("utm_content", "textlink")
            .Set("utm_medium", TString{response->RecommendationSource()})
            .Build();
}

void FillFrameActionOnCardCallback(NAlice::NScenarios::TCallbackDirective& onCardActionDirective, const TServiceResponse& response, const TServiceResponseItem& item,
        const size_t itemNumber) {
    onCardActionDirective.SetName("on_card_action");
    onCardActionDirective.SetIgnoreAnswer(true);
    *onCardActionDirective.MutablePayload() = NAlice::TProtoStructBuilder()
            .Set("card_id", SKILL_RECOMMENDATION_CARD_ID.data())
            .Set("intent_name", SKILL_RECOMMENDATION_FORM_NAME.data())
            .Set("item_number", ToString(itemNumber + 1))
            .Set("case_name", TString::Join(
                "skill_recommendation__",
                TString{response->RecommendationSource()},
                "__",
                TString{response->RecommendationType()},
                "__",
                TString{item->Id()}))
            .Build();
}

NAlice::NScenarios::TFrameAction ConstructFrameAction(const TServiceResponse& response, const TServiceResponseItem& item,
        const size_t itemNumber) {
    NAlice::NScenarios::TFrameAction frameAction;
    auto& directives = *frameAction.MutableDirectives()->MutableList();

    // add "type" directive - imitates typing in the chat
    FillFrameActionTypeTextDirective(*directives.Add()->MutableTypeTextDirective(), item);

    // add "external_source_action" directive
    FillFrameActionExternalSourceCallback(*directives.Add()->MutableCallbackDirective(), response);

    // add "on_card_action" directive
    FillFrameActionOnCardCallback(*directives.Add()->MutableCallbackDirective(), response, item, itemNumber);

    return frameAction;
}

NSc::TValue ConstructShowButtonData(const TString& actionId, const TServiceResponseItem& item, const TContext& ctx) {
    NSc::TValue data;
    data["action_id"].SetString(actionId);
    data["title"].SetString(TString{item->Name()});
    data["text"].SetString(TString{item->Description()});

    NSc::TValue theme;
    theme["image_url"].SetString(ConstructLogoUrl(item->Look(), item->LogoPrefix(), item->LogoAvatarId(), ctx));
    data["theme"].Swap(theme);

    return data;
}

void AddShowButtonDirectiveAndActions(const TServiceResponse& response, TContext& ctx) {
    NSc::TValue showButtonsDirective;
    showButtonsDirective["screen_id"].SetString("cloud_ui");

    size_t itemNumber = 0;
    for (const TServiceResponseItem& item : response->Items()) {
        const TString actionId = CreateGuidAsString();

        // add TFrameAction
        const auto frameAction = ConstructFrameAction(response, item, itemNumber);
        ctx.AddFrameActionBlock(actionId, frameAction);

        // add button data
        showButtonsDirective["buttons"].Push() = ConstructShowButtonData(actionId, item, ctx);

        ++itemNumber;
    }
    ctx.AddCommand<TShowButtonsDirective>(SHOW_BUTTONS_DIRECTIVE, std::move(showButtonsDirective));

    if (ctx.ClientFeatures().SupportsCloudUiFilling()) {
        NSc::TValue fillCloudUiDirective;
        fillCloudUiDirective["text"].SetString(CLOUD_UI_HARDCODED_TEXT);
        ctx.AddCommand<TFillCloudUiDirective>(FILL_CLOUD_UI_DIRECTIVE, std::move(fillCloudUiDirective));
    }
}

TVector<TServiceResponseItem> GetSkillsToDisplay(EServiceRequestCard cardName, const TServiceResponse& response) {
    TVector<TServiceResponseItem> res;

    if (cardName == EServiceRequestCard::Onboarding && response->HasEditorsAnswer()) {
        THashSet<TString> matchedSkills;
        for (const auto& skill : response->EditorsAnswer().Matcher().Skills().Values()) {
            matchedSkills.insert(TString(skill));
        }
        for (const TServiceResponseItem& skill : response->Items()) {
            if (matchedSkills.contains(TString(skill.Id()))) {
                res.push_back(skill);
            }
        }
    } else {
        size_t textAnswersCount = GetTextAnswersCount(cardName);
        for (size_t i = 0; i < textAnswersCount && i < response->Items().Size(); ++i) {
            res.push_back(response->Items(i));
        }
    }
    return res;
}

TVector<TServiceResponseItem> GetSkillsToSuggest(const TServiceResponse& response, TConstArrayRef<const TServiceResponseItem> skillsToDispl) {
    TVector<TServiceResponseItem> res;

    for (const TServiceResponseItem& skill : response->Items()) {
        bool ok = true;
        for (const TServiceResponseItem& skillToDispl : skillsToDispl) {
            if (skill.Id().Get() == skillToDispl.Id().Get()) {
                ok = false;
            }
        }
        if (ok) {
            res.push_back(skill);
        }
        if (res.size() >= TEXTANSWER_MAX_SUGGESTED_SKILLS_COUNT) {
            break;
        }
    }
    return res;
}

void RenderResponseAsText(EServiceRequestCard cardName, const TServiceResponse& response, TContext& ctx) {
    TVector<TServiceResponseItem> skillsToDisplay = GetSkillsToDisplay(cardName, response);

    const bool disableTextCard = cardName == EServiceRequestCard::GetGreetings && ctx.MetaClientInfo().IsNavigator();
    if (!disableTextCard) {
        NSc::TValue data;
        for (const auto &skill : skillsToDisplay) {
            NSc::TValue answer;
            TCardBlockItemRef caseRef(&answer);
            RenderItem(caseRef, skill, response, ctx);
            data["cases"].Push(std::move(answer));
        }
        if (response->HasEditorsAnswer()) {
            data["editors_answer"] = response->EditorsAnswer().Text();
        }
        ctx.AddTextCardBlock("onboarding__skills", std::move(data));
    }

    if (cardName == EServiceRequestCard::GetGreetings && !ctx.MetaClientInfo().IsNavigator()) {
        //SmartSpeakers, YaMusic, YaAuto do not get here, so it works only for other non-card clients
        ctx.AddOnboardingSuggest();
    }

    TVector<TServiceResponseItem> skillsToSuggest = GetSkillsToSuggest(response, skillsToDisplay);
    for (const auto& skill : skillsToSuggest) {
        NSc::TValue answer;
        TCardBlockItemRef caseRef(&answer);
        RenderItem(caseRef, skill, response, ctx);
        ctx.AddSuggest("onboarding__skill", std::move(answer));
    }
}

TResultValue RenderResponseFailure(TContext& ctx, EServiceRequestCard cardName) {
    if (cardName == EServiceRequestCard::GamesOnboarding || cardName == EServiceRequestCard::WhatsNew) { // Do we have a default form?
        if (ctx.ClientFeatures().SupportsDivCards() && cardName == EServiceRequestCard::GamesOnboarding) {
            ctx.AddDivCardBlock("onboarding__default_card", NSc::TValue());
        } else {
            ctx.AddTextCardBlock("onboarding__default_answer");
        }
        return TResultValue();
    }
    if (TSessionStartFormHandler::SetAsResponse(ctx)) {
        return ctx.RunResponseFormHandler();
    }
    if (!ctx.Meta().Utterance().Get().Empty()) {
        ctx.AddSearchSuggest();
    }
    return TResultValue();
}

///////////////////////////////////////////////////////////////////////////////
////
//// TSkillRecommendationFormHandler
////
class TSkillRecommendationFormHandler: public IHandler {
public:
    TResultValue Do(TRequestHandler& r) override;
};

///////////////////////////////////////////////////////////////////////////////
////
//// TSkillRecommendationFormHandler implementation
////

TResultValue TSkillRecommendationFormHandler::Do(TRequestHandler& r) {
    Y_STATS_SCOPE_HISTOGRAM(ToString(EStatsIncConuter::SkillRecommendation));
    TContext& ctx = r.Ctx();
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::ONBOARDING);
    TContext::TSlot* cardNameSlot = ctx.GetOrCreateSlot(TStringBuf("card_name"), TStringBuf("string"));

    auto cardName = EServiceRequestCard::Onboarding;
    if (!cardNameSlot->Value.IsNull()) {
        TryFromString(cardNameSlot->Value.GetString(), cardName);
    }
    Y_STATS_INC_COUNTER_IF(!ctx.IsTestUser(), ToString(EStatsIncConuter::SkillRecommendationCard) + ToString(cardName));

    TServiceResponse response;
    if (!TryGetRecommendationsFromService(ctx, cardName, response) || response->Items().Empty()) {
        Y_STATS_INC_COUNTER_IF(!ctx.IsTestUser(), ToString(EStatsIncConuter::SkillRecommendationZeroResponse));
        return RenderResponseFailure(ctx, cardName);
    }
    LOG(DEBUG) << "Got AliceSkillService response: " << response->ToJson() << Endl;

    if (ctx.ClientFeatures().SupportsDivCards()) {
        RenderResponseAsDivCard(cardName, response, ctx);
    } else {
        RenderResponseAsText(cardName, response, ctx);
    }

    const THashSet<EServiceRequestCard> CLOUD_UI_CARDS = {
        EServiceRequestCard::GetGreetings,
        EServiceRequestCard::FirstSessionOnboarding,
    };
    if (ctx.HasExpFlag(NAlice::NExperiments::ONBOARDING_USE_CLOUD_UI)
            && ctx.ClientFeatures().SupportsCloudUi()
            && CLOUD_UI_CARDS.contains(cardName)) {
        AddShowButtonDirectiveAndActions(response, ctx);
    }

    if (!ctx.Meta().Utterance().Get().Empty()) {
        ctx.AddSearchSuggest();
    }

    return TResultValue();
}

} // namespace anonymous

///////////////////////////////////////////////////////////////////////////////
////
//// TSkillRecommendationInitializer implementation
////

// static
void TSkillRecommendationInitializer::Register(THandlersMap* handlers) {
    handlers->emplace(SKILL_RECOMMENDATION_NEXT_FORM_NAME,
            []() { return MakeHolder<TSkillRecommendationFormHandler>(); });
    handlers->emplace(SKILL_RECOMMENDATION_FORM_NAME,
            []() { return MakeHolder<TSkillRecommendationFormHandler>(); });
}

// static
TContext::TPtr TSkillRecommendationInitializer::SetAsResponse(TContext& ctx, EServiceRequestCard requestCard) {
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::ONBOARDING);
    auto ctxResponse = ctx.SetResponseForm(SKILL_RECOMMENDATION_FORM_NAME, false);
    if (ctxResponse) {
        TContext::TSlot* cardNameSlot = ctxResponse->GetOrCreateSlot(TStringBuf("card_name"), TStringBuf("string"));
        cardNameSlot->Value.SetString(ToString(requestCard));
    }

    return ctxResponse;
}

} // namespace NBASS::NExternalSkill

