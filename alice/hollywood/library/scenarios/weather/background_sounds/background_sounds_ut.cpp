#include <alice/hollywood/library/scenarios/weather/background_sounds/background_sounds.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywood::NWeather {

using ETemperatureCondition = TBackgroundSounds::ETemperatureCondition;
using ESeason = TBackgroundSounds::ESeason;
using EDayTime = TBackgroundSounds::EDayTime;

Y_UNIT_TEST_SUITE(BackgroundSoundsSuite) {
    Y_UNIT_TEST(TestEmpty) {
        TRng rng(13071999);
        UNIT_ASSERT(TBackgroundSounds(rng).TryCalculateBackgroundFilename().Empty());
        UNIT_ASSERT(TBackgroundSounds(rng).SetWeatherCondition("dummy").TryCalculateBackgroundFilename().Empty());
    }

    Y_UNIT_TEST(TestUsualTemperature) {
        TRng rng(13071999);
        for (const double t : {-9.0, 0.0, 29.0}) {
            const auto bgs = TBackgroundSounds(rng)
                .SetWeatherCondition("dummy")
                .SetTemperature(t)
                .TryCalculateBackgroundFilename();
            UNIT_ASSERT(bgs.Empty());
        }
    }

    Y_UNIT_TEST(TestColdTemperature) {
        TRng rng(13071999);
        for (const double t : {-11.0, -10000.0, -1232323232.0}) {
            const auto bgs = TBackgroundSounds(rng)
                .SetWeatherCondition("dummy")
                .SetTemperature(t)
                .TryCalculateBackgroundFilename();
            UNIT_ASSERT(bgs.Defined());
            UNIT_ASSERT_VALUES_EQUAL(*bgs, "weather_backgrounds/very_cold.pcm");
        }
    }

    Y_UNIT_TEST(TestHotTemperature) {
        TRng rng(13071999);
        for (const double t : {+31.0, +10000.0, +1232323232.0}) {
            const auto bgs = TBackgroundSounds(rng)
                .SetWeatherCondition("dummy")
                .SetTemperature(t)
                .TryCalculateBackgroundFilename();
            UNIT_ASSERT(bgs.Defined());
            UNIT_ASSERT_VALUES_EQUAL(*bgs, "weather_backgrounds/very_hot.pcm");
        }
    }

    Y_UNIT_TEST(TestSimpleDay) {
        TRng rng(13071999);
        const auto bgs = TBackgroundSounds(rng)
            .SetWeatherCondition("dummy")
            .SetSeason(ESeason::Autumn)
            .SetDayTime(EDayTime::Day)
            .TryCalculateBackgroundFilename();
        UNIT_ASSERT(bgs.Empty());
    }

    Y_UNIT_TEST(TestCoolDay) {
        TRng rng(13071999);
        const auto bgs = TBackgroundSounds(rng)
            .SetWeatherCondition("dummy")
            .SetSeason(ESeason::Summer)
            .SetDayTime(EDayTime::Night)
            .TryCalculateBackgroundFilename();
        UNIT_ASSERT(bgs.Defined());
        UNIT_ASSERT_VALUES_EQUAL(*bgs, "weather_backgrounds/summer_night.pcm");
    }

    Y_UNIT_TEST(TestWeatherCondition) {
        TRng rng(13071999);
        const auto bgs = TBackgroundSounds(rng)
            .SetWeatherCondition("moderate-rain")
            .TryCalculateBackgroundFilename();
        UNIT_ASSERT(bgs.Defined());
        UNIT_ASSERT_VALUES_EQUAL(*bgs, "weather_backgrounds/light_rain.pcm");
    }

    Y_UNIT_TEST(TestManyVariants) {
        TRng rng(13071999);
        THashMap<TString, size_t> cnt;
        for (int i = 0; i < 10000; ++i) {
            const auto bgs = TBackgroundSounds(rng)
                .SetWeatherCondition("thunderstorm-wet-snow") // +2 variants
                .SetTemperature(-20.0) // +1 variant
                .SetSeason(ESeason::Winter).SetDayTime(EDayTime::Night) // +1 variant
                .TryCalculateBackgroundFilename();
            UNIT_ASSERT(bgs.Defined());
            ++cnt[*bgs];
        }

        UNIT_ASSERT_VALUES_EQUAL(cnt.size(), 4);
        UNIT_ASSERT_VALUES_EQUAL(cnt["weather_backgrounds/thunderstorm_with_rain.pcm"], 2513);
        UNIT_ASSERT_VALUES_EQUAL(cnt["weather_backgrounds/wet_snow.pcm"], 2527);
        UNIT_ASSERT_VALUES_EQUAL(cnt["weather_backgrounds/winter_night.pcm"], 2459);
        UNIT_ASSERT_VALUES_EQUAL(cnt["weather_backgrounds/very_cold.pcm"], 2501);
    }
}

} // namespace NAlice::NHollywood::NWeather
