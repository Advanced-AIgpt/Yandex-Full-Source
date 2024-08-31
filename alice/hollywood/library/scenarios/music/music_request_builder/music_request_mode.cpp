#include "music_request_mode.h"

namespace NAlice::NHollywood::NMusic {

ERequestMode GetRequestMode(const TBiometryData& biometryData) {
    if (biometryData.IsIncognitoUser) {
        return ERequestMode::Incognito;
    } else if (biometryData.IsGuestUser) {
        return ERequestMode::Guest;
    } else {
        return ERequestMode::Owner;
    }
}

ERequestMode ToRequestMode(TBiometryOptions::EPlaybackMode playbackMode) {
    switch (playbackMode) {
    case TBiometryOptions_EPlaybackMode_OwnerMode:
        return ERequestMode::Owner;
    case TBiometryOptions_EPlaybackMode_IncognitoMode:
        return ERequestMode::Incognito;
    case TBiometryOptions_EPlaybackMode_GuestMode:
        return ERequestMode::Guest;
    default:
        return ERequestMode::Unknown;
    }
}

TMusicRequestModeInfoBuilder::TMusicRequestModeInfoBuilder() {
    MusicRequestModeInfo_.AuthMethod = EAuthMethod::Unknown;
    MusicRequestModeInfo_.RequestMode = ERequestMode::Unknown;
    MusicRequestModeInfo_.OwnerUserId = "unspecified";
    MusicRequestModeInfo_.RequesterUserId = "unspecified";
}

TMusicRequestModeInfoBuilder& TMusicRequestModeInfoBuilder::SetAuthMethod(EAuthMethod authMethod) {
    MusicRequestModeInfo_.AuthMethod = authMethod;
    return *this;
}

TMusicRequestModeInfoBuilder& TMusicRequestModeInfoBuilder::SetRequestMode(ERequestMode requestMode) {
    MusicRequestModeInfo_.RequestMode = requestMode;
    return *this;
}

TMusicRequestModeInfoBuilder& TMusicRequestModeInfoBuilder::SetOwnerUserId(const TStringBuf ownerUserId) {
    MusicRequestModeInfo_.OwnerUserId = ownerUserId;
    return *this;
}

TMusicRequestModeInfoBuilder& TMusicRequestModeInfoBuilder::SetRequesterUserId(const TStringBuf requesterUserId) {
    MusicRequestModeInfo_.RequesterUserId = requesterUserId;
    return *this;
}

TMusicRequestModeInfo TMusicRequestModeInfoBuilder::Build() const {
    return MusicRequestModeInfo_;
}

TMusicRequestModeInfo TMusicRequestModeInfoBuilder::BuildAndMove() {
    return std::move(MusicRequestModeInfo_);
}

} // namespace NAlice::NHollywood::NMusic
