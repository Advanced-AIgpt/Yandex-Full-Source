#pragma once

#include <alice/library/biometry/biometry.h>
#include <alice/library/data_sync/data_sync.h>
#include <alice/library/logger/logger.h>

#include <alice/hollywood/library/biometry/biometry_delegate.h>
#include <alice/hollywood/library/personal_data/personal_data.h>
#include <alice/hollywood/library/request/request.h>

#include <util/generic/maybe.h>
#include <util/generic/string.h>

namespace NAlice::NHollywood::NVoiceprint {

struct TServerBiometryData {
    bool IsIncognitoUser;
    TString UserName;
    TString Uid;
};

template<typename TScenarioRequestWrapper>
TBiometryDelegate MakeBiometryDelegate(TRTLogger& logger, const TScenarioRequestWrapper& request, TStringBuf uid) {
    auto userNameKey = ToPersonalDataKey(request.ClientInfo(), NAlice::NDataSync::EUserSpecificKey::UserName);
    const auto* userNamePtr = request.GetPersonalDataString(userNameKey);
    TMaybe<TString> userName;
    if (userNamePtr) {
        userName.ConstructInPlace(*userNamePtr);
    }

    auto guestUidKey = ToPersonalDataKey(request.ClientInfo(), NAlice::NDataSync::EUserSpecificKey::GuestUID);
    const auto* guestUidPtr = request.GetPersonalDataString(guestUidKey);
    TMaybe<TString> guestUid;
    if (guestUidPtr) {
        guestUid.ConstructInPlace(*guestUidPtr);
    }

    LOG_INFO(logger) << "Constructing biometry delegate with userName=" << userName << ", uid=" << uid
                     << ", guestUid=" << guestUid;

    return TBiometryDelegate{userName, TString{uid}, guestUid};
}

template<typename TScenarioRequestWrapper>
TMaybe<TServerBiometryData> ProcessServerBiometry(TRTLogger& logger,
                                                  const TScenarioRequestWrapper& request,
                                                  TStringBuf uid,
                                                  NBiometry::TBiometry::EMode baseMode,
                                                  bool addMaxAccuracyIncognitoCheck)
{
    TServerBiometryData data;
    auto biometryDelegate = MakeBiometryDelegate(logger, request, uid);

    const auto& input = request.Input().Proto();
    if (!input.HasVoice() && !input.GetVoice().HasBiometryScoring()) {
        LOG_INFO(logger) << "No biometry scoring data found";
        return Nothing();
    }

    NAlice::NBiometry::TBiometry baseBiometry{input.GetVoice().GetBiometryScoring(), biometryDelegate, baseMode};

    if (const auto err = baseBiometry.GetUserId(data.Uid)) {
        LOG_WARN(logger) << "Error geting user id: " << err->ToJson();
        return Nothing();
    }

    if (const auto err = baseBiometry.GetUserName(data.UserName)) {
        LOG_WARN(logger) << "Error getting user name: " << err->ToJson();
    }

    if (addMaxAccuracyIncognitoCheck) {
        NAlice::NBiometry::TBiometry maxAccuracyBiometry{input.GetVoice().GetBiometryScoring(), biometryDelegate, NBiometry::TBiometry::EMode::MaxAccuracy};
        data.IsIncognitoUser = !maxAccuracyBiometry.IsKnownUser();
    } else {
        data.IsIncognitoUser = !baseBiometry.IsKnownUser();
    }

    return data;
}

} // namespace NAlice::NHollywood::NVoiceprint
