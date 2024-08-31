#pragma once

#include <alice/matrix/notificator/library/services/common_context/common_context.h>
#include <alice/matrix/notificator/library/storages/directives/storage.h>

#include <alice/matrix/library/services/iface/service.h>

namespace NMatrix::NNotificator {

class TDirectiveService : public IService {
public:
    explicit TDirectiveService(
        const TServicesCommonContext& servicesCommonContext
    );

    bool Integrate(NAppHost::TLoop& loop, uint16_t port) override;

private:
    void OnDirectiveChangeStatusHttpRequest(const NNeh::IRequestRef& request);
    void OnDirectiveStatusHttpRequest(const NNeh::IRequestRef& request);

private:
    TDirectivesStorage DirectivesStorage_;
    TRtLogClient& RtLogClient_;
};

} // namespace NMatrix::NNotificator
