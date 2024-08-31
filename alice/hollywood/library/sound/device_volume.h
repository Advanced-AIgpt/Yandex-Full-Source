#pragma once

#include <util/generic/fwd.h>


namespace NAlice::NHollywood::NSound {

class TDeviceVolume {
public:
    TDeviceVolume(i64 current, i64 max = 10, i64 step = 0);

    static TDeviceVolume BuildFromState(const auto& deviceState);

    /**
     * Simple current state getters
     */
    i64 GetCurrent() const;
    i64 GetMax() const;
    i64 GetMin() const;
    i64 IsMaximum() const;
    i64 IsMinimum() const;

    /**
     * Is new volume level in client supported bounds
     */
    i64 IsSupported(i64 level) const;

    /**
     * Converters for absolute volume levels (including human speakable) to numeric value for current client bounds
     */
    i64 AbsoluteLevelDegree(TStringBuf key) const;
    static i64 AbsoluteLevelFloat(float level);
    i64 AbsoluteLevelPercent(float percent) const;
    i64 AbsoluteLevelWithDenominator(i64 level, i64 denominator) const;

    /**
     * Converters for relative quiter/louder commands to numeric value according to current client bounds and step
     */
    i64 LouderOneStep(i64 sign) const;
    i64 LouderSteps(i64 steps) const;
    i64 LouderBy(i64 delta) const;
    i64 LouderBy(float delta) const;
    i64 LouderByFactor(float factor) const;
    i64 QuiterByFactor(float factor) const;
    i64 LouderByPercent(float percent) const;
    i64 LouderByDegree(TStringBuf key, i64 sign = 1) const;
    i64 QuiterByDegree(TStringBuf key) const;

private:
    i64 FitInRange(i64 level, bool doNotMute = false) const;

private:
    i64 Current = -1;
    i64 Max = 10;
    i64 Min = 0;
    i64 Step = 1;  // step of volume change
};

TDeviceVolume TDeviceVolume::BuildFromState(const auto& deviceState) {
    if (deviceState.HasSoundMaxLevel()) {
        return TDeviceVolume(deviceState.GetSoundLevel(), deviceState.GetSoundMaxLevel());
    }
    return TDeviceVolume(deviceState.GetSoundLevel());
};

} // namespace NAlice::NHollywood::NSound
