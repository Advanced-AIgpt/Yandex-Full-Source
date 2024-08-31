#pragma once

#include <util/generic/string.h>

namespace NAlice::NShooter {

class IYavRequester {
public:
    virtual ~IYavRequester() = default;
    virtual TString Request(const TString& secretId, const TString& oauthToken) const = 0;
};

class TYavRequester : public IYavRequester {
public:
    TString Request(const TString& secretId, const TString& oauthToken) const override;
};

} // namespace NAlice::NShooter
