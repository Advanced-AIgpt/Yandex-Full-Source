#include <alice/matrix/scheduler/library/config/config.pb.h>
#include <alice/matrix/scheduler/library/services/common_context/common_context.h>
#include <alice/matrix/scheduler/library/services/scheduler/service.h>

#include <alice/matrix/library/daemon/main.h>
#include <alice/matrix/library/services/metrics/main_metrics_service.h>
#include <alice/matrix/library/services/metrics/ydb_metrics_service.h>

static constexpr char MATRIX_SCHEDULER[] = "matrix_scheduler";

int main(int argc, const char* argv[]) {
    return NMatrix::RunDaemon<
        MATRIX_SCHEDULER,
        NMatrix::NScheduler::TServicesCommonContextBuilder,

        // Services
        NMatrix::TMainMetricsService,
        NMatrix::TYDBMetricsService,

        NMatrix::NScheduler::TSchedulerService
    >(argc, argv);
}
