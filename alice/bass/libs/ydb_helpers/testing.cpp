#include "testing.h"

#include <alice/bass/libs/logging_v2/logger.h>

#include <util/stream/file.h>

namespace NYdbHelpers {
TLocalDatabase::~TLocalDatabase() {
    if (Driver)
        Driver->Stop(true /* wait */);
}

void TLocalDatabase::Init() {
    try {
        TFileInput d("ydb_database.txt");
        Database = d.ReadLine();

        TFileInput e("ydb_endpoint.txt");
        Endpoint = e.ReadLine();
    } catch (const TFileError&) {
        LOG(INFO) << "If you see this message then you probably need to "
                  << "run these tests via \"ya make -ttt\"" << Endl;

        throw;
    }

    const auto config = NYdb::TDriverConfig{}.SetEndpoint(Endpoint).SetDatabase(Database);
    Driver = MakeHolder<NYdb::TDriver>(config);
}
} // namespace NYdbHelpers
