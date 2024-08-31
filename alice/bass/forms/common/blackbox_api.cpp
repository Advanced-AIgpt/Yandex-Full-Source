#include "blackbox_api.h"

namespace NBASS {

// TBlackBoxAPI ----------------------------------------------------------------
bool TBlackBoxAPI::GetUid(TContext& context, TString& uid) {
    return TPersonalDataHelper(context).GetUid(uid);
}

bool TBlackBoxAPI::GetUserInfo(TContext& context, TPersonalDataHelper::TUserInfo& userInfo) {
    return TPersonalDataHelper(context).GetUserInfo(userInfo);
}

// TBlackBoxAPIFake ------------------------------------------------------------
TBlackBoxAPIFake::TBlackBoxAPIFake(TStringBuf uid)
    : UID(TString(uid)) {
}

bool TBlackBoxAPIFake::GetUid(TContext& /* ctx */, TString& uid) {
    ++NumCalls;
    if (!UID)
        return false;
    uid = *UID;
    return true;
}

bool TBlackBoxAPIFake::GetUserInfo(TContext& /* ctx */, TPersonalDataHelper::TUserInfo& userInfo) {
    ++NumCalls;
    if (!UID)
        return false;
    userInfo.SetUid(*UID);
    userInfo.SetFirstName("Vasily");
    return true;
}

} // namespace NBASS
