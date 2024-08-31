#pragma once

#include "defs.h"

#include <alice/library/blackbox/blackbox.h>
#include <alice/library/client/protos/client_info.pb.h>
#include <alice/library/logger/logger.h>
#include <alice/megamind/protos/common/frame.pb.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>

namespace NAlice::NHollywood::NCombinators {

TMaybe<NScenarios::TServerDirective> CreateUpdateCarouselScheduleAction(
    const TClientInfoProto& clientInfo,
    const TMaybe<TBlackBoxFullUserInfoProto>& blackBoxUserInfo,
    TRTLogger& logger);

TMaybe<NScenarios::TServerDirective> CreateUpdateMainScreenScheduleAction(
    const TClientInfoProto& clientInfo,
    const TString& userPuid);

TMaybe<NScenarios::TServerDirective> CreateUpdateScheduleAction(
    const TClientInfoProto& clientInfo,
    const TString& userPuid,
    const TString actionName,
    const TSemanticFrameRequestData& request,
    int refreshPeriodMinutes);

} // NAlice::NHollywood::NCombinators
