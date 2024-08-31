#include "sound_level_calculation.h"

#include <util/generic/is_in.h>
#include <util/string/cast.h>

#include <utility>

namespace {

constexpr i64 MIN_VOLUME_PERCENTS = 21;
constexpr i64 MAX_VOLUME_PERCENTS = 100;

} // namespace

namespace NAlice::NHollywood::NSound {

i64 CalculateSoundLevelForSetLevel(const TFrame& frame, const TDeviceVolume& deviceSound, const ui64 onboardingUsageCounter) {
    if (const auto lvlSlot = frame.FindSlot("level")) {
        // Like "set volume 7"
        const TString& lvlSlotType = lvlSlot->Type;

        if (lvlSlotType == "sys.num" || lvlSlotType == "custom.number" || lvlSlotType == "sys.float") {
            i64 level = deviceSound.GetCurrent();
            if (lvlSlotType == "sys.float") {
                const float value = FromString<float>(lvlSlot->Value.AsString());
                if (value >= 0.0 && value <= 1.0) {
                    // set volume to one third
                    return deviceSound.AbsoluteLevelPercent(value);
                } else {
                    // set volume 8.5 (will be additionally processed later for negative values)
                    level = deviceSound.AbsoluteLevelFloat(value);
                }
            } else {
                // Like "set volume on 4" or "set volume on 1 out of 5" or "set volume on 30%" or "set volume on 30 (meaning percents)"
                const i64 value = FromString<i64>(lvlSlot->Value.AsString());
                if (const auto denomSlot = frame.FindSlot("denominator")) {
                    // set volume on 1 out of 5 (will be additionally processed later for negative values)
                    const i64 denominator = FromString<i64>(denomSlot->Value.AsString());
                    level = deviceSound.AbsoluteLevelWithDenominator(value, denominator);
                } else {
                    const i64 minVolumePercents = deviceSound.GetCurrent() > 6 && onboardingUsageCounter < 2
                        ? MIN_VOLUME_PERCENTS
                        : deviceSound.GetMax() + 1;
                    if (value > deviceSound.GetMax() && value >= minVolumePercents && value <= MAX_VOLUME_PERCENTS) {
                        // if user says "set to 50" on device with 0-10 volume range, probably he means 50%
                        // or if user says "set to 11" on the same device several times, probably he really means 11%
                        return deviceSound.AbsoluteLevelPercent(value / 100.0);
                    } else {
                        // set new level to unsupported value (will be additionally processed later for negative values)
                        level = value;
                    }
                }
            }

            // If user says "set volume -2", probably they want to make quieter by 2 divisions
            if (level < 0) {
                level = deviceSound.LouderBy(level);
            }

            return level;
        }

        // Like "set volume to max"
        if (lvlSlotType == "custom.volume_setting") {
            const i64 level = deviceSound.AbsoluteLevelDegree(lvlSlot->Value.AsString());
            return level;
        }
    }

    return deviceSound.GetCurrent();
}

TMaybe<i64> CalculateSoundLevelForSetLevelOnOtherDevice(const TFrame& frame) {
    const auto lvlSlot = frame.FindSlot("level");
    if (!lvlSlot) {
        return {};
    }

    if ((IsIn({"sys.num", "fst.num", "custom.number"}, lvlSlot->Type) && lvlSlot->Value.As<i64>() >= 0) ||
        lvlSlot->Type == "custom.volume_setting")
    {
        return CalculateSoundLevelForSetLevel(frame, TDeviceVolume(0));
    }

    return {};
}

std::pair<i64, bool> CalculateSoundLevelForLouderOrQuiter(const TFrame& frame, const TDeviceVolume& deviceSound, bool isLouder, i64 defaultChangeSteps) {
    const i64 sign = isLouder ? 1 : -1;
    const i64 currentSoundLevel = deviceSound.GetCurrent();
    bool isSimpleRelativeRequest = false;

    if (const auto absSlot = frame.FindSlot("absolute_change")) {
        const TString& absSlotType = absSlot->Type;
        // Like "turn it down by 3"
        if (absSlotType == "sys.num" || absSlotType == "custom.number") {
            const i64 delta = FromString<i64>(absSlot->Value.AsString());
            return std::pair(deviceSound.LouderBy(sign * delta), isSimpleRelativeRequest);
        }
        // Like "turn it down by 2.5"
        if (absSlotType == "sys.float") {
            float delta = FromString<float>(absSlot->Value.AsString());
            if (0.0 <= delta && delta < 1.0) {
                // Like "turn it down by 1/2" (meaning on 50%)
                delta *= currentSoundLevel;
            }
            return std::pair(deviceSound.LouderBy(sign * delta), isSimpleRelativeRequest);
        }
    }
    if (const auto relSlot = frame.FindSlot("relative_change")) {
        // Like "make it twice as loud" (if volume was 0 - assume it was 1)
        const float factor = FromString<float>(relSlot->Value.AsString());
        return std::pair(isLouder ? deviceSound.LouderByFactor(factor) : deviceSound.QuiterByFactor(factor), isSimpleRelativeRequest);
    }
    if (const auto percSlot = frame.FindSlot("percentage_change")) {
        // Like "make it louder on 35%"
        const float percent = FromString<float>(percSlot->Value.AsString());
        if (isLouder && currentSoundLevel == 0) {
            // If volume was 0 - assume % from maximal volume
            return std::pair(deviceSound.AbsoluteLevelPercent(percent / 100.0), isSimpleRelativeRequest);
        }
        return std::pair(deviceSound.LouderByPercent(sign * percent / 100.0), isSimpleRelativeRequest);
    }
    if (const auto degreeSlot = frame.FindSlot("degree_change")) {
        // Like "make it a bit quieter" or "make it as quiet as possible"
        const TString degree = degreeSlot->Value.AsString();
        return std::pair(isLouder ? deviceSound.LouderByDegree(degree) : deviceSound.QuiterByDegree(degree), isSimpleRelativeRequest);
    }

    // Simple "make sound quieter"
    isSimpleRelativeRequest = true;
    return std::pair(deviceSound.LouderSteps(sign * defaultChangeSteps), isSimpleRelativeRequest);
}

} // namespace NAlice::NHollywood::NSound
