#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/sound/device_volume.h>
#include <alice/hollywood/library/sound/sound_level_calculation.h>
#include <alice/hollywood/library/sound/sound_common.h>
#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywood::NSound {

TFrame CreateNoSlotFrame(TStringBuf frameName) {
    return TFrame((TString(frameName)));
}

TFrame CreateOneSlotFrame(TStringBuf frameName, const TString& slotName, const TString& slotType, const TString& slotValue) {
    TFrame frame((TString(frameName)));
    frame.AddSlot({slotName, slotType, TSlot::TValue(slotValue)});
    return frame;
}

void AddSlot(TFrame& frame, const TString& slotName, const TString& slotType, const TString& slotValue) {
    frame.AddSlot({slotName, slotType, TSlot::TValue(slotValue)});
}

Y_UNIT_TEST_SUITE(SoundLouder) {
    Y_UNIT_TEST(LouderNoSlots) {
        // Make louder
        TFrame frame = CreateNoSlotFrame(LOUDER_FRAME);

        TDeviceVolume deviceSound(1, 10);
        const auto& newLevelPair = CalculateSoundLevelForLouderOrQuiter(frame, deviceSound, /* isLouder */ true);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.first, 2);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.second, true);
    };

    Y_UNIT_TEST(LouderNoSlotsCustomDefaultChangeSteps) {
        // Make louder
        TFrame frame = CreateNoSlotFrame(LOUDER_FRAME);

        TDeviceVolume deviceSound(1, 10);
        const auto& newLevelPair = CalculateSoundLevelForLouderOrQuiter(frame, deviceSound, /* isLouder */ true, /* defaultChangeSteps */ 3);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.first, 4);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.second, true);
    };

    Y_UNIT_TEST(LouderAbsoluteInt) {
        // Make louder on 3
        TFrame frame = CreateOneSlotFrame(LOUDER_FRAME, "absolute_change", "sys.num", "3");

        TDeviceVolume deviceSound(1, 10);
        const auto& newLevelPair = CalculateSoundLevelForLouderOrQuiter(frame, deviceSound, /* isLouder */ true);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.first, 4);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.second, false);
    };

    Y_UNIT_TEST(LouderAbsoluteOverflow) {
        // Make louder on 20
        TFrame frame = CreateOneSlotFrame(LOUDER_FRAME, "absolute_change", "sys.num", "20");

        TDeviceVolume deviceSound(1, 10);
        const auto& newLevelPair = CalculateSoundLevelForLouderOrQuiter(frame, deviceSound, /* isLouder */ true);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.first, 10);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.second, false);
    };

    Y_UNIT_TEST(LouderAbsoluteCustom) {
        // Make louder on 4
        TFrame frame = CreateOneSlotFrame(LOUDER_FRAME, "absolute_change", "custom.number", "4");

        TDeviceVolume deviceSound(1, 10);
        const auto& newLevelPair = CalculateSoundLevelForLouderOrQuiter(frame, deviceSound, /* isLouder */ true);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.first, 5);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.second, false);
    };

    Y_UNIT_TEST(LouderAbsoluteFloat) {
        // Make louder on 2.7
        TFrame frame = CreateOneSlotFrame(LOUDER_FRAME, "absolute_change", "sys.float", "2.7");

        TDeviceVolume deviceSound(1, 10);
        const auto& newLevelPair = CalculateSoundLevelForLouderOrQuiter(frame, deviceSound, /* isLouder */ true);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.first, 4);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.second, false);
    };

    Y_UNIT_TEST(LouderRelativeFloat) {
        // Make louder on 0.5 (50% louder)
        TFrame frame = CreateOneSlotFrame(LOUDER_FRAME, "absolute_change", "sys.float", "0.5");

        TDeviceVolume deviceSound(4, 10);
        const auto& newLevelPair = CalculateSoundLevelForLouderOrQuiter(frame, deviceSound, /* isLouder */ true);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.first, 6);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.second, false);
    };

    Y_UNIT_TEST(LouderRelative) {
        // Make louder 3 times
        TFrame frame = CreateOneSlotFrame(LOUDER_FRAME, "relative_change", "sys.float", "3");

        TDeviceVolume deviceSound(1, 10);
        const auto& newLevelPair = CalculateSoundLevelForLouderOrQuiter(frame, deviceSound, /* isLouder */ true);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.first, 3);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.second, false);
    };

    Y_UNIT_TEST(LouderRelativeFromZero) {
        // Make louder 5 times (If current level is zero, assume to make sound level 5)
        TFrame frame = CreateOneSlotFrame(LOUDER_FRAME, "relative_change", "sys.float", "5");

        TDeviceVolume deviceSound(0, 10);
        const auto& newLevelPair = CalculateSoundLevelForLouderOrQuiter(frame, deviceSound, /* isLouder */ true);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.first, 5);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.second, false);
    };

    Y_UNIT_TEST(LouderPercentage) {
        // Make louder on 50%
        TFrame frame = CreateOneSlotFrame(LOUDER_FRAME, "percentage_change", "sys.float", "50");

        TDeviceVolume deviceSound(4, 10);
        const auto& newLevelPair = CalculateSoundLevelForLouderOrQuiter(frame, deviceSound, /* isLouder */ true);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.first, 6);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.second, false);
    };

    Y_UNIT_TEST(LouderPercentageFromZero) {
        // Make louder on 50% (If current level is zero, assume to make sound level 50% from maximum)
        TFrame frame = CreateOneSlotFrame(LOUDER_FRAME, "percentage_change", "sys.float", "50");

        TDeviceVolume deviceSound(0, 10);
        const auto& newLevelPair = CalculateSoundLevelForLouderOrQuiter(frame, deviceSound, /* isLouder */ true);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.first, 5);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.second, false);
    };

    Y_UNIT_TEST(LouderDegree) {
        // Make much louder (on 4)
        TFrame frame = CreateOneSlotFrame(LOUDER_FRAME, "degree_change", "custom.degree", "big");

        TDeviceVolume deviceSound(1, 10);
        const auto& newLevelPair = CalculateSoundLevelForLouderOrQuiter(frame, deviceSound, /* isLouder */ true);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.first, 5);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.second, false);
    };
};

Y_UNIT_TEST_SUITE(SoundQuiter) {
    Y_UNIT_TEST(QuiterNoSlots) {
        // Make quiter
        TFrame frame = CreateNoSlotFrame(QUITER_FRAME);

        TDeviceVolume deviceSound(5, 10);
        const auto& newLevelPair = CalculateSoundLevelForLouderOrQuiter(frame, deviceSound, /* isLouder */ false);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.first, 4);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.second, true);
    };

    Y_UNIT_TEST(QuiterNoSlotsCustomDefaultChangeSteps) {
        // Make quiter
        TFrame frame = CreateNoSlotFrame(QUITER_FRAME);

        TDeviceVolume deviceSound(5, 10);
        const auto& newLevelPair = CalculateSoundLevelForLouderOrQuiter(frame, deviceSound, /* isLouder */ false, /* defaultChangeSteps */ 3);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.first, 2);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.second, true);
    };

    Y_UNIT_TEST(QuiterAbsoluteInt) {
        // Make quiter on 3
        TFrame frame = CreateOneSlotFrame(QUITER_FRAME, "absolute_change", "sys.num", "3");

        TDeviceVolume deviceSound(7, 10);
        const auto& newLevelPair = CalculateSoundLevelForLouderOrQuiter(frame, deviceSound, /* isLouder */ false);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.first, 4);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.second, false);
    };

    Y_UNIT_TEST(QuiterAbsoluteOverflow) {
        // Make quiter on 20
        TFrame frame = CreateOneSlotFrame(QUITER_FRAME, "absolute_change", "sys.num", "20");

        TDeviceVolume deviceSound(5, 10);
        const auto& newLevelPair = CalculateSoundLevelForLouderOrQuiter(frame, deviceSound, /* isLouder */ false);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.first, 0);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.second, false);
    };

    Y_UNIT_TEST(QuiterAbsoluteCustom) {
        // Make quiter on 4
        TFrame frame = CreateOneSlotFrame(QUITER_FRAME, "absolute_change", "custom.number", "4");

        TDeviceVolume deviceSound(5, 10);
        const auto& newLevelPair = CalculateSoundLevelForLouderOrQuiter(frame, deviceSound, /* isLouder */ false);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.first, 1);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.second, false);
    };

    Y_UNIT_TEST(QuiterAbsoluteFloat) {
        // Make quiter on 2.7
        TFrame frame = CreateOneSlotFrame(QUITER_FRAME, "absolute_change", "sys.float", "2.7");

        TDeviceVolume deviceSound(5, 10);
        const auto& newLevelPair = CalculateSoundLevelForLouderOrQuiter(frame, deviceSound, /* isLouder */ false);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.first, 2);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.second, false);
    };

    Y_UNIT_TEST(QuiterRelativeFloat) {
        // Make quiter on 0.5 (50% louder)
        TFrame frame = CreateOneSlotFrame(QUITER_FRAME, "absolute_change", "sys.float", "0.5");

        TDeviceVolume deviceSound(4, 10);
        const auto& newLevelPair = CalculateSoundLevelForLouderOrQuiter(frame, deviceSound, /* isLouder */ false);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.first, 2);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.second, false);
    };

    Y_UNIT_TEST(QuiterRelative) {
        // Make quiter 3 times
        TFrame frame = CreateOneSlotFrame(QUITER_FRAME, "relative_change", "sys.float", "3");

        TDeviceVolume deviceSound(6, 10);
        const auto& newLevelPair = CalculateSoundLevelForLouderOrQuiter(frame, deviceSound, /* isLouder */ false);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.first, 2);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.second, false);
    };

    Y_UNIT_TEST(QuiterPercentage) {
        // Make quiter on 50%
        TFrame frame = CreateOneSlotFrame(QUITER_FRAME, "percentage_change", "sys.float", "50");

        TDeviceVolume deviceSound(4, 10);
        const auto& newLevelPair = CalculateSoundLevelForLouderOrQuiter(frame, deviceSound, /* isLouder */ false);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.first, 2);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.second, false);
    };

    Y_UNIT_TEST(QuiterDegree) {
        // Make quiter medium (on 2)
        TFrame frame = CreateOneSlotFrame(QUITER_FRAME, "degree_change", "custom.degree", "medium");

        TDeviceVolume deviceSound(5, 10);
        const auto& newLevelPair = CalculateSoundLevelForLouderOrQuiter(frame, deviceSound, /* isLouder */ false);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.first, 3);
        UNIT_ASSERT_VALUES_EQUAL(newLevelPair.second, false);
    };
};

Y_UNIT_TEST_SUITE(SoundSetLevel) {
    Y_UNIT_TEST(SetLevelNoSlots) {
        // Set level ???
        TFrame frame = CreateNoSlotFrame(SET_LEVEL_FRAME);

        TDeviceVolume deviceSound(5, 10);
        i64 newLevel = CalculateSoundLevelForSetLevel(frame, deviceSound);
        UNIT_ASSERT_VALUES_EQUAL(newLevel, 5);
    };

    Y_UNIT_TEST(SetLevelFloat) {
        // Set level to 3.7
        TFrame frame = CreateOneSlotFrame(SET_LEVEL_FRAME, "level", "sys.float", "3.7");

        TDeviceVolume deviceSound(5, 10);
        i64 newLevel = CalculateSoundLevelForSetLevel(frame, deviceSound);
        UNIT_ASSERT_VALUES_EQUAL(newLevel, 4);
    };

    Y_UNIT_TEST(SetLevelFloatLessOne) {
        // Set level to 0.2 (from maximum)
        TFrame frame = CreateOneSlotFrame(SET_LEVEL_FRAME, "level", "sys.float", "0.2");

        TDeviceVolume deviceSound(5, 10);
        i64 newLevel = CalculateSoundLevelForSetLevel(frame, deviceSound);
        UNIT_ASSERT_VALUES_EQUAL(newLevel, 2);
    };

    Y_UNIT_TEST(SetLevelInt) {
        // Set level to 7
        TFrame frame = CreateOneSlotFrame(SET_LEVEL_FRAME, "level", "sys.num", "7");

        TDeviceVolume deviceSound(5, 10);
        i64 newLevel = CalculateSoundLevelForSetLevel(frame, deviceSound);
        UNIT_ASSERT_VALUES_EQUAL(newLevel, 7);
    };

    Y_UNIT_TEST(SetLevelInPercents) {
        // Set level to 50 (assume 50%)
        TFrame frame = CreateOneSlotFrame(SET_LEVEL_FRAME, "level", "sys.num", "50");

        TDeviceVolume deviceSound(1, 10);
        i64 newLevel = CalculateSoundLevelForSetLevel(frame, deviceSound);
        UNIT_ASSERT_VALUES_EQUAL(newLevel, 5);
    };

    Y_UNIT_TEST(SetNegativeLevel) {
        // Set level to -2 (assume quiter on 2)
        TFrame frame = CreateOneSlotFrame(SET_LEVEL_FRAME, "level", "sys.num", "-2");

        TDeviceVolume deviceSound(5, 10);
        i64 newLevel = CalculateSoundLevelForSetLevel(frame, deviceSound);
        UNIT_ASSERT_VALUES_EQUAL(newLevel, 3);
    };

    Y_UNIT_TEST(SetLevelWithDenominator) {
        // Set level to 2 out of 5
        TFrame frame = CreateOneSlotFrame(SET_LEVEL_FRAME, "level", "sys.num", "2");
        AddSlot(frame, "denominator", "sys.num", "5");

        TDeviceVolume deviceSound(5, 10);
        i64 newLevel = CalculateSoundLevelForSetLevel(frame, deviceSound);
        UNIT_ASSERT_VALUES_EQUAL(newLevel, 4);
    };

    Y_UNIT_TEST(SetLevelWithDenominatorPercents) {
        // Set level to 40%
        TFrame frame = CreateOneSlotFrame(SET_LEVEL_FRAME, "level", "sys.num", "40");
        AddSlot(frame, "denominator", "sys.num", "100");

        TDeviceVolume deviceSound(5, 10);
        i64 newLevel = CalculateSoundLevelForSetLevel(frame, deviceSound);
        UNIT_ASSERT_VALUES_EQUAL(newLevel, 4);
    };

    Y_UNIT_TEST(SetLevelDegree) {
        // Set level to maximum
        TFrame frame = CreateOneSlotFrame(SET_LEVEL_FRAME, "level", "custom.volume_setting", "maximum");

        TDeviceVolume deviceSound(5, 10);
        i64 newLevel = CalculateSoundLevelForSetLevel(frame, deviceSound);
        UNIT_ASSERT_VALUES_EQUAL(newLevel, 10);
    };
};


} // namespace NAlice::NHollywood::NSound
