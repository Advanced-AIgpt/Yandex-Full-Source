#pragma once

#include <util/generic/hash.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/maybe.h>

namespace NAlice::NSkillDiscovery {

inline constexpr TStringBuf IMAGE_TYPE_BIG = "one-x";
inline constexpr TStringBuf IMAGE_TYPE_ORIG = "orig";
inline constexpr TStringBuf IMAGE_TYPE_MOBILE_LOGO = "mobile-logo-x";
inline constexpr TStringBuf IMAGE_TYPE_SMALL = "menu-list-x";

inline constexpr TStringBuf AVATAR_NAMESPACE_SKILL_IMAGE = "dialogs-skill-card";
inline constexpr TStringBuf AVATAR_NAMESPACE_SKILL_LOGO = "dialogs";

double NormalizeRelev(double relev);
bool IsCommercialQuery(double commercialMx);
TString CreateImageUrl(const TStringBuf avatarHost,
                       const TStringBuf imageId,
                       const TStringBuf imageType,
                       const TStringBuf ns,
                       double scaleFactor);

} // NAlice::NSkillDiscovery
