#pragma once

#include <alice/tests/difftest/shooter/library/yav/yav_requester.h>

#include <util/generic/string.h>
#include <util/generic/hash.h>

namespace NAlice::NShooter {

class IYav : public THashMap<TString, TString> {
public:
    virtual ~IYav() = default;
    IYav(THashMap<TString, TString>&& map);
    IYav(const TString& secretId, const TString& oauthToken, const IYavRequester& requester);
};

class TYav : public IYav {
public:
    TYav(const TString& secretId, const TString& oauthToken, const IYavRequester& requester = TYavRequester());
};

} // namespace NAlice::NShooter
