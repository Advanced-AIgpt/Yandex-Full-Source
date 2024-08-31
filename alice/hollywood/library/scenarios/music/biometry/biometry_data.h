#pragma once

#include <util/generic/string.h>

namespace NAlice::NHollywood::NMusic {

struct TBiometryData {
   bool IsIncognitoUser;
   bool IsGuestUser;
   bool IsChild;
   TString OwnerName;
   TString UserId;
};

} // namespace NAlice::NHollywood::NMusic
