#include "ydb.h"

#include <alice/bass/libs/config/config.h>

#include <util/generic/yexception.h>

namespace NBASS {
NYdb::TDriver ConstructYdbDriver(const TConfig& config) {
    if (!config.HasYDb())
        ythrow yexception() << "Ydb section must be defined in config";

    const auto& ydb = config.YDb();
    const NYdb::TDriverConfig ydbConfig = NYdb::TDriverConfig()
                                              .SetEndpoint(TString{*ydb.Endpoint()})
                                              .SetDatabase(TString{*ydb.DataBase()})
                                              .SetAuthToken(TString{*ydb.Token()});
    return NYdb::TDriver{ydbConfig};
}
} // namespace NBASS
