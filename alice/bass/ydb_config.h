#pragma once

#include <alice/bass/libs/config/config.h>
#include <alice/bass/libs/globalctx/fwd.h>
#include <alice/bass/libs/ydb_config/config.h>

#include <ydb/public/sdk/cpp/client/ydb_table/table.h>

#include <util/datetime/base.h>
#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>
#include <util/generic/singleton.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <util/system/rwlock.h>

namespace NBASS {

class TYdbConfig {
public:
    TYdbConfig(const TConfig& config);

    void Update();

    TMaybe<TString> Get(TStringBuf key);

    TString GetOrDefault(TStringBuf key, TStringBuf value);

    TVector<TMaybe<TString>> GetMany(const TVector<TStringBuf>& keys);

private:
    TMaybe<TString> GetImpl(TStringBuf key);

private:
    // Following fields must be used on the scheduler thread only.
    THolder<NYdb::NTable::TTableClient> Client;
    THolder<NYdbConfig::TConfig> Table;

    // Following fields can be used in form handlers, therefore must
    // be protected.
    THashMap<TString, TString> KVs;
    TRWMutex Mutex;
};
} // namespace NBASS
