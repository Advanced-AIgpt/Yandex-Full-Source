#pragma once

#include "config.h"

#include <alice/joker/library/backend/backend.h>
#include <alice/joker/library/s3/s3.h>

#include <util/folder/path.h>
#include <util/generic/ptr.h>

namespace NAlice::NJoker {

class TS3Backend : public TBackend {
public:
    explicit TS3Backend(const TConfig::TS3BackendConfigConst& backend);

    TStatus ObtainStub(const TStubId& id, const TString& version, TStubItemPtr& stubItem) override;
    TStatus SaveStub(const TString& version, TStubItemPtr stubItem) override;

private:
    THolder<IS3Storage> Storage_;
};

} // namespace NAlice::NJoker
