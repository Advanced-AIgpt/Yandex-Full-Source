#include "postroll_actions.h"

#include <dj/lib/proto/common_enums.pb.h>
#include <dj/services/alisa_skills/profile/proto/payload.pb.h>

#include <alice/library/metrics/names.h>
#include <alice/megamind/library/proactivity/common/common.h>

#include <alice/library/json/json.h>

#include <contrib/libs/re2/re2/re2.h>

namespace {

const THashSet<TString> STOP_INTENTS = {
    "personal_assistant.handcrafted.cancel",
    "personal_assistant.handcrafted.fast_cancel",
    "personal_assistant.handcrafted.rude",
    "personal_assistant.handcrafted.user_reactions_negative_feedback",
};

const TString CLICK_ACTION_TYPE = "Click";

} // namespace

namespace NAlice::NMegamind {

NDJ::TActionProto MakePostrollAction(const TSpeechKitRequest& skr, const NDJ::NAS::TProtoItem& item,
                                     const NDJ::NAS::EAlisaSkillsActionType actionType, const TString& source,
                                     const TString& postrollViewRequestId, const NDJ::NAS::TPostrollContext* proactivityContext) {
    NDJ::TActionProto action;
    action.SetType(NDJ::ECommonObjectType::OT_User);
    action.SetId(skr.ClientInfo().Uuid);
    action.SetToType(NDJ::NAS::EAlisaSkillsObjectType::OT_Skill);
    action.SetToId(item.GetId());
    action.SetActionType(actionType);
    action.SetValue(1);
    action.SetTimestamp(skr.ClientInfo().Epoch);
    action.SetRequestId(skr.RequestId());

    auto& actionData = *action.MutablePayload()->MutableExtension(NDJ::NAS::TActionPayloadData::AlicePayloadExtension);
    for (const auto& tag : item.GetTags()) {
        *actionData.AddTags() = tag;
    }
    if (!source.empty()) {
        actionData.SetSource(source);
    }
    if (proactivityContext) {
        actionData.MutableContext()->CopyFrom(*proactivityContext);
    }
    if (!postrollViewRequestId.empty()) {
        actionData.SetPostrollViewRequestId(postrollViewRequestId);
    }

    return action;
}

NDJ::NAS::TProtoItem PostrollViewToProtoItem(const TProactivityStorage::TPostrollView& postrollView) {
    NDJ::NAS::TProtoItem protoItem;
    protoItem.SetId(postrollView.GetItemId());
    *protoItem.MutableTags() = postrollView.GetTags();
    return protoItem;
}

namespace {

struct TEvaluatorCtx {
    TProtoEvaluator Evaluator;
    NDJ::NAS::TProactivityRequest::TSemanticFrames SemanticFramesProto;
    NScenarios::TBegemotIotNluResult IotResultProto;

    explicit TEvaluatorCtx(TResponseModifierContext& ctx, const TScenarioResponse& response) {
        for (const auto &semanticFrame : ctx.SemanticFrames()) {
            SemanticFramesProto.AddSemanticFrames()->CopyFrom(semanticFrame);
        }

        if (ctx.WizardResponse().GetProtoResponse().GetAliceIot().HasResult()) {
            IotResultProto = ctx.WizardResponse().GetProtoResponse().GetAliceIot().GetResult();
        }

        const auto &skr = ctx.SpeechKitRequest();
        Evaluator.SetProtoRef("AnalyticsInfo", response.ResponseBodyIfExists()->GetAnalyticsInfo());
        Evaluator.SetProtoRef("ClientInfo", skr->GetApplication());
        Evaluator.SetProtoRef("DeviceState", skr->GetRequest().GetDeviceState());
        Evaluator.SetProtoRef("IotResult", IotResultProto);
        Evaluator.SetProtoRef("ModifiersInfo", ctx.ModifiersInfo());
        Evaluator.SetProtoRef("NotificationState", skr->GetRequest().GetNotificationState());
        Evaluator.SetProtoRef("SemanticFrames", SemanticFramesProto);
        Evaluator.SetProtoRef("UserInfo", ctx.BlackBoxResponse());
    }
};

} // anonymous namespace

bool CheckCurrentScenarioForClickAction(TResponseModifierContext& ctx, const TScenarioResponse& response) {
    THashSet<TString> processedIds;
    auto& proactivityStorage = *ctx.ModifiersStorage().MutableProactivity();
    const TSpeechKitRequest& skr = ctx.SpeechKitRequest();
    const TVector<TSemanticFrame>& semanticFrames = ctx.SemanticFrames();
    const auto& actualDeviceState = skr->GetRequest().GetDeviceState();

    TMaybe<TEvaluatorCtx> evaluatorCtx;
    const auto getEvaluator = [&]() -> TProtoEvaluator& {
        return (evaluatorCtx.Defined() ? evaluatorCtx.GetRef() : evaluatorCtx.ConstructInPlace(ctx, response)).Evaluator;
    };

    bool shouldUpdateStorage = false;

    const auto isTrueSuccessCondition = [&](const NDJ::NAS::TSuccessCondition& successCondition) {
        return IsValidSuccessCondition(successCondition)
               && AnyFrameSatisfiesCondition(semanticFrames, successCondition.GetFrame())
               && DeviceStateSatisfiesCondition(actualDeviceState, successCondition.GetDeviceState())
               && (!successCondition.HasCheck() || getEvaluator().EvaluateBool(successCondition.GetCheck()));
    };

    const auto findTrueSuccessCondition = [&](const NDJ::NAS::TItemAnalytics& analytics) -> const NDJ::NAS::TSuccessCondition* {
        for (const auto& successCondition : analytics.GetSuccessConditions()) {
            if (isTrueSuccessCondition(successCondition)) {
                return &successCondition;
            }
        }
        return nullptr;
    };

    for (auto it = proactivityStorage.GetLastPostrollViews().begin(); it != proactivityStorage.GetLastPostrollViews().end(); ) {
        bool shouldErase = false;

        if (const auto* successCondition = findTrueSuccessCondition(it->GetAnalytics())) {
            if (!processedIds.contains(it->GetItemId())) {
                processedIds.insert(it->GetItemId());
                *ctx.ProactivityLogStorage().AddActions() = MakePostrollAction(skr, PostrollViewToProtoItem(*it),
                    NDJ::NAS::EAlisaSkillsActionType::AT_Click, it->GetSource(), it->GetRequestId(), &(it->GetContext()));
                ctx.Sensors().IncRate(NSignal::LabelsForPostrollSource(CLICK_ACTION_TYPE, it->GetSource()));
                ctx.Sensors().IncRate(NSignal::LabelsForPostrollItemInfo(CLICK_ACTION_TYPE, it->GetAnalytics().GetInfo()));
                *ctx.ModifiersInfo().MutableProactivity()->AddPostrollClickIds() = it->GetItemId();
                auto& postrollClick = *ctx.ModifiersInfo().MutableProactivity()->AddPostrollClicks();
                postrollClick.SetItemId(it->GetItemId());
                postrollClick.SetBaseId(it->GetBaseId());
                postrollClick.SetItemInfo(it->GetAnalytics().GetInfo());
                postrollClick.SetShowReqId(it->GetRequestId());
            }
            // TODO(jan-fazli): remove this kostil when hdmi postrolls should be gone from storage
            const bool isHdmiPostroll = it->GetTags().size() == 1 && it->GetTags()[0] == "hdmi";
            shouldErase = successCondition->GetIsTrigger() || isHdmiPostroll;
        }

        if (shouldErase) {
            it = proactivityStorage.MutableLastPostrollViews()->erase(it);
            shouldUpdateStorage = true;
        } else {
            ++it;
        }
    }

    return shouldUpdateStorage;
}

void CheckCurrentScenarioForDeclineAction(TResponseModifierContext& ctx, const TScenarioResponse& response) {
    auto& proactivityStorage = *ctx.ModifiersStorage().MutableProactivity();
    if (ctx.SpeechKitRequest().PrevReqId() != proactivityStorage.GetLastPostrollRequestId()) {
        return;
    }

    const bool isStopIntent = STOP_INTENTS.contains(GetProactivitySource(ctx.Session(), response));
    const bool isDeclineActionIntent = response.GetScenarioName() == HOLLYWOOD_DO_NOTHING_SCENARIO;
    if (!isStopIntent && !isDeclineActionIntent) {
        return;
    }

    const auto lastPostrollViews = proactivityStorage.GetLastPostrollViews();
    if (lastPostrollViews.empty()) {
        return;
    }

    const auto lastPostroll = lastPostrollViews[lastPostrollViews.size() - 1];
    const auto actionType = isStopIntent
        ? NDJ::NAS::EAlisaSkillsActionType::AT_Stop
        : NDJ::NAS::EAlisaSkillsActionType::AT_DeclineButton;

    *ctx.ProactivityLogStorage().AddActions() = MakePostrollAction(
        ctx.SpeechKitRequest(),
        PostrollViewToProtoItem(lastPostroll),
        actionType,
        lastPostroll.GetSource(),
        proactivityStorage.GetLastPostrollRequestId(),
        &lastPostroll.GetContext()
    );
}

} // NAlice::NMegamind
