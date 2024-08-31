#pragma once

#include <alice/hollywood/library/scenarios/general_conversation/proto/general_conversation.pb.h>

#include <ydb/public/sdk/cpp/client/ydb_driver/driver.h>
#include <ydb/public/sdk/cpp/client/ydb_table/table.h>

namespace NAlice::NHollywood::NGeneralConversation {

class TLongSessionClient {
public:
    struct TParams {
        TString Endpoint;
        TString Database;
        TString Path;
        TString AuthToken;
    };

public:
    TLongSessionClient(const TParams& params);

public:
    TLongSession Get(const TString& uuid);
    void Update(const TString& uuid, const TLongSession& longSession, ui64 serverTimeMs);
    void Shutdown();

private:
    std::unique_ptr<NYdb::TDriver> Driver;
    std::unique_ptr<NYdb::NTable::TTableClient> Client;
    const TString Path;
};

} // namespace NAlice::NHollywood::NGeneralConversation
