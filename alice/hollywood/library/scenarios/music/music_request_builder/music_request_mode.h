#pragma once

#include <alice/hollywood/library/scenarios/music/biometry/biometry_data.h>
#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>

namespace NAlice::NHollywood::NMusic {

enum class ERequestMode {
    Unknown /* "unknown" */,
    Owner /* "owner" */,
    Incognito /* "incognito" */,
    Guest /* "guest" */,
    ClientBiometry /* "client_biometry" */, // NOTE: client biometry was used (may be both owner or guest)
};

enum class EAuthMethod {
    Unknown /* "unknown" */,
    UserId /* "user_id" */,
    OAuth /* "oauth" */,
};

ERequestMode GetRequestMode(const TBiometryData& biometryData);
ERequestMode ToRequestMode(TBiometryOptions::EPlaybackMode playbackMode);

struct TMusicRequestModeInfo {
    EAuthMethod AuthMethod;
    ERequestMode RequestMode;
    TStringBuf OwnerUserId;
    TStringBuf RequesterUserId;
};

class TMusicRequestModeInfoBuilder {
public:
    TMusicRequestModeInfoBuilder();

    TMusicRequestModeInfoBuilder& SetAuthMethod(EAuthMethod authMethod);

    TMusicRequestModeInfoBuilder& SetRequestMode(ERequestMode requestMode);

    TMusicRequestModeInfoBuilder& SetOwnerUserId(const TStringBuf ownerUserId);

    TMusicRequestModeInfoBuilder& SetRequesterUserId(const TStringBuf requesterUserId);

    TMusicRequestModeInfo Build() const;
    TMusicRequestModeInfo BuildAndMove();

private:
    TMusicRequestModeInfo MusicRequestModeInfo_;
};

} // namespace NAlice::NHollywood::NMusic
