#include <alice/hollywood/library/scenarios/weather/util/avatars.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywood::NWeather {

Y_UNIT_TEST_SUITE(AvatarsMap) {
    Y_UNIT_TEST(FindScale3) {
        TAvatarsMap avatarsMap{2.5};

        const auto* avatar = avatarsMap.Find("skc_n");
        UNIT_ASSERT(avatar);
        UNIT_ASSERT_VALUES_EQUAL(avatar->Http, "http://avatars.mds.yandex.net/get-bass/397492/weather_60x60_3d8421fa4ebf65116b130d17eac1a61d4f91485e9c0703aa53a6925d6e6876d0.png/orig");
        UNIT_ASSERT_VALUES_EQUAL(avatar->Https, "https://avatars.mds.yandex.net/get-bass/397492/weather_60x60_3d8421fa4ebf65116b130d17eac1a61d4f91485e9c0703aa53a6925d6e6876d0.png/orig");
    }

    Y_UNIT_TEST(FindScale5) {
        TAvatarsMap avatarsMap{5.0};

        const auto* avatar = avatarsMap.Find("big_bkn_-sn_n");
        UNIT_ASSERT(avatar);
        UNIT_ASSERT_VALUES_EQUAL(avatar->Http, "http://avatars.mds.yandex.net/get-bass/397492/weather_80x80_b0718f8d6737fd0b6175e42d43166323f3a3f60177502078119f7bea81ee57be.png/orig");
        UNIT_ASSERT_VALUES_EQUAL(avatar->Https, "https://avatars.mds.yandex.net/get-bass/397492/weather_80x80_b0718f8d6737fd0b6175e42d43166323f3a3f60177502078119f7bea81ee57be.png/orig");
    }

    Y_UNIT_TEST(FindWrong) {
        TAvatarsMap avatarsMap{3.0};

        const auto* avatar = avatarsMap.Find("wroooooon_id");
        UNIT_ASSERT(!avatar);
    }
}

} // namespace NAlice::NHollywood::NWeather
