#include <alice/matrix/worker/library/config/config.pb.h>
#include <alice/matrix/worker/library/services/common_context/common_context.h>
#include <alice/matrix/worker/library/services/worker/service.h>

#include <alice/matrix/library/daemon/main.h>
#include <alice/matrix/library/services/metrics/main_metrics_service.h>
#include <alice/matrix/library/services/metrics/ydb_metrics_service.h>

static constexpr char MATRIX_WORKER[] = "matrix_worker";

int main(int argc, const char* argv[]) {
    return NMatrix::RunDaemon<
        MATRIX_WORKER,
        NMatrix::NWorker::TServicesCommonContextBuilder,

        // Services
        NMatrix::TMainMetricsService,
        NMatrix::TYDBMetricsService,

        NMatrix::NWorker::TWorkerService
    >(argc, argv);
}
