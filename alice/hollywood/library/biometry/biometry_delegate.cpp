#include "biometry_delegate.h"

namespace NAlice::NHollywood {

NAlice::NBiometry::TResult TBiometryDelegate::GetUserName(TStringBuf userId, TString& userName) const {
    if (UserId_.Defined() && userId == *UserId_ && UserName_.Defined()) {
        userName = *UserName_;
        return {};
    }
    return NAlice::NBiometry::TError{NAlice::NBiometry::EErrorType::Logic};
}

NAlice::NBiometry::TResult TBiometryDelegate::GetGuestId(TStringBuf userId, TString& guestId) const {
    if (UserId_.Defined() && GuestId_.Defined() && userId == *UserId_) {
        guestId = *GuestId_;
        return {};
    }
    return NAlice::NBiometry::TError{NAlice::NBiometry::EErrorType::Logic};
}

} // namespace NAlice::NHollywood
