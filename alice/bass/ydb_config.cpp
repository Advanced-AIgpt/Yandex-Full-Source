#include "ydb_config.h"

#include "ydb.h"

#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/ydb_helpers/exception.h>
#include <alice/bass/libs/ydb_helpers/settings.h>
#include <alice/bass/libs/logging_v2/logger.h>

namespace NBASS {

TYdbConfig::TYdbConfig(const TConfig& config) {
    LOG(INFO) << "Starting YDB client (endpoint: " << config.YDb().Endpoint().Get() << ')' << Endl;

    const auto driver = ConstructYdbDriver(config);
    Client = MakeHolder<NYdb::NTable::TTableClient>(driver);
    Table = MakeHolder<NYdbConfig::TConfig>(*Client, TString{*config.YDb().DataBase()},
                                            NYdbHelpers::DefaultYdbRetrySettings());

    LOG(INFO) << "YDB client started" << Endl;
}

void TYdbConfig::Update() {
    LOG(INFO) << "Updating YDB config..." << Endl;

    Y_ASSERT(Table);

    const auto status = Table->GetAll();
    if (!status.IsSuccess()) {
        LOG(ERR) << "Failed to update YDB Config: " << NYdbHelpers::StatusToString(status) << Endl;
        return;
    }

    THashMap<TString, TString> kvs;
    for (const auto& kv : status.GetKeyValues()) {
        if (kv.HasKey() && kv.HasValue())
            kvs[kv.GetKey()] = kv.GetValue();
    }

    {
        TWriteGuard guard(Mutex);
        KVs.swap(kvs);
    }
}

TMaybe<TString> TYdbConfig::Get(TStringBuf key) {
    TReadGuard guard(Mutex);
    return GetImpl(key);
}

TString TYdbConfig::GetOrDefault(TStringBuf key, TStringBuf value) {
    TReadGuard guard(Mutex);
    if (const auto* p = KVs.FindPtr(key))
        return *p;
    return TString{value};
}

TVector<TMaybe<TString>> TYdbConfig::GetMany(const TVector<TStringBuf>& keys) {
    TReadGuard guard(Mutex);
    TVector<TMaybe<TString>> values;
    for (const auto& key : keys)
        values.push_back(GetImpl(key));
    return values;
}

TMaybe<TString> TYdbConfig::GetImpl(TStringBuf key) {
    if (const auto* p = KVs.FindPtr(key))
        return *p;
    return Nothing();
}
} // namespace NBASS
