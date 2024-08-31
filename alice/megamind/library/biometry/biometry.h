#pragma once

#include <alice/megamind/library/util/status.h>

#include <alice/megamind/protos/common/events.pb.h>
#include <alice/megamind/protos/scenarios/request.pb.h>

namespace NAlice::NMegamind {

inline constexpr TStringBuf BIOMETRY_AGE_TAG = "children";
inline constexpr TStringBuf BIOMETRY_GENDER_TAG = "gender";

inline constexpr TStringBuf BIOMETRY_GENDER_FEMALE = "female";
inline constexpr TStringBuf BIOMETRY_GENDER_MALE = "male";
inline constexpr TStringBuf BIOMETRY_GENDER_UNKNOWN = "unknown";

inline constexpr TStringBuf BIOMETRY_AGE_ADULT = "adult";
inline constexpr TStringBuf BIOMETRY_AGE_CHILD = "child";

NScenarios::TUserClassification ParseUserClassification(const TBiometryClassification& biometryClassification);

} // namespace NAlice::NMegamind
