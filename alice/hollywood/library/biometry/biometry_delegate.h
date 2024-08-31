#pragma once

#include <alice/library/biometry/biometry.h>

namespace NAlice::NHollywood {

class TBiometryDelegate final : public NAlice::NBiometry::TBiometry::IDelegate {
public:
    TBiometryDelegate(TMaybe<TString> userName, TMaybe<TString> userId, TMaybe<TString> guestId)
    : UserName_(std::move(userName))
    , UserId_(std::move(userId))
    , GuestId_(std::move(guestId)) {
    }

    TMaybe<TString> GetUid() const override {
        return UserId_;
    }

    NAlice::NBiometry::TResult GetUserName(TStringBuf userId, TString& userName) const override;

    NAlice::NBiometry::TResult GetGuestId(TStringBuf userId, TString& guestId) const override;

private:
    TMaybe<TString> UserName_;
    TMaybe<TString> UserId_;
    TMaybe<TString> GuestId_;
};

} // namespace NAlice::NHollywood
