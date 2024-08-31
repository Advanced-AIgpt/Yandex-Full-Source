#include <alice/hollywood/library/scenarios/weather/handles/prepare_city_handle.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywood::NWeather {

Y_UNIT_TEST_SUITE(PrepareCityHandle) {
    Y_UNIT_TEST(CityStates) {
        TVector<std::pair<NGeobase::TId, NGeobase::TId>> fixlist = {
            {10024, 10143}, // Тунис
            {10070, 10465}, // Монако
            {10089, 10525}, // Гибралтар
            {10105, 10619}, // Сингапур
            {20790, 20796}, // Сан-Марино
            {20826, 20828}, // Алжир
            {20968, 21415}, // Гватемала
            {21203, 21204}, // Люксембург
            {21299, 21300}, // Панама
            {21359, 21946}, // Ватикан
            {21475, 21476}, // Джибути
        };

        TVector<NGeobase::TId> nonFixlist = {
            10067, // Лихтенштейн
            10088, // Андорра
            21199, // Сан-Томе и Принсипи
            20769, // Сальвадор
            225, // Россия
        };

        for (const auto& [countryId, cityId] : fixlist) {
            UNIT_ASSERT_VALUES_EQUAL(cityId, FixGeoIdIfCityState(countryId));
        }

        for (const auto countryId : nonFixlist) {
            UNIT_ASSERT_VALUES_EQUAL(countryId, FixGeoIdIfCityState(countryId));
        }
    }
}

} // namespace NAlice::NHollywood::NWeather
