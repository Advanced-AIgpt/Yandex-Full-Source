#pragma once

#include "config.h"
#include "requests_history.h"
#include "memory_storage.h"

#include <util/generic/fwd.h>

namespace NAlice::NJoker {

class TGlobalContext {
public:
    TGlobalContext(const TString& configFileName);
    ~TGlobalContext();

    const TConfig& Config() const {
        return Config_;
    }

    TMemoryStorage& MemoryStorage();
    TRequestsHistory& RequestsHistory();

private:
    const TConfig Config_;
    TMemoryStorage MemoryStorage_;
    TRequestsHistory RequestsHistory_;
};

} // namespace NAlice::NJoker
