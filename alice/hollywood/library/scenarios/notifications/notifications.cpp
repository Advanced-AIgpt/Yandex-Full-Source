#include "notifications.h"

#include <alice/hollywood/library/scenarios/notifications/proto/notifications.pb.h>

#include <alice/hollywood/library/environment_state/endpoint.h>
#include <alice/hollywood/library/frame/callback.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/registry/registry.h>

#include <alice/megamind/protos/common/app_type.pb.h>
#include <alice/megamind/protos/scenarios/notification_state.pb.h>

#include <alice/library/json/json.h>
#include <alice/library/logger/logger.h>
#include <alice/library/proto/protobuf.h>

#include <alice/protos/endpoint/capability.pb.h>
#include <alice/protos/endpoint/endpoint.pb.h>

#include <util/stream/file.h>

namespace NAlice::NHollywood {

namespace {

const TString NLG_TEMPLATE = "notifications";
const TString NLG_RENDER_NOTIFICATIONS = "render_notifications";
const TString NLG_RENDER_NO_NOTIFICATIONS = "render_no_notifications";
const TString NLG_RENDER_UNAVAILABLE = "unavailable";
const TString NLG_RENDER_IRRELEVANT = "irrelevant";
const TString NLG_RENDER_ONBOARDING = "render_onboarding";
const TString NLG_RENDER_ONBOARDING_REFUSE = "render_onboarding_refuse";
const TString NLG_RENDER_SUBSCRIBE = "render_subscribe";
const TString NLG_RENDER_SUBSCRIBE_ACCEPT = "render_subscribe_accept";
const TString NLG_RENDER_SUBSCRIBE_REFUSE = "render_subscribe_refuse";
const TString NLG_RENDER_SUBSCRIBE_TWICE = "render_subscribe_twice";
const TString NLG_RENDER_UNSUBSCRIBE = "render_unsubscribe";
const TString NLG_RENDER_UNSUBSCRIBE_EMPTY = "render_unsubscribe_empty";
const TString NLG_RENDER_UNSUBSCRIBE_ACCEPT = "render_unsubscribe_accept";
const TString NLG_RENDER_UNSUBSCRIBE_REFUSE = "render_unsubscribe_refuse";
const TString NLG_RENDER_UNSUBSCRIBE_INSTRUCTION = "render_unsubscribe_instruction";
const TString NLG_RENDER_NO_SUBSCRIPTION = "render_no_subscription";
const TString NLG_RENDER_DROP_ALL = "render_drop_all";

const TString ONBOARDING_FRAME = "alice.notifications_onboarding";
const TString SUBSCRIBE_FRAME = "alice.notifications_subscribe";
const TString UNSUBSCRIBE_FRAME = "alice.notifications_unsubscribe";
const TString READ_FRAME = "alice.notifications_read";
const TString DO_NOTHING_FRAME = "alice.do_nothing";
const TString DROP_ALL_CONFIRM_FRAME = "alice.notifications_drop_all_confirm";
const TString DROP_ALL_FRAME = "alice.notifications_drop_all";
const TString SUBSCRIPTIONS_LIST_FRAME = "alice.notifications_subscriptions_list";
const TString PROACTIVITY_CONFIRM_FRAME = "alice.proactivity.confirm";
const TString PROACTIVITY_DECLINE_FRAME = "alice.proactivity.decline";

const TString ONBOARDING_SLOT = "add_onboarding";
const TString WIDE_REQUEST_SLOT = "wide_request";
const TString REFUSE_SLOT = "refuse";
const TString ACCEPT_SLOT = "accept";
const TString SUBSCRIPTION_SLOT = "notification_subscription";

const TString NOTIFICATIONS_FIELD = "notifications";
const TString NOTIFICATIONS_COUNT_FIELD = "notifications_count";
const TString SUBSCRIPTION_FIELD = "subscription";
const TString HAS_PREVIOUS_NOTIFICATIONS = "previous_notifications";

constexpr TStringBuf NOTIFICATIONS_GIF_URI_PREFIX = "https://static-alice.s3.yandex.net/led-production/notifications/read_notifications/v0_";
constexpr size_t MAX_NOTITIFCATIONS_LED = 9;

static const TVector<TStringBuf> REFUSE_PHRASES = {
    "нет",
    "не надо",
    "не хочу",
    "обойдусь",
};

// Больше не хотите получать от меня уведомления?
static const TVector<TStringBuf> UNSUBSCRIBE_AGREE_PHRASES = {
    "да",
    "хочу",
    "ага",
};

const TString DEFAULT_SUBSCRIPTION_DATA = R"(
    {
        "id": "1",
        "name": "регулярный дайджест Алисы",
        "voice": "Хотите, я буду сообщать вам о том, чему я научилась или о том, что я стала лучше и полезнее?"
    }
)";

void AddRenderedPhrase(TScenarioHandleContext& ctx, const TString& intent, const TString& phrase, const TNlgData& nlgData, TRunResponseBuilder& builder) {
    LOG_INFO(ctx.Ctx.Logger()) << "Rendering phrase " << phrase;
    auto& bodyBuilder = *builder.GetResponseBodyBuilder();
    bodyBuilder.GetAnalyticsInfoBuilder().SetIntentName(intent);
    bodyBuilder.AddRenderedTextWithButtonsAndVoice(NLG_TEMPLATE, phrase, /* buttons */{}, nlgData);
    ctx.ServiceCtx.AddProtobufItem(*std::move(builder).BuildResponse(), RESPONSE_ITEM);
}

void MakeStringSlot(const TString& slotName, TSemanticFrame_TSlot& slot) {
    slot.SetName(slotName);
    slot.SetType("string");
    slot.SetValue("true");
}

TSemanticFrame CreateFrameWithSlot(const TString& frameName, const TString& slotName, const TSemanticFrame_TSlot* subscription) {
    TSemanticFrame frame;
    frame.SetName(frameName);
    MakeStringSlot(slotName, *frame.AddSlots());
    if (subscription) {
        *frame.AddSlots() = *subscription;
    } else {
        TSemanticFrame_TSlot& subscriptionSlot = *frame.AddSlots();
        subscriptionSlot.SetName(SUBSCRIPTION_SLOT);
        subscriptionSlot.SetType("custom.notification_subscription");
        subscriptionSlot.SetValue(DEFAULT_SUBSCRIPTION_DATA);
    }
    return frame;
}

void AddParsedUtteranceAction(const TString& nluFrameName, const NAlice::TNotification& notification, TResponseBodyBuilder& bodyBuilder) {
    NScenarios::TFrameAction action;
    action.MutableNluHint()->SetFrameName(nluFrameName);
    if (notification.HasParsedUtterance()) {
        *action.MutableParsedUtterance() = std::move(notification.GetParsedUtterance());
    } else {
        *action.MutableParsedUtterance()->MutableFrame() = notification.GetFrame();
        action.MutableParsedUtterance()->SetUtterance(notification.GetFrameUtterance());
    }
    bodyBuilder.AddAction(nluFrameName, std::move(action));
}

void AddFrameAction(const TString& nluFrameName, TSemanticFrame&& frame, TResponseBodyBuilder& bodyBuilder) {
    NScenarios::TFrameAction action;
    action.MutableNluHint()->SetFrameName(nluFrameName);
    *action.MutableCallback() = ToCallback(frame);
    bodyBuilder.AddAction(nluFrameName, std::move(action));
}

void AddFrameAction(const TString& nluFrameName, const TString& frameName, TResponseBodyBuilder& bodyBuilder) {
    TSemanticFrame semanticFrame;
    semanticFrame.SetName(frameName);
    return AddFrameAction(nluFrameName, std::move(semanticFrame), bodyBuilder);
}

void AddDeclineButton(TResponseBodyBuilder& bodyBuilder) {
    TFrameNluHint declineNluHint;
    declineNluHint.SetFrameName(PROACTIVITY_DECLINE_FRAME);

    TSemanticFrame declineFrame;
    declineFrame.SetName(DO_NOTHING_FRAME);

    NScenarios::TFrameAction actionDecline;
    *actionDecline.MutableNluHint() = std::move(declineNluHint);
    *actionDecline.MutableFrame() = std::move(declineFrame);
    bodyBuilder.AddAction(PROACTIVITY_DECLINE_FRAME, std::move(actionDecline));
}

TMaybe<TAnimationCapability> ParseAnimationCapability(const TScenarioRunRequestWrapper& request) {
    const auto* envState = GetEnvironmentStateProto(request);
    if (!envState) {
        return {};
    }
    
    const auto* endpoint = NHollywood::FindEndpoint(*envState, request.ClientInfo().DeviceId);
    if (!endpoint) {
        return {};
    }
    
    TAnimationCapability capability;
    if (NHollywood::ParseTypedCapability(capability, *endpoint)) {
        return capability;
    }

    return {};
}

bool HasNotificationAnimation(const TAnimationCapability& capability) {
    const auto& state = capability.GetState();
    for (const auto& [key, screenState] : state.GetScreenStatesMap()) {
        if (screenState.GetAnimation().GetAnimationType() == TAnimationCapability::TAnimation::Notification) {
            return true;
        }
    }

    return false;
}

void HandleIrrelevant(TScenarioHandleContext& ctx, const TNlgData& nlgData, TRunResponseBuilder& builder) {
    builder.SetIrrelevant();
    return AddRenderedPhrase(ctx, "alice.notifications_irrelevant", NLG_RENDER_IRRELEVANT, nlgData, builder);
}

void HandleUnavailable(TScenarioHandleContext& ctx, const TNlgData& nlgData, TRunResponseBuilder& builder) {
    builder.SetIrrelevant();
    return AddRenderedPhrase(ctx, "alice.notifications_unavailable", NLG_RENDER_UNAVAILABLE, nlgData, builder);
}

void HandleUnsubscribeEmpty(TScenarioHandleContext& ctx, const TSemanticFrame& frame, const TNlgData& nlgData, TRunResponseBuilder& builder) {
    auto& bodyBuilder = *builder.GetResponseBodyBuilder();

    AddFrameAction(PROACTIVITY_CONFIRM_FRAME, CreateFrameWithSlot(SUBSCRIBE_FRAME, ACCEPT_SLOT, nullptr), bodyBuilder);
    AddFrameAction(PROACTIVITY_DECLINE_FRAME, CreateFrameWithSlot(SUBSCRIBE_FRAME, REFUSE_SLOT, nullptr), bodyBuilder);

    return AddRenderedPhrase(ctx, frame.GetName(), NLG_RENDER_UNSUBSCRIBE_EMPTY, nlgData, builder);
}

NScenarios::TDirective CreateSubscriptionDirective(bool unsubscribe, int id) {
    NScenarios::TDirective directive;
    NScenarios::TUpdateNotificationSubscriptionDirective* subscribeDirective = directive.MutableUpdateNotificationSubscriptionDirective();
    subscribeDirective->SetSubscriptionId(id);
    subscribeDirective->SetUnsubscribe(unsubscribe);
    return directive;
}

struct TSubscriptionRequestData {
    bool Accepted = false;
    const TSemanticFrame_TSlot* SubscriptionSlot = nullptr;
    TString SubscriptionId;
};

TMaybe<TSubscriptionRequestData> HandleSubscriptionsFrame(TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request,
                                                          const TSemanticFrame& frame, TNlgData& nlgData,
                                                          TRunResponseBuilder& builder, const TString& refuseAnalyticsName,
                                                          const TString& refuseAnalyticsDescription, const TString& refusePhrase)
{
    TSubscriptionRequestData requestData;
    for (const auto& slot : frame.GetSlots()) {
        if (slot.GetName() == REFUSE_SLOT) {
            auto& bodyBuilder = *builder.GetResponseBodyBuilder();
            bodyBuilder.GetAnalyticsInfoBuilder().AddObject(refuseAnalyticsName, refuseAnalyticsName, refuseAnalyticsDescription);
            AddRenderedPhrase(ctx, frame.GetName(), refusePhrase, nlgData, builder);
            return Nothing();
        }

        if (slot.GetName() == ACCEPT_SLOT) {
            requestData.Accepted = true;
        }
        if (slot.GetName() == SUBSCRIPTION_SLOT) {
            requestData.SubscriptionSlot = &slot;
        }
    }
    NJson::TJsonValue subscription = JsonFromString(requestData.SubscriptionSlot ? requestData.SubscriptionSlot->GetValue() : DEFAULT_SUBSCRIPTION_DATA);
    nlgData.Context[SUBSCRIPTION_FIELD] = subscription;
    requestData.SubscriptionId = subscription["id"].GetString();
    if (const auto& expFlag = subscription["experiment"].GetString()) {
        if (!request.HasExpFlag(expFlag)) {
            HandleIrrelevant(ctx, nlgData, builder);
            return Nothing();
        }
    }
    return requestData;
}

void HandleUnsubscribe(TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request, const TSemanticFrame& frame,
                       TNlgData& nlgData, TRunResponseBuilder& builder,const NScenarios::TDataSource& state)
{
    auto& bodyBuilder = *builder.GetResponseBodyBuilder();
    const auto requestDataMaybe = HandleSubscriptionsFrame(ctx, request, frame, nlgData, builder, "notification_unsubscription_refuse",
                                                          "User refused to unsubscribe update notifications",
                                                           NLG_RENDER_UNSUBSCRIBE_REFUSE);
    if (!requestDataMaybe.Defined()) {
        return;
    }
    const auto& requestData = *requestDataMaybe;
    if (!requestData.SubscriptionSlot) {
        bodyBuilder.GetAnalyticsInfoBuilder().AddObject("notification_unsubscribe_no_slot", "notification_unsubscribe_no_slot",
                                                        "Пользователь захотел отписаться без указания подписки");
        return AddRenderedPhrase(ctx, frame.GetName(), NLG_RENDER_UNSUBSCRIBE_INSTRUCTION, nlgData, builder);
    }

    const auto& subscriptions = state.GetNotificationState().GetSubscriptions();
    if (subscriptions.empty()) {
        return HandleUnsubscribeEmpty(ctx, frame, nlgData, builder);
    }

    bool hasSubscription = false;
    for (const auto& stateSubscription : subscriptions) {
        if (stateSubscription.GetId() == requestData.SubscriptionId) {
            hasSubscription = true;
        }
    }
    if (!hasSubscription) {
        return AddRenderedPhrase(ctx, frame.GetName(), NLG_RENDER_NO_SUBSCRIPTION, nlgData, builder);
    }

    if (requestData.Accepted) {
        int id;
        if (!TryFromString(requestData.SubscriptionId, id)) {
            return HandleIrrelevant(ctx, nlgData, builder);
        }
        bodyBuilder.AddDirective(CreateSubscriptionDirective(/* unsubscribe */ true, id));
        bodyBuilder.GetAnalyticsInfoBuilder().AddObject("notification_unsubscription_accept", "notification_unsubscribe_accept", "User accepted to unsubscribe update notifications");
        return AddRenderedPhrase(ctx, frame.GetName(), NLG_RENDER_UNSUBSCRIBE_ACCEPT, nlgData, builder);
    }

    AddFrameAction(PROACTIVITY_CONFIRM_FRAME, CreateFrameWithSlot(UNSUBSCRIBE_FRAME, ACCEPT_SLOT, requestData.SubscriptionSlot), bodyBuilder);
    AddFrameAction(PROACTIVITY_DECLINE_FRAME, CreateFrameWithSlot(UNSUBSCRIBE_FRAME, REFUSE_SLOT, requestData.SubscriptionSlot), bodyBuilder);

    return AddRenderedPhrase(ctx, frame.GetName(), NLG_RENDER_UNSUBSCRIBE, nlgData, builder);
}

void HandleSubscribe(TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request, const TSemanticFrame& frame,
                     TNlgData& nlgData, TRunResponseBuilder& builder, const NScenarios::TDataSource& state)
{
    auto& bodyBuilder = *builder.GetResponseBodyBuilder();
    const auto requestDataMaybe = HandleSubscriptionsFrame(ctx, request, frame, nlgData, builder, "notification_subscription_refuse",
                                                           "User refused to subscribe update notifications",
                                                           NLG_RENDER_SUBSCRIBE_REFUSE);
    if (!requestDataMaybe.Defined()) {
        return;
    }
    const auto& requestData = *requestDataMaybe;

    const auto& subscriptions = state.GetNotificationState().GetSubscriptions();
    for (const auto& stateSubscription : subscriptions) {
        if (stateSubscription.GetId() == requestData.SubscriptionId) {
            return AddRenderedPhrase(ctx, frame.GetName(), NLG_RENDER_SUBSCRIBE_TWICE, nlgData, builder);
        }
    }
    if (subscriptions.empty()) {
        nlgData.Context["first_subscription"] = true;
    }

    if (requestData.Accepted) {
        int id;
        if (!TryFromString(requestData.SubscriptionId, id)) {
            return HandleIrrelevant(ctx, nlgData, builder);
        }
        bodyBuilder.AddDirective(CreateSubscriptionDirective(/* unsubscribe */ false, id));
        bodyBuilder.GetAnalyticsInfoBuilder().AddObject("notification_subscription_accept", "notification_subscribe_accept", "User accepted to subscribe update notifications");
        return AddRenderedPhrase(ctx, frame.GetName(), NLG_RENDER_SUBSCRIBE_ACCEPT, nlgData, builder);
    }

    AddFrameAction(PROACTIVITY_CONFIRM_FRAME, CreateFrameWithSlot(SUBSCRIBE_FRAME, ACCEPT_SLOT, requestData.SubscriptionSlot), bodyBuilder);
    AddFrameAction(PROACTIVITY_DECLINE_FRAME, CreateFrameWithSlot(SUBSCRIBE_FRAME, REFUSE_SLOT, requestData.SubscriptionSlot), bodyBuilder);

    return AddRenderedPhrase(ctx, frame.GetName(), NLG_RENDER_SUBSCRIBE, nlgData, builder);
}

void HandleOnboarding(TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request, const TSemanticFrame& frame,
                      TNlgData& nlgData, TRunResponseBuilder& builder, const NScenarios::TDataSource& state)
{
    auto& bodyBuilder = *builder.GetResponseBodyBuilder();

    for (const auto& slot : frame.GetSlots()) {
        if (slot.GetName() == WIDE_REQUEST_SLOT) {
            if (state.GetNotificationState().GetNotifications().empty()) {
                return HandleIrrelevant(ctx, nlgData, builder);
            }
        }

        if (slot.GetName() == REFUSE_SLOT) {
            bodyBuilder.GetAnalyticsInfoBuilder().AddObject("notification_read_refuse", "notification_read_refuse", "User refused to listen notifications");
            return AddRenderedPhrase(ctx, frame.GetName(), NLG_RENDER_ONBOARDING_REFUSE, nlgData, builder);
        }
    }

    TSemanticFrame readFrame;
    readFrame.SetName(READ_FRAME);
    AddFrameAction(PROACTIVITY_CONFIRM_FRAME, std::move(readFrame), bodyBuilder);
    AddFrameAction(PROACTIVITY_DECLINE_FRAME, CreateFrameWithSlot(ONBOARDING_FRAME, REFUSE_SLOT, nullptr), bodyBuilder);

    const auto animationCapability = ParseAnimationCapability(request);
    if (animationCapability) {
        auto* playerFeatures = builder.GetMutableFeatures().MutablePlayerFeatures();
        playerFeatures->SetRestorePlayer(true);
        playerFeatures->SetSecondsSincePause(HasNotificationAnimation(animationCapability.GetRef()) ? 0 : 1);
    }

    return AddRenderedPhrase(ctx, frame.GetName(), NLG_RENDER_ONBOARDING, nlgData, builder);
}

void FillNlgData(TScenarioHandleContext& ctx, TNlgData& nlgData, NJson::TJsonValue notification, const ui64 notificationsCount) {
    if (!notification.Has("voice")) {
        notification["voice"] = notification["text"];
    }
    nlgData.Context[NOTIFICATIONS_FIELD].AppendValue(notification);
    nlgData.Context[NOTIFICATIONS_COUNT_FIELD] = notificationsCount;
    LOG_INFO(ctx.Ctx.Logger()) << "Render notification: " << notification;
}

void RenderNotification(TScenarioHandleContext& ctx, TNlgData& nlgData, TResponseBodyBuilder& bodyBuilder,
                        const NAlice::TNotification& notification, const ui64 notificationsCount) {
    FillNlgData(ctx, nlgData, JsonFromProto(notification), notificationsCount);

    bodyBuilder.GetAnalyticsInfoBuilder().AddObject(notification.GetId(), "smartspeaker_notification", notification.GetText());
    if (notification.HasFrame() || notification.HasParsedUtterance()) {
        AddParsedUtteranceAction(PROACTIVITY_CONFIRM_FRAME, notification, bodyBuilder);

        TNotificationsSession session;
        session.SetHasPreviousNotifications(true);
        bodyBuilder.SetState(session);

        TSemanticFrame readNotificationsFrame;
        readNotificationsFrame.SetName(READ_FRAME);
        AddFrameAction(PROACTIVITY_DECLINE_FRAME, std::move(readNotificationsFrame), bodyBuilder);
    }
}

void AddLedScreenDirective(TScenarioHandleContext& ctx, TResponseBodyBuilder& bodyBuilder, size_t notificationsCount) {
    if (notificationsCount > MAX_NOTITIFCATIONS_LED) {
        LOG_ERROR(ctx.Ctx.Logger()) << "Don't have gif with number: " << notificationsCount;
        notificationsCount = MAX_NOTITIFCATIONS_LED;
    }
    const auto imageUri = TString::Join(NOTIFICATIONS_GIF_URI_PREFIX, ToString(notificationsCount), ".gif");
    NJson::TJsonValue data;
    data["payload"][0]["frontal_led_image"] = imageUri;
    bodyBuilder.AddClientActionDirective("draw_led_screen", data);
    bodyBuilder.AddClientActionDirective("force_display_cards", {});
}

bool AddNotificationFromState(TScenarioHandleContext& ctx, TNlgData& nlgData, TResponseBodyBuilder& bodyBuilder,
                              const NScenarios::TDataSource& state, const NScenarios::TInterfaces& interfaces)
{
    const auto& notifications = state.GetNotificationState().GetNotifications();
    LOG_DEBUG(ctx.Ctx.Logger()) << "Notification state: " << JsonFromProto(state.GetNotificationState());

    if (notifications.empty()) {
        return false;
    }

    NScenarios::TDirective directive;
    NScenarios::TMarkNotificationAsReadDirective* markReadDirective = directive.MutableMarkNotificationAsReadDirective();
    for (const auto& notification : notifications) {
        RenderNotification(ctx, nlgData, bodyBuilder, notification, notifications.size());
        markReadDirective->AddNotificationIds(notification.GetId());

        if (notification.HasFrame() || notification.HasParsedUtterance()) {
            break;
        }
    }
    bodyBuilder.AddDirective(std::move(directive));
    if (interfaces.GetHasLedDisplay()) {
        LOG_DEBUG(ctx.Ctx.Logger()) << "Add led directive to response";
        AddLedScreenDirective(ctx, bodyBuilder, notifications.size());
    }
    return true;
}

void HandleDropAll(TScenarioHandleContext& ctx, const TSemanticFrame& frame, TNlgData& nlgData, TRunResponseBuilder& builder,
                    const NScenarios::TDataSource& state)
{
    const auto& notifications = state.GetNotificationState().GetNotifications();
    if (notifications.empty()) {
        for (const auto& slot : frame.GetSlots()) {
            if (slot.GetName() == WIDE_REQUEST_SLOT) {
                return HandleIrrelevant(ctx, nlgData, builder);
            }
        }

        nlgData.Context["no_notifications"] = true;
        builder.GetResponseBodyBuilder()->SetShouldListen(false);
        return AddRenderedPhrase(ctx, frame.GetName(), NLG_RENDER_DROP_ALL, nlgData, builder);
    }

    auto& bodyBuilder = *builder.GetResponseBodyBuilder();

    AddFrameAction(PROACTIVITY_CONFIRM_FRAME, DROP_ALL_CONFIRM_FRAME, bodyBuilder);
    AddDeclineButton(bodyBuilder);

    nlgData.Context["onboarding"] = true;
    return AddRenderedPhrase(ctx, frame.GetName(), NLG_RENDER_DROP_ALL, nlgData, builder);
}

void HandleDropAllConfirmed(TScenarioHandleContext& ctx, const TSemanticFrame& frame, TNlgData& nlgData, TRunResponseBuilder& builder,
                    const NScenarios::TDataSource& state)
{
    const auto& notifications = state.GetNotificationState().GetNotifications();
    auto& bodyBuilder = *builder.GetResponseBodyBuilder();

    NScenarios::TDirective directive;
    NScenarios::TMarkNotificationAsReadDirective* markReadDirective = directive.MutableMarkNotificationAsReadDirective();
    for (const auto& notification : notifications) {
        markReadDirective->AddNotificationIds(notification.GetId());
    }
    
    bodyBuilder.AddDirective(std::move(directive));

    builder.GetResponseBodyBuilder()->SetShouldListen(false);
    return AddRenderedPhrase(ctx, frame.GetName(), NLG_RENDER_DROP_ALL, nlgData, builder);
}

void HandleRead(TScenarioHandleContext& ctx, const TSemanticFrame& frame, TNlgData& nlgData, TRunResponseBuilder& builder,
                const NScenarios::TDataSource& state, const NScenarios::TScenarioBaseRequest& request) {
    for (const auto& slot : frame.GetSlots()) {
        if (slot.GetName() == ONBOARDING_SLOT) {
            nlgData.Context[ONBOARDING_SLOT] = NJson::TJsonValue(true);
        }
    }

    const auto& rawState = request.GetState();
    if (rawState.Is<TNotificationsSession>() && !request.GetIsNewSession()) {
        TNotificationsSession session;
        rawState.UnpackTo(&session);
        nlgData.Context[HAS_PREVIOUS_NOTIFICATIONS] = session.GetHasPreviousNotifications();
    }

    const bool hasNotification = AddNotificationFromState(ctx, nlgData, *builder.GetResponseBodyBuilder(),
                                                          state, request.GetInterfaces());
    const auto phrase = hasNotification ? NLG_RENDER_NOTIFICATIONS : NLG_RENDER_NO_NOTIFICATIONS;
    if (!hasNotification) {
        builder.GetResponseBodyBuilder()->SetShouldListen(false);
    }
    return AddRenderedPhrase(ctx, frame.GetName(), phrase, nlgData, builder);
}

void AddPushMessageDirective(TResponseBodyBuilder& bodyBuilder) {
    NScenarios::TServerDirective directive;
    NScenarios::TPushMessageDirective& push = *directive.MutablePushMessageDirective();
    push.SetTitle("Настройка уведомлений");
    push.SetBody("Управление подписками");
    push.SetLink("ya-search-app-open://?uri=yellowskin://?url=https%3A%2F%2Fyandex.ru%2Fquasar%2Faccount%2Falice-subscriptions%3Fsource%3Dpush");
    push.SetPushId("alice.notifications");
    push.SetPushTag("alice.notifications");
    push.SetThrottlePolicy("eddl-unlimitted");
    push.AddAppTypes(NAlice::EAppType::AT_SEARCH_APP);
    bodyBuilder.AddServerDirective(std::move(directive));
}

void HandleSubscriptionsList(TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request, const TSemanticFrame& frame,
                             TNlgData& nlgData, TRunResponseBuilder& builder, const NScenarios::TDataSource& state) {
    auto subscriptions = state.GetNotificationState().GetSubscriptions();
    LOG_DEBUG(ctx.Ctx.Logger()) << "Rendering subscriptions list: " << JsonFromProto(state.GetNotificationState());

    if (subscriptions.empty()) {
        nlgData.Context["subscriptions_empty"] = true;
    }

    SortBy(subscriptions.rbegin(), subscriptions.rend(), [](const auto& subscription) {return subscription.GetTimestamp();});

    for (int pos = 0; pos < subscriptions.size(); ++pos) {
        if (pos == 3) {
            nlgData.Context["extra_subscriptions_count"] = subscriptions.size() - 3;
            break;
        }
        nlgData.Context["subscriptions"].AppendValue(JsonFromProto(subscriptions[pos]));
    }

    if (request.HasExpFlag("send_push_for_subscription_list")) {
        LOG_INFO(ctx.Ctx.Logger()) << "Add PushMessageDirective";
        AddPushMessageDirective(*builder.GetResponseBodyBuilder());
        nlgData.Context["send_push"] = true;
    }
    return AddRenderedPhrase(ctx, frame.GetName(), "render_subscriptions_list", nlgData, builder);
}

} // namespace

void TNotificationsRunHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder builder(&nlgWrapper);
    auto& bodyBuilder = builder.CreateResponseBodyBuilder();
    bodyBuilder.CreateAnalyticsInfoBuilder();
    bodyBuilder.GetAnalyticsInfoBuilder().SetProductScenarioName("smartspeaker_notifications");
    bodyBuilder.SetShouldListen(true);

    TNlgData nlgData{ctx.Ctx.Logger(), request};

    const auto* notificationState = request.GetDataSource(EDataSourceType::NOTIFICATION_STATE);
    if (!notificationState || !request.ClientInfo().IsSmartSpeaker() && !requestProto.GetBaseRequest().GetInterfaces().GetHasNotifications()) {
        return HandleUnavailable(ctx, nlgData, builder);
    }

    const auto& input = request.Input();
    const TMaybe<TFrame> callbackFrame = GetCallbackFrame(input.GetCallback());
    if (const TMaybe<TFrame> frame = TryGetFrame(UNSUBSCRIBE_FRAME, callbackFrame, input); frame.Defined()) {
       return HandleUnsubscribe(ctx, request, frame->ToProto(), nlgData, builder, *notificationState);
    }

    if (const TMaybe<TFrame> frame = TryGetFrame(SUBSCRIBE_FRAME, callbackFrame, input); frame.Defined()) {
       return HandleSubscribe(ctx, request, frame->ToProto(), nlgData, builder, *notificationState);
    }

    if (const TMaybe<TFrame> frame = TryGetFrame(ONBOARDING_FRAME, callbackFrame, input); frame.Defined()) {
       return HandleOnboarding(ctx, request, frame->ToProto(), nlgData, builder, *notificationState);
    }

    if (const TMaybe<TFrame> frame = TryGetFrame(DROP_ALL_FRAME, callbackFrame, input); frame.Defined() && request.HasExpFlag(EXP_HW_ENABLE_NOTIFICATIONS_DROP_ALL)) {
       return HandleDropAll(ctx, frame->ToProto(), nlgData, builder, *notificationState);
    }

    if (const TMaybe<TFrame> frame = TryGetFrame(DROP_ALL_CONFIRM_FRAME, callbackFrame, input); frame.Defined() && request.HasExpFlag(EXP_HW_ENABLE_NOTIFICATIONS_DROP_ALL)) {
       return HandleDropAllConfirmed(ctx, frame->ToProto(), nlgData, builder, *notificationState);
    }

    if (const TMaybe<TFrame> frame = TryGetFrame(READ_FRAME, callbackFrame, input); frame.Defined()) {
       return HandleRead(ctx, frame->ToProto(), nlgData, builder, *notificationState, requestProto.GetBaseRequest());
    }

    if (const TMaybe<TFrame> frame = TryGetFrame(SUBSCRIPTIONS_LIST_FRAME, callbackFrame, input); frame.Defined()) {
       return HandleSubscriptionsList(ctx, request, frame->ToProto(), nlgData, builder, *notificationState);
    }

    return HandleIrrelevant(ctx, nlgData, builder);
}

REGISTER_SCENARIO("notifications",
                  AddHandle<TNotificationsRunHandle>()
                  .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NNotifications::NNlg::RegisterAll));

REGISTER_SCENARIO("notifications_manager",
                  AddHandle<TNotificationsRunHandle>()
                  .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NNotifications::NNlg::RegisterAll));

} // namespace NAlice::NHollywood
