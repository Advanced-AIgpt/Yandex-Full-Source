#include <alice/matrix/notificator/library/config/config.pb.h>

#include <alice/matrix/notificator/library/services/common_context/common_context.h>
#include <alice/matrix/notificator/library/services/delivery/service.h>
#include <alice/matrix/notificator/library/services/devices/service.h>
#include <alice/matrix/notificator/library/services/directive/service.h>
#include <alice/matrix/notificator/library/services/gdpr/service.h>
#include <alice/matrix/notificator/library/services/locator/service.h>
#include <alice/matrix/notificator/library/services/notifications/service.h>
#include <alice/matrix/notificator/library/services/proxy/service.h>
#include <alice/matrix/notificator/library/services/subscriptions/service.h>
#include <alice/matrix/notificator/library/services/update_connected_clients/service.h>
#include <alice/matrix/notificator/library/services/update_device_environment/service.h>

#include <alice/matrix/library/daemon/main.h>
#include <alice/matrix/library/services/metrics/main_metrics_service.h>
#include <alice/matrix/library/services/metrics/ydb_metrics_service.h>

static constexpr char MATRIX[] = "matrix";

int main(int argc, const char* argv[]) {
    return NMatrix::RunDaemon<
        MATRIX,
        NMatrix::NNotificator::TServicesCommonContextBuilder,

        // Services

        // HTTP
        NMatrix::TMainMetricsService,
        NMatrix::TYDBMetricsService,

        NMatrix::NNotificator::TProxyService,

        NMatrix::NNotificator::TDeliveryService,
        NMatrix::NNotificator::TDevicesService,
        NMatrix::NNotificator::TDirectiveService,
        NMatrix::NNotificator::TGDPRService,
        NMatrix::NNotificator::TLocatorService,
        NMatrix::NNotificator::TNotificationsService,
        NMatrix::NNotificator::TSubscriptionsService,

        // AppHost (GRPC)
        NMatrix::NNotificator::TUpdateConnectedClientsService,
        NMatrix::NNotificator::TUpdateDeviceEnvironmentService
    >(argc, argv);
}
