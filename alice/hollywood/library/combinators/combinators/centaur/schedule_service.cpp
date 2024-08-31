#include "schedule_service.h"

#include <alice/megamind/protos/common/atm.pb.h>
#include <alice/protos/api/matrix/delivery.pb.h>
#include <alice/protos/api/matrix/schedule_action.pb.h>

namespace NAlice::NHollywood::NCombinators {

using namespace NAlice::NScenarios;

TMaybe<TServerDirective> CreateUpdateCarouselScheduleAction(
    const TClientInfoProto& clientInfo,
    const TMaybe<TBlackBoxFullUserInfoProto>& blackBoxUserInfo,
    TRTLogger& logger)
{
    if (!blackBoxUserInfo.Defined()) {
        LOG_ERROR(logger) << "Creating update carousel schedule action failed: no blackbox user info"; 
        return Nothing();
    }
    const auto& userPuid = blackBoxUserInfo->GetUserInfo().GetUid();

    TSemanticFrameRequestData semanticFrame;
    auto& collectCardSemanticFrame = *semanticFrame.MutableTypedSemanticFrame()->MutableCentaurCollectCardsSemanticFrame();
    collectCardSemanticFrame.MutableIsScheduledUpdate()->SetBoolValue(true);

    auto& analytics = *semanticFrame.MutableAnalytics();
    analytics.SetProductScenario(CENTAUR_TEASERS_PRODUCT_SCENARIO);
    analytics.SetPurpose(COLLECT_CAROUSEL_PURPOSE);
    analytics.SetOrigin(TAnalyticsTrackingModule_EOrigin_Scenario);

    return CreateUpdateScheduleAction(clientInfo, userPuid, CAROUSEL_UPDATE_ACTION_NAME, semanticFrame, CAROUSEL_REFRESH_PERIOD_MINUTES);
}

TMaybe<TServerDirective> CreateUpdateMainScreenScheduleAction(
    const TClientInfoProto& clientInfo,
    const TString& userPuid)
{
    TSemanticFrameRequestData semanticFrame;
    auto& collectMainScreenSemanticFrame = *semanticFrame.MutableTypedSemanticFrame()->MutableCentaurCollectMainScreenSemanticFrame();

    auto& analytics = *semanticFrame.MutableAnalytics();
    analytics.SetProductScenario(CENTAUR_MAIN_SCREEN_PRODUCT_SCENARIO);
    analytics.SetPurpose(COLLECT_MAIN_SCREEN_PURPOSE);
    analytics.SetOrigin(TAnalyticsTrackingModule_EOrigin_Scenario);

    return CreateUpdateScheduleAction(clientInfo, userPuid, MAIN_SCREEN_UPDATE_ACTION_NAME, semanticFrame, MAIN_SCREEN_REFRESH_PERIOD_MINUTES);
}

TMaybe<TServerDirective> CreateUpdateScheduleAction(
    const TClientInfoProto& clientInfo,
    const TString& userPuid,
    const TString actionName,
    const TSemanticFrameRequestData& request,
    int refreshPeriodMinutes)
{
    TString scheduleActionId = TStringBuilder{} << actionName << "_" << userPuid << "_" << clientInfo.GetDeviceId();

    TServerDirective serverDirective;
    auto& scheduleAction = *serverDirective.MutableAddScheduleActionDirective()->MutableScheduleAction();
    scheduleAction.SetPuid(userPuid);
    scheduleAction.SetDeviceId(clientInfo.GetDeviceId());
    scheduleAction.SetId(scheduleActionId);
    scheduleAction.SetOverride(true);

    auto& sendPolicy = *scheduleAction.MutableSendPolicy()->MutableSendPeriodicallyPolicy();
    sendPolicy.SetPeriodMs(refreshPeriodMinutes * 60 * 1000);
    auto& retryPolicy = *sendPolicy.MutableRetryPolicy();
    retryPolicy.SetDoNotRetryIfDeviceOffline(true);
    retryPolicy.SetMaxRetries(REFRESH_MAX_RETRIES);
    retryPolicy.SetRestartPeriodScaleMs(refreshPeriodMinutes* 60 * 1000);
    retryPolicy.SetMaxRestartPeriodMs(REFRESH_MAX_RESTART_PERIOD_DAYS * 24 * 60 * 60 * 1000);
    retryPolicy.SetRestartPeriodBackOff(REFRESH_RESTART_PERIOD_BACKOFF);

    auto& delivery = *scheduleAction.MutableAction()->MutableOldNotificatorRequest()->MutableDelivery();
    delivery.SetPuid(userPuid);
    delivery.SetDeviceId(clientInfo.GetDeviceId());
    *delivery.MutableSemanticFrameRequestData() = std::move(request);

    return serverDirective;
}

} // NAlice::NHollywood::NCombinators
