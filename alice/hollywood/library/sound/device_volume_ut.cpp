#include <alice/hollywood/library/sound/device_volume.h>
#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywood::NSound {

Y_UNIT_TEST_SUITE(DeviceVolume) {
    Y_UNIT_TEST(VolumeMathKolonka) {
        const TDeviceVolume volume(1, 10);
        UNIT_ASSERT_VALUES_EQUAL(volume.GetCurrent(), 1);
        UNIT_ASSERT_VALUES_EQUAL(volume.GetMax(), 10);
        UNIT_ASSERT_VALUES_EQUAL(volume.GetMin(), 0);
        UNIT_ASSERT_VALUES_EQUAL(volume.AbsoluteLevelFloat(8.5f), 9);
        UNIT_ASSERT_VALUES_EQUAL(volume.AbsoluteLevelPercent(0.3), 3);

        UNIT_ASSERT_VALUES_EQUAL(volume.AbsoluteLevelWithDenominator(2, 10), 2);
        UNIT_ASSERT_VALUES_EQUAL(volume.AbsoluteLevelWithDenominator(2, 7), 3);
        UNIT_ASSERT_VALUES_EQUAL(volume.AbsoluteLevelWithDenominator(2, 0), 2);

        UNIT_ASSERT_VALUES_EQUAL(volume.AbsoluteLevelDegree("minimum"), 1);
        UNIT_ASSERT_VALUES_EQUAL(volume.AbsoluteLevelDegree("middle"), 4);
        UNIT_ASSERT_VALUES_EQUAL(volume.AbsoluteLevelDegree("hey"), 1); // If incorrect degree, volume remains the same

        UNIT_ASSERT_VALUES_EQUAL(volume.LouderOneStep(+1), 2);
        UNIT_ASSERT_VALUES_EQUAL(volume.LouderOneStep(-1), 0);
        UNIT_ASSERT_VALUES_EQUAL(volume.LouderOneStep(100), 2);
        UNIT_ASSERT_VALUES_EQUAL(volume.LouderSteps(5), 6);
        UNIT_ASSERT_VALUES_EQUAL(volume.LouderSteps(-100), 0);

        UNIT_ASSERT_VALUES_EQUAL(volume.LouderBy((i64)5), 6);
        UNIT_ASSERT_VALUES_EQUAL(volume.LouderBy((i64)100), 10);
        UNIT_ASSERT_VALUES_EQUAL(volume.LouderBy((i64)-100), 0);
        UNIT_ASSERT_VALUES_EQUAL(volume.LouderBy(1.5f), 3);

        UNIT_ASSERT_VALUES_EQUAL(volume.LouderByFactor(3.0f), 3);
        UNIT_ASSERT_VALUES_EQUAL(volume.LouderByFactor(3.5f), 4);
        UNIT_ASSERT_VALUES_EQUAL(volume.LouderByFactor(0), 0);
        UNIT_ASSERT_VALUES_EQUAL(volume.QuiterByFactor(4.0), 0);
        UNIT_ASSERT_VALUES_EQUAL(volume.QuiterByFactor(0), 1);

        UNIT_ASSERT_VALUES_EQUAL(volume.LouderByDegree("small"), 2);
        UNIT_ASSERT_VALUES_EQUAL(volume.LouderByDegree("big"), 5);
        UNIT_ASSERT_VALUES_EQUAL(volume.LouderByDegree("unknown"), 2);  // one step louder

        // upper bound check
        const TDeviceVolume volume2(10, 10);
        UNIT_ASSERT_VALUES_EQUAL(volume2.LouderOneStep(+1), 10);
        UNIT_ASSERT_VALUES_EQUAL(volume2.LouderSteps(2), 10);
        UNIT_ASSERT_VALUES_EQUAL(volume2.LouderBy((i64)5), 10);

        // do not mute check
        UNIT_ASSERT_VALUES_EQUAL(volume2.LouderSteps(-20), 1);

        // sound "twice louder" from zero means "twise louder than one"
        const TDeviceVolume volume3(0, 10);
        UNIT_ASSERT_VALUES_EQUAL(volume3.LouderByFactor(2), 2);

        // louder/quiter by percent
        const TDeviceVolume volume4(4, 10);
        UNIT_ASSERT_VALUES_EQUAL(volume4.LouderByPercent(0.5), 6);
        UNIT_ASSERT_VALUES_EQUAL(volume4.LouderByPercent(-0.5), 2);
        UNIT_ASSERT_VALUES_EQUAL(volume4.LouderByPercent(1.0), 8);
        UNIT_ASSERT_VALUES_EQUAL(volume4.QuiterByDegree("small"), 3);
        UNIT_ASSERT_VALUES_EQUAL(volume4.LouderByPercent(0.01), 5);
        UNIT_ASSERT_VALUES_EQUAL(volume4.LouderByPercent(-0.01), 3);

        // quiter by relative change never mutes sound, if current level > 1
        const TDeviceVolume volume5(2, 10);
        UNIT_ASSERT_VALUES_EQUAL(volume5.QuiterByDegree("big"), 1);
        UNIT_ASSERT_VALUES_EQUAL(volume5.LouderByPercent(-0.90), 1);
        UNIT_ASSERT_VALUES_EQUAL(volume5.QuiterByFactor(6), 1);

        // quiter mutes sound if level == 1
        const TDeviceVolume volume6(1, 10);
        UNIT_ASSERT_VALUES_EQUAL(volume6.QuiterByDegree("big"), 0);
        UNIT_ASSERT_VALUES_EQUAL(volume6.LouderByPercent(-0.03), 0);
        UNIT_ASSERT_VALUES_EQUAL(volume6.QuiterByFactor(6), 0);
    };

    Y_UNIT_TEST(VolumeMathTV) {
        const TDeviceVolume volume(25, 100);
        UNIT_ASSERT_VALUES_EQUAL(volume.LouderOneStep(1), 30);
        UNIT_ASSERT_VALUES_EQUAL(volume.LouderOneStep(-1), 20);
        UNIT_ASSERT_VALUES_EQUAL(volume.LouderSteps(2), 35);
        UNIT_ASSERT_VALUES_EQUAL(volume.LouderSteps(-2), 15);
        UNIT_ASSERT_VALUES_EQUAL(volume.LouderSteps(-100), 1);
        UNIT_ASSERT_VALUES_EQUAL(volume.LouderSteps(300), 100);

        UNIT_ASSERT_VALUES_EQUAL(volume.LouderByDegree("small"), 30);
        UNIT_ASSERT_VALUES_EQUAL(volume.QuiterByDegree("small"), 20);
        UNIT_ASSERT_VALUES_EQUAL(volume.LouderByDegree("big"), 45);
    };
};

} // namespace NAlice::NHollywood::NSound
