#pragma once

#include <alice/cuttlefish/library/cuttlefish/common/metrics.h>
#include <alice/cuttlefish/library/logging/log_context.h>
#include <alice/cachalot/api/protos/cachalot.pb.h>
#include <alice/cuttlefish/library/protos/session.pb.h>
#include <alice/cuttlefish/library/protos/uniproxy2.pb.h>
#include <alice/cuttlefish/library/protos/wsevent.pb.h>


namespace NAlice::NCuttlefish::NAppHostServices {
    void FillSmartActivation(
        NCachalotProtocol::TActivationAnnouncementRequest&,
        const NAliceProtocol::TRequestContext&,
        const TString& smartActivationUserId,
        const TString& smartActivationDeviceId,
        const TString& smartActivationDeviceModel,
        TLogContext* logContext
    );

    NAliceProtocol::TWsEvent BuildSpotterValidation(
        bool& validationResult,
        const NCachalotProtocol::TActivationFinalResponse&,
        const TMaybe<NCachalotProtocol::TActivationLog>& smartActivationLog,
        const TMaybe<bool>& asrValid,
        const TString& refMessageId
    );

    NAliceProtocol::TWsEvent BuildFallbackSpotterValidation(
        const TMaybe<NCachalotProtocol::TActivationLog>& smartActivationLog,
        const TMaybe<bool>& asrValid,
        const TString& refMessageId
    );

    NAliceProtocol::TUniproxyDirective BuildSessionLogMultiActivation(
        const NCachalotProtocol::TActivationLog&,
        const TString& refMessageId
    );
}
