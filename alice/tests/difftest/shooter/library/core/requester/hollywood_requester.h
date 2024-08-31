#pragma once

#include <alice/tests/difftest/shooter/library/core/requester/requester.h>
#include <alice/tests/difftest/shooter/library/core/fwd.h>

#include <util/generic/string.h>
#include <util/folder/path.h>

namespace NAlice::NShooter {

class THollywoodRequester : public IRequester {
public:
    THollywoodRequester(const IEngine& engine);
    TMaybe<TRequestResponse> Request(const TFsPath& path) const override;

private:
    const IEngine& Engine_;
};

} // namespace NAlice::NShooter
