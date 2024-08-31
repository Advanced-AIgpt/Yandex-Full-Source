#pragma once

#include "config.h"

#include <alice/joker/library/backend/backend.h>

#include <util/folder/path.h>

namespace NAlice::NJoker {

class TFSBackend : public TBackend {
public:
    explicit TFSBackend(const TConfig::TFSBackendConfigConst& backend);

    TStatus ObtainStub(const TStubId& id, const TString& version, TStubItemPtr& stubItem) override;
    TStatus SaveStub(const TString& version, TStubItemPtr stubItem) override;

private:
    const TFsPath Dir_;
};


} // namespace NAlice::NJoker
