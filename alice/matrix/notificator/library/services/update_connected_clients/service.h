#pragma once

#include <alice/matrix/notificator/library/services/common_context/common_context.h>
#include <alice/matrix/notificator/library/services/update_connected_clients/protos/service.apphost.h>
#include <alice/matrix/notificator/library/storages/connections/storage.h>
#include <alice/matrix/notificator/library/storages/directives/storage.h>

#include <alice/matrix/library/services/iface/service.h>


namespace NMatrix::NNotificator {

class TUpdateConnectedClientsService
    : public IService
    , public NServiceProtos::TUpdateConnectedClientsServiceAsync
{
public:
    explicit TUpdateConnectedClientsService(
        const TServicesCommonContext& servicesCommonContext
    );

    bool Integrate(NAppHost::TLoop& loop, uint16_t port) override;

private:
    NThreading::TFuture<NServiceProtos::TUpdateConnectedClientsResponse> UpdateConnectedClients(
        NAppHost::TTypedServiceContextPtr ctx,
        const NServiceProtos::TUpdateConnectedClientsRequest* request
    ) override;

private:
    TConnectionsStorage ConnectionsStorage_;
    TDirectivesStorage DirectivesStorage_;
    TRtLogClient& RtLogClient_;

    const bool DisableYDBOperationsForDiffUpdates_;
    const bool DisableYDBOperationsForFullStateUpdates_;
    const bool DisableYDBOperationsForDirectivesSelects_;
};

} // namespace NMatrix::NNotificator
