#pragma once

#include "session.h"
#include "fwd.h"
#include "status.h"
#include "stub_id.h"

#include <library/cpp/neh/neh.h>

#include <ydb/public/sdk/cpp/client/ydb_driver/driver.h>
#include <ydb/public/sdk/cpp/client/ydb_table/table.h>

namespace NAlice::NJokerLight {

class TYdb {
public:
    TYdb(const TContext& context);

    void UpsertSession(const TString& sessionId, const TSession::TSettings& settings);
    TMaybe<TSession::TSettings> ObtainSession(const TString& sessionId);

    TMaybe<NYdb::TResultSetParser> ObtainStub(const TStubId& stubId);
    void SaveStubError(const TStubId& stubId, const TError& error);
    void SaveStub(const TStubId& stubId, NNeh::TResponseRef response, const TString& data, const TString& realAddr);

private:
    const TContext& Context_;
    const TString Database_;
    NYdb::TDriver Driver_;
    NYdb::NTable::TTableClient Client_;

    // local session info cache
    THashMap<TString, TSession::TSettings> SessionSettingsCache_;
    THashMap<TString, TInstant> SessionSettingsLoadTime_;
};

} // namespace NAlice::NJokerLight
