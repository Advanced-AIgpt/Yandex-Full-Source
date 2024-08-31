#include "postroll.h"
#include "postroll_actions.h"

#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/library/modifiers/context.h>
#include <alice/megamind/library/modifiers/utils.h>
#include <alice/megamind/library/proactivity/common/common.h>
#include <alice/megamind/library/scenarios/defs/names.h>

#include <alice/megamind/protos/modifiers/modifiers.pb.h>
#include <alice/memento/proto/user_configs.pb.h>
#include <alice/protos/data/contextual_data.pb.h>
#include <alice/protos/data/proactivity/analytics.pb.h>

#include <alice/library/experiments/experiments.h>
#include <alice/library/frame/description.h>
#include <alice/library/metrics/names.h>
#include <alice/library/proactivity/apply_conditions/check_apply_conditions.h>
#include <alice/library/proactivity/success_conditions/match_success_conditions.h>

#include <dj/lib/proto/action.pb.h>
#include <dj/services/alisa_skills/profile/proto/profile_enums.pb.h>
#include <dj/services/alisa_skills/server/proto/data/data_types.pb.h>

#include <library/cpp/expression/expression.h>

#include <util/generic/algorithm.h>
#include <util/generic/is_in.h>
#include <util/generic/utility.h>
#include <util/random/shuffle.h>
#include <util/string/cast.h>

namespace NAlice::NMegamind {

namespace {

// In seconds
constexpr ui64 MAX_STORAGE_UPDATE_TIME_DELTA = 21600;
constexpr ui64 SHOWS_HISTORY_LEN = 5;

using TCtx = TResponseModifierContext;
using TDirectives = NProtoBuf::RepeatedPtrField<NAlice::NSpeechKit::TDirective>;
using TApplyConditions = NProtoBuf::RepeatedPtrField<NDJ::NAS::TApplyCondition>;

constexpr TStringBuf MODIFIER_NAME = "Postroll";

const TString FRAME_CONFIRM = "alice.proactivity.confirm";
const TString FRAME_DECLINE = "alice.proactivity.decline";
const TString FRAME_DO_NOTHING = "alice.do_nothing";
const TString VIEW_ACTION_TYPE = "View";
constexpr TStringBuf ACTION_NAME = "postroll_action";
constexpr TStringBuf ACTION_NAME_DECLINE = "postroll_decline";

TProactivityStorage GetProactivityStorage(const TResponseModifierContext& ctx) {
    return GetProactivityStorage(ctx.SpeechKitRequest(), ctx.Session(), ctx.UserConfigs().GetProactivityConfig().GetStorage());
}

void LoadStorageData(TCtx& ctx) {
    *ctx.ModifiersStorage().MutableProactivity() = GetProactivityStorage(ctx);
}

void SaveStorage(const TCtx& ctx, TScenarioResponse& response) {
    if (!ctx.HasExpFlag(EXP_PROACTIVITY_DISABLE_MEMENTO)) {
        ru::yandex::alice::memento::proto::TProactivityConfig config;
        config.MutableStorage()->CopyFrom(ctx.ModifiersStorage().GetProactivity());
        AddMementoDirectiveToResponse(response, ru::yandex::alice::memento::proto::EConfigKey::CK_PROACTIVITY, config);
    }
    // Clear datasync
    AddDatasyncDirectiveToResponse(response, PROACTIVITY_HISTORY, "{}");
}

class TPostrollModifierContext {
private:
    static TErrorOr<bool> CheckApplyConditionResponseComponent(const NDJ::NAS::TApplyCondition::EResponseVoiceOrText condition,
                                                               bool postrollHasComponent, bool responseHasComponent) {
        if (const auto conditionIsOk = NAlice::CheckApplyConditionResponseComponentIsOk(condition, postrollHasComponent, responseHasComponent)) {
            return conditionIsOk.GetRef();
        }

        return TError{TError::EType::ModifierError} << "Unknown EResponseVoiceOrText case";
    }

    TProtoEvaluator& GetEvaluator() const {
        if (Evaluator.Defined()) {
            return Evaluator.GetRef();
        }
        TProtoEvaluator& evaluator = Evaluator.ConstructInPlace(Ctx.Logger());
        // ResponseBody must exist in modifiers
        evaluator.SetProtoRef("AnalyticsInfo", Response.ResponseBodyIfExists()->GetAnalyticsInfo());
        return evaluator;
    }

    static const TApplyConditions& DefaultConditions() {
        static const TApplyConditions DEFAULT_CONDITIONS{[]() {
            TApplyConditions applyConditions;
            applyConditions.Add();
            return applyConditions;
        }()};
        return DEFAULT_CONDITIONS;
    }

public:
    TPostrollModifierContext(TResponseModifierContext& ctx, TScenarioResponse& response)
        : Ctx(ctx)
        , Response(response)
        , CurrentText(GetTextFromResponse(response))
        , CurrentVoice(GetSpeechFromResponse(response))
        , ShouldListen(GetShouldListenFromResponse(response))
        , ResponseHasText(!CurrentText.Empty())
        , ResponseHasVoice(Ctx.HasExpFlag(EXP_PROACTIVITY_DEBUG_TEXT_RESPONSE) || !CurrentVoice.Empty())
        , Directives(response.BuilderIfExists()->GetSKRProto().GetResponse().GetDirectives())
    {
    }

    TApplyResult CheckApplyConditions(const TApplyConditions& conditions, const NDJ::NAS::TProtoItem::TResult::TAliceText& postroll) const {
        const bool isDefault = conditions.empty();
        for (const auto& cond : isDefault ? DefaultConditions() : conditions) {
            const auto& voiceCondOk = CheckApplyConditionResponseComponent(cond.GetVoiceInResponse(), !postroll.GetVoice().empty(), ResponseHasVoice);
            if (!voiceCondOk.IsSuccess()) {
                return *voiceCondOk.Error();
            }
            const auto& textCondOk = CheckApplyConditionResponseComponent(cond.GetTextInResponse(), !postroll.GetText().empty(), ResponseHasText);
            if (!textCondOk.IsSuccess()) {
                return *textCondOk.Error();
            }
            const bool directiveCondOk = !cond.HasDirective() || NAlice::CheckApplyConditionResponseDirectiveIsOk(cond.GetDirective(), Directives);
            const bool checkCondOk = !cond.HasCheck() || GetEvaluator().EvaluateBool(cond.GetCheck());
            if ((!cond.HasListening() || cond.GetListening().value() == ShouldListen)
                && voiceCondOk.Value()
                && textCondOk.Value()
                && directiveCondOk
                && checkCondOk)
            {
                return ApplySuccess();
            }
        }
        return NonApply(isDefault ? "default condition is not satisfied" : "apply condition is not satisfied");
    }

    void SetShouldListen(bool phraseShouldListen) {
        // Keep listening if listened before postroll
        SetShouldListenToResponse(Response, ShouldListen || phraseShouldListen);
    }

private:
    mutable TMaybe<TProtoEvaluatorWithTraceLog> Evaluator;
    TResponseModifierContext& Ctx;
    TScenarioResponse& Response;
    const TStringBuf CurrentText;
    const TStringBuf CurrentVoice;
    const bool ShouldListen;
    const bool ResponseHasText;
    const bool ResponseHasVoice;
    const TDirectives& Directives;
};

void LoadAppProactivitySettings(TCtx& ctx) {
    if (const auto& personalData = ctx.PersonalData()) {
        auto* info = ctx.ModifiersInfo().MutableProactivity();
        info->SetDisabledInApp(IsProactivityDisabledInApp(personalData));
    }
}

struct TTextAndVoice {
    TTextAndVoice(const TString& text, const TString& voice)
        : Text(text)
        , Voice(voice)
    {
    }

    // TString cause TextAdder saves TString references and these values still need to be modified
    TString Text;
    TString Voice;
};

void AppendPhrase(const TCtx& ctx, TScenarioResponse& response, TTextAndVoice textVoice) {
    if (textVoice.Text.empty() && ctx.HasExpFlag(EXP_PROACTIVITY_DEBUG_TEXT_RESPONSE)) {
        textVoice.Text = textVoice.Voice;
    }
    textVoice.Voice = TString::Join(
        ctx.HasExpFlag(EXP_PROACTIVITY_ENABLE_NOTIFICATION_SOUND) ? "<speaker audio=\"postroll_notification_sound.opus\"> " : "",
        "<speaker voice=\"shitova\"",
        ctx.HasExpFlag(EXP_PROACTIVITY_ENABLE_EMOTIONAL_TTS) ? ">" : " emotion=\"neutral\">",
        textVoice.Voice
    );
    AddTextToResponse(response, textVoice.Text, textVoice.Voice, /* appendTts= */ true);
}

void AddFrameAction(TScenarioResponse& response, NScenarios::TFrameAction action,
                    NModifiers::NProactivity::TPostroll& analytics,
                    const NScenarios::TFrameAction* subactionPtr = nullptr) {
    if (!action.HasNluHint()) {
        action.MutableNluHint()->SetFrameName(FRAME_CONFIRM);
    }
    if (subactionPtr) {
        action.MergeFrom(*subactionPtr);
    }
    AddActionToResponse(response, ACTION_NAME, std::move(action));
    analytics.AddFrameActions()->SetName(TString{ACTION_NAME});
}

void AddDeclineAction(TScenarioResponse& response, NModifiers::NProactivity::TPostroll& analytics) {
    NScenarios::TFrameAction action;
    action.MutableNluHint()->SetFrameName(FRAME_DECLINE);
    action.MutableFrame()->SetName(FRAME_DO_NOTHING);
    AddActionToResponse(response, ACTION_NAME_DECLINE, std::move(action));
    analytics.AddFrameActions()->SetName(TString{ACTION_NAME_DECLINE});
}

void UpdateLastPostrollViews(TCtx& ctx, const NDJ::NAS::TProtoItem& item, int showsHistoryLen,
                             const TString& source, const NDJ::NAS::TPostrollContext* context = nullptr) {
    TProactivityStorage& storage = *ctx.ModifiersStorage().MutableProactivity();
    TProactivityStorage_TPostrollView postrollView;
    postrollView.SetItemId(item.GetId());
    postrollView.SetBaseId(item.GetBaseItem());
    *postrollView.MutableAnalytics() = item.GetAnalytics();
    *postrollView.MutableTags() = item.GetTags();
    postrollView.SetSource(source);
    if (context) {
        postrollView.MutableContext()->CopyFrom(*context);
    }
    postrollView.SetRequestId(ctx.SpeechKitRequest().RequestId());

    *(storage.AddLastPostrollViews()) = postrollView;
    if (storage.GetLastPostrollViews().size() > showsHistoryLen) {
        const auto first = storage.MutableLastPostrollViews()->begin();
        const auto last = first + (storage.GetLastPostrollViews().size() - showsHistoryLen);
        storage.MutableLastPostrollViews()->erase(first, last);
    }
}

void SaveData(TCtx& ctx, TScenarioResponse& response, const NDJ::NAS::TProtoItem& item,
              int showsHistoryLen, const TString& source, const NDJ::NAS::TPostrollContext& context, const NAlice::NData::NProactivity::ESkillRecQuotaType& quotaType) {
    auto& storage = *ctx.ModifiersStorage().MutableProactivity();
    const auto showTime = ctx.ClientInfo().Epoch;
    const auto showCount = storage.GetPostrollCount() + 1;
    storage.SetLastShowTime(showTime);
    storage.SetPostrollCount(showCount);
    storage.SetLastShowRequestCount(storage.GetRequestCount());
    if (!item.GetTags().empty()) {
        TProactivityStorage_TTagStats tagStats;
        tagStats.SetLastShowTime(showTime);
        tagStats.SetLastShowPostrollCount(showCount);
        tagStats.SetLastShowRequestCount(storage.GetRequestCount());
        for (const auto& tag : item.GetTags()) {
            (*storage.MutableTagStats())[tag] = tagStats;
        }
    }

    auto* info = ctx.ModifiersInfo().MutableProactivity();
    info->SetItemId(item.GetId());
    info->SetFromSkillRec(true);
    if (item.HasAnalytics()) {
        info->SetItemInfo(item.GetAnalytics().GetInfo());
    }
    info->SetAppended(true); // "disable answer" works differently with service
    *info->MutableTags() = item.GetTags();
    info->SetIsMarketingPostroll(item.GetMarketingScore() > 0);
    info->SetQuotaType(quotaType);

    *ctx.ProactivityLogStorage().MutableAnalytics() = item.GetAnalytics();

    if (HasValidSuccessCondition(item.GetAnalytics())) {
        const auto* const contextPtr = ctx.HasExpFlag(EXP_PROACTIVITY_DISABLE_MEMENTO) ? &context : nullptr;
        UpdateLastPostrollViews(ctx, item, showsHistoryLen, source, contextPtr);
        storage.SetLastPostrollRequestId(ctx.SpeechKitRequest().RequestId());
        *ctx.ProactivityLogStorage().AddActions() = MakePostrollAction(ctx.SpeechKitRequest(), item,
            NDJ::NAS::EAlisaSkillsActionType::AT_View, source, TString{}, contextPtr);
        ctx.Sensors().IncRate(NSignal::LabelsForPostrollSource(VIEW_ACTION_TYPE, source));
        ctx.Sensors().IncRate(NSignal::LabelsForPostrollItemInfo(VIEW_ACTION_TYPE, item.GetAnalytics().GetInfo()));
    }

    SaveStorage(ctx, response);
}

// Modifies response
class TPostrollModifier : public TResponseModifier {
public:
    TPostrollModifier()
        : TResponseModifier(MODIFIER_NAME)
    {
    }

    TApplyResult TryApply(TResponseModifierContext& ctx, TScenarioResponse& response) override {
        LoadStorageData(ctx);

        auto& proactivityStorage = *ctx.ModifiersStorage().MutableProactivity();
        proactivityStorage.SetRequestCount(proactivityStorage.GetRequestCount() + 1);

        const auto currTime = ctx.ClientInfo().Epoch;
        const auto lastUpdateTime = proactivityStorage.GetLastStorageUpdateTime();
        if (currTime < lastUpdateTime) {
            return TError{TError::EType::ModifierError} << "Current time is lower than LastStorageUpdateTime";
        }
        // We might not upload updated storage but we change it locally beforehand nonetheless
        proactivityStorage.SetLastStorageUpdateTime(currTime);

        CheckCurrentScenarioForDeclineAction(ctx, response);

        const bool shouldUpdateStorage = CheckCurrentScenarioForClickAction(ctx, response);
        const auto applyResult = TryApplyImpl(ctx, response);
        const bool updatedStorageNow = !applyResult.Error() && !applyResult.Value();
        auto maxStorageUpdateTimeDelta = MAX_STORAGE_UPDATE_TIME_DELTA;
        if (const auto expValue = GetExperimentValueWithPrefix(ctx.ExpFlags(), EXP_PROACTIVITY_STORAGE_UPDATE_TIME_DELTA_PREFIX)) {
            TryFromString(*expValue, maxStorageUpdateTimeDelta);
        }
        const bool updatedStorageRecently = (currTime - lastUpdateTime) < maxStorageUpdateTimeDelta;
        const bool isServerAction = ctx.SpeechKitRequest().Event().GetType() == EEventType::server_action;
        if (IsProactivityAllowedInApp<TResponseModifierContext>(ctx) &&
            !updatedStorageNow &&
            (shouldUpdateStorage || (!updatedStorageRecently && !isServerAction))
        ) {
            SaveStorage(ctx, response);
        }

        return applyResult;
    }

private:
    TApplyResult TryApplyImpl(TResponseModifierContext& ctx, TScenarioResponse& response) {
        // Only for analytics info
        LoadAppProactivitySettings(ctx);

        const auto fullSource = GetProactivitySource(ctx.Session(), response);

        ui64 showsHistoryLen = SHOWS_HISTORY_LEN;
        if (const auto expShowsHistoryLen = GetExperimentValueWithPrefix(ctx.ExpFlags(), EXP_PROACTIVITY_SHOWS_HISTORY_LEN)) {
            TryFromString(*expShowsHistoryLen, showsHistoryLen);
        }

        if (ctx.Proactivity().empty()) {
            return NonApply("no valid SkillRec response for source");
        }

        if (const auto* body = response.ResponseBodyIfExists()) {
            const auto& contextualData = body->GetContextualData();
            if (contextualData.GetProactivity().GetHint() == NData::TContextualData_TProactivity_EHint_AlreadyProactive) {
                return NonApply("scenario response has AlreadyProactive hint");
            }
        }

        TVector<const NDJ::NAS::TProactivityRecommendation*> recommendations;
        recommendations.reserve(ctx.Proactivity().size());
        for (const auto& proactivity : ctx.Proactivity()) {
            if (proactivity.HasItem()) {
                recommendations.push_back(&proactivity);
            }
        }

        if (recommendations.empty()) {
            return TError{TError::EType::ModifierError} << "No item in SkillRec response";
        }

        constexpr auto byScoreDesc = [](auto a, auto b) { return a->GetScore() > b->GetScore(); };
        std::sort(recommendations.begin(), recommendations.end(), byScoreDesc);

        TPostrollModifierContext postrollModifierCtx{ctx, response};

        for (auto it = recommendations.begin();; ++it) {
            const auto& proactivity = **it;
            const auto& item = proactivity.GetItem();
            const auto& result = item.GetResult();
            const auto& postroll = result.GetPostroll();

            const auto& applCondIsOk = postrollModifierCtx.CheckApplyConditions(proactivity.GetConditions(), postroll);
            if (!applCondIsOk.IsSuccess() || applCondIsOk.Value()) {
                if (&proactivity == recommendations.back()) {
                    return applCondIsOk;
                }
                continue;
            }

            // Found true apply condition with max score (recommendations are sorted by score)
            if (result.HasPostroll()) {
                AppendPhrase(ctx, response, TTextAndVoice{postroll.GetText(), postroll.GetVoice()});
            }
            NModifiers::NProactivity::TPostroll analytics{};
            postrollModifierCtx.SetShouldListen(result.GetShouldListen());
            if (result.HasFrameAction()) {
                AddFrameAction(response, result.GetFrameAction(), analytics);
                if (!result.GetDoNotAddDefaultDeclineAction()) {
                    AddDeclineAction(response, analytics);
                }
            }
            ctx.MegamindAnalyticsInfoBuilder().SetPostroll(analytics);

            SaveData(ctx, response, item, showsHistoryLen, fullSource, proactivity.GetContext(), proactivity.GetQuotaType());
            return ApplySuccess();
        }
    }
};

} // namespace

TModifierPtr CreatePostrollModifier() {
    return MakeHolder<TPostrollModifier>();
}

} // NAlice::NMegamind
