#include "ydb_helpers.h"

namespace NRTLog {
    using namespace NYdb;

    TDriverConfig ToDriverConfig(const TYdbSettings& ydbSettings) {
        return TDriverConfig()
            .SetEndpoint(ydbSettings.Endpoint)
            .SetDatabase(ydbSettings.Database)
            .SetAuthToken(ydbSettings.AuthToken);
    }
}
