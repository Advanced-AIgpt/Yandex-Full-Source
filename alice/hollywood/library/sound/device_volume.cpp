#include "device_volume.h"

#include <util/generic/hash.h>

#include <cmath>


namespace {

constexpr i64 MIN_VOLUME = 0;
constexpr i64 MIN_AUDIBLE_VOLUME = 1;

const THashMap<TString, float> VOLUME_MAP = {
    {"minimum", 0.1},
    {"very_quiet", 0.1},
    {"quiet", 0.2},
    {"middle", 0.4},
    {"high", 0.8},
    {"very_high", 0.9},
    {"maximum", 1},
};

// number of steps of volume change
// ex: medium change = 2 steps
const THashMap<TString, int> VOLUME_RELATIVE_MAP = {
    {"small", 1},
    {"medium", 2},
    {"big", 4},
    {"maximal", 10},
};

} // namespace

namespace NAlice::NHollywood::NSound {

TDeviceVolume::TDeviceVolume(i64 current, i64 max, i64 step)
    : Current{current}
    , Max{max}
{
    if (!step) {
        // step is 5% of MaxVolume, but not less than 1
        Step = std::round(std::max(1.0f, static_cast<float>(max)*0.05f));
    } else {
        Step = step;
    }
};

i64 TDeviceVolume::GetCurrent() const {
    return Current;
};

i64 TDeviceVolume::GetMax() const {
    return Max;
};

i64 TDeviceVolume::GetMin() const {
    return Min;
};

i64 TDeviceVolume::IsMaximum() const {
    return Current >= Max;
};

i64 TDeviceVolume::IsMinimum() const {
    return Current <= Min;
};

i64 TDeviceVolume::IsSupported(i64 level) const {
    return Min <= level && level <= Max;
};

i64 TDeviceVolume::AbsoluteLevelDegree(TStringBuf key) const {
    if (const float* level = VOLUME_MAP.FindPtr(key)) {
        const i64 scaleFactor = Max;
        return std::round(*level * scaleFactor);
    }
    return Current;
};

i64 TDeviceVolume::AbsoluteLevelFloat(float level) {
    return std::round(level);
};

i64 TDeviceVolume::AbsoluteLevelPercent(float percent) const {
    return std::round(Max * percent);
};

i64 TDeviceVolume::AbsoluteLevelWithDenominator(i64 level, i64 denominator) const {
    if (denominator != 0) {
        return std::round(static_cast<float>(level * Max) / denominator);
    }
    return level;
};

i64 TDeviceVolume::LouderOneStep(i64 sign) const {
    const int _sign = sign > 0 ? 1 : -1;
    return FitInRange(Current + Step * _sign);
};

i64 TDeviceVolume::LouderSteps(i64 steps) const {
    return FitInRange(Current + Step * steps, Current > MIN_AUDIBLE_VOLUME);
};

i64 TDeviceVolume::LouderBy(i64 delta) const {
    return FitInRange(Current + delta);
};

i64 TDeviceVolume::LouderBy(float delta) const {
    return FitInRange(std::round(Current + delta));
};

i64 TDeviceVolume::LouderByFactor(float factor) const {
    // if volume is 0 - assume it as 1
    const float volume = std::max(1.0f, static_cast<float>(Current)) * factor;
    return FitInRange(std::round(volume));
};

i64 TDeviceVolume::QuiterByFactor(float factor) const {
    if (factor != 0) {
        const float volume = static_cast<float>(Current) / factor;
        return FitInRange(std::round(volume), Current > MIN_AUDIBLE_VOLUME);
    }
    return Current;
};

i64 TDeviceVolume::LouderByPercent(float percent) const {
    i64 newLevel;
    const i64 delta = std::round(static_cast<float>(Current) * percent);

    // if user asks to make louder, we must make it louder at least one step
    if (delta == 0) {
        newLevel = percent > 0 ? Current + 1 : Current - 1;
    } else {
        newLevel = Current + delta;
    }

    // if we can make it quiter and not mute sound - let's do it
    // but if current volume already minimum, than we have no other choice but mute
    return FitInRange(newLevel, Current > MIN_AUDIBLE_VOLUME);
};

i64 TDeviceVolume::LouderByDegree(TStringBuf key, i64 sign) const {
    i64 delta = Step;
    if (const auto* ptr = VOLUME_RELATIVE_MAP.FindPtr(key)) {
        delta = Step * *ptr;
    }
    return FitInRange(Current + sign * delta, Current > 1);
};

i64 TDeviceVolume::QuiterByDegree(TStringBuf key) const {
    return LouderByDegree(key, -1);
};

i64 TDeviceVolume::FitInRange(i64 level, bool doNotMute) const {
    // Fit level in range [MIN_VOLUME; MAX_VOLUME]
    // Or in range [MIN_AUDIBLE_VOLUME; MAX_VOLUME] if doNotMute

    if (doNotMute && level <= MIN_AUDIBLE_VOLUME) {
        return MIN_AUDIBLE_VOLUME;
    }
    if (!doNotMute && level <= MIN_VOLUME) {
        return MIN_VOLUME;
    }
    if (level >= Max) {
        return Max;
    }
    return level;
};

} // namespace NAlice::NHollywood::NSound
