#pragma once

#include "biometry_data.h"

#include <alice/library/biometry/biometry.h>
#include <alice/library/data_sync/data_sync.h>
#include <alice/library/logger/logger.h>

#include <alice/hollywood/library/biometry/biometry_delegate.h>
#include <alice/hollywood/library/personal_data/personal_data.h>
#include <alice/hollywood/library/request/request.h>

namespace NAlice::NHollywood::NMusic {

bool IsClassifiedAsChildRequest(const TBiometryClassification& biometryClassification);

TBiometryData ConstructBiometryDataWithGuestUserId(const TString& guestUserId);

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
TMaybe<TBiometryData> ProcessBiometry(TRTLogger& logger,
                     const TScenarioRequestWrapper& request, TStringBuf uid)
{
    TBiometryData data;
    auto biometryDelegate = MakeBiometryDelegate(logger, request, uid);

    const auto& input = request.Input().Proto();
    if (!input.HasVoice() && !input.GetVoice().HasBiometryScoring()) {
        LOG_INFO(logger) << "No biometry scoring data found";
        return Nothing();
    }
    NAlice::NBiometry::TBiometry biometry{input.GetVoice().GetBiometryScoring(), biometryDelegate,
                                          NBiometry::TBiometry::EMode::HighTPR};
    if (const auto err = biometry.GetUserId(data.UserId)) {
        LOG_WARN(logger) << "Error getting user id " << err->ToJson();
        return Nothing();
    }

    // "Guest user" term has new semantic since https://st.yandex-team.ru/HOLLYWOOD-537
    data.IsIncognitoUser = biometry.IsGuestUser();
    data.IsGuestUser = false;

    if (data.IsIncognitoUser) {
        // In such a case Alice says that "только <ownerName> может ставить лайки"
        NAlice::NBiometry::TBiometry noguestBiometry{input.GetVoice().GetBiometryScoring(), biometryDelegate,
                                                     NBiometry::TBiometry::EMode::NoGuest};
        if (const auto err = noguestBiometry.GetUserName(data.OwnerName)) {
            LOG_WARN(logger) << "Error getting user name " << err->ToJson();
        }
    }
    return data;
}

TMaybe<TBiometryData> ProcessBiometrySpecified(TRTLogger& logger, const TScenarioRunRequestWrapper& request, TStringBuf uid);
TMaybe<TBiometryData> ProcessBiometrySpecified(TRTLogger& logger, const TScenarioApplyRequestWrapper& request, TStringBuf uid);

template<typename TScenarioRequestWrapper>
TMaybe<bool> IsClassifiedAsChildRequest(const TScenarioRequestWrapper& request) {
    const auto& input = request.Input().Proto();
    if (input.GetVoice().HasBiometryClassification()) {
        return IsClassifiedAsChildRequest(input.GetVoice().GetBiometryClassification());
    } else {
        return Nothing();
    }
}

template<typename TScenarioRequestWrapper>
TBiometryData ProcessBiometryOrFallback(TRTLogger& logger, const TScenarioRequestWrapper& request, TStringBuf uid) {
    TBiometryData biometryData;

    if (auto data = ProcessBiometrySpecified(logger, request, uid)) {
        LOG_INFO(logger) << "ProcessBiometry success isIncognitoUser=" << data->IsIncognitoUser << ", isGuestUser=" << data->IsGuestUser
                         << ", ownerName=" << data->OwnerName << ", userId=" << data->UserId;
        biometryData = std::move(*data);
    } else {
        LOG_INFO(logger) << "ProcessBiometry reject: no biometry data or broken biometry data... We act as the owner with uid=" << uid;
        biometryData = {.IsIncognitoUser = false, .IsGuestUser = false, .OwnerName = "", .UserId = TString{uid}};
    }

    if (const auto isChild = IsClassifiedAsChildRequest(request); isChild.Defined()) {
        LOG_INFO(logger) << "ProcessBiometry child classification success isChild=" << *isChild;
        biometryData.IsChild = *isChild;
    } else {
        LOG_WARN(logger) << "ProcessBiometry child classification reject... We act as the user is not a child";
        biometryData.IsChild = false;
    }

    return biometryData;
}

} // namespace NAlice::NHollywood::NMusic
