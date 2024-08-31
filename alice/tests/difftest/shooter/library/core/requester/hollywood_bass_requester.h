#pragma once

#include <alice/tests/difftest/shooter/library/core/requester/requester.h>
#include <alice/tests/difftest/shooter/library/core/fwd.h>

#include <util/generic/string.h>
#include <util/folder/path.h>

namespace NAlice::NShooter {

class THollywoodBassRequester : public IRequester {
public:
    THollywoodBassRequester(const IContext& ctx, const IEngine& engine);
    TMaybe<TRequestResponse> Request(const TFsPath& path) const override;

private:
    const IContext& Ctx_;
    const IEngine& Engine_;
};

} // namespace NAlice::NShooter
