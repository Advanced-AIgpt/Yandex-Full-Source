#include "service.h"

#include "update_connected_clients_request.h"

#include <alice/matrix/library/services/typed_apphost_service/utils.h>


namespace NMatrix::NNotificator {

TUpdateConnectedClientsService::TUpdateConnectedClientsService(
    const TServicesCommonContext& servicesCommonContext
)
    : ConnectionsStorage_(
        servicesCommonContext.YDBDriver,
        servicesCommonContext.Config.GetUpdateConnectedClientsService().GetYDBClient()
    )
    , DirectivesStorage_(
        servicesCommonContext.YDBDriver,
        servicesCommonContext.Config.GetUpdateConnectedClientsService().GetYDBClient()
    )
    , RtLogClient_(servicesCommonContext.RtLogClient)
    , DisableYDBOperationsForDiffUpdates_(servicesCommonContext.Config.GetUpdateConnectedClientsService().GetDisableYDBOperationsForDiffUpdates())
    , DisableYDBOperationsForFullStateUpdates_(servicesCommonContext.Config.GetUpdateConnectedClientsService().GetDisableYDBOperationsForFullStateUpdates())
    , DisableYDBOperationsForDirectivesSelects_(servicesCommonContext.Config.GetUpdateConnectedClientsService().GetDisableYDBOperationsForDirectivesSelects())
{}

NThreading::TFuture<NServiceProtos::TUpdateConnectedClientsResponse> TUpdateConnectedClientsService::UpdateConnectedClients(
    NAppHost::TTypedServiceContextPtr ctx,
    const NServiceProtos::TUpdateConnectedClientsRequest* request
) {
    if (IsSuspended()) {
        return CreateTypedAppHostServiceIsSuspendedFastError<NServiceProtos::TUpdateConnectedClientsResponse>();
    }

    auto req = MakeIntrusive<TUpdateConnectedClientsRequest>(
        GetActiveRequestCounterRef(),
        RtLogClient_,
        ctx,
        *request,
        ConnectionsStorage_,
        DirectivesStorage_,
        DisableYDBOperationsForDiffUpdates_,
        DisableYDBOperationsForFullStateUpdates_,
        DisableYDBOperationsForDirectivesSelects_
    );

    if (req->IsFinished()) {
        return req->Reply();
    }

    return req->ServeAsync().Apply([req](const NThreading::TFuture<void>& fut) {
        return req->ReplyWithFutureCheck(fut);
    });
}

bool TUpdateConnectedClientsService::Integrate(NAppHost::TLoop& loop, uint16_t port) {
    loop.RegisterService(port, *this);

    // Init metrics
    TSourceMetrics metrics(TUpdateConnectedClientsRequest::NAME);
    metrics.InitAppHostResponseOk();

    return true;
}

} // namespace NMatrix::NNotificator
