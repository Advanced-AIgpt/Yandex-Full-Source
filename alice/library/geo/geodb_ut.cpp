#include <alice/library/geo/geodb.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice {

Y_UNIT_TEST_SUITE(GeoUtilParser) {
    Y_UNIT_TEST(SysGeoParser) {
        NGeobase::TId id;

        TGeoUtilStatus status = ParseSysGeo(R"({"city":{"id":None,"name":"Атлантида"}})", id);
        UNIT_ASSERT(status.Defined());
        UNIT_ASSERT_VALUES_EQUAL(status->Code(), EGeoUtilErrorCode::INVALID_DATA);

        status = ParseSysGeo(R"({"city":{"id":12121212121212121212,"name":"Bug"}})", id);
        UNIT_ASSERT(status.Defined());
        UNIT_ASSERT_VALUES_EQUAL(status->Code(), EGeoUtilErrorCode::INVALID_DATA);

        UNIT_ASSERT(ParseSysGeo(R"({"city":{"id":213,"name":"Москва"}})", id).Empty());
        UNIT_ASSERT_VALUES_EQUAL(id, 213);

        UNIT_ASSERT(ParseSysGeo(R"({"country":{"id":225,"name":"Россия"}})", id).Empty());
        UNIT_ASSERT_VALUES_EQUAL(id, 225);

        UNIT_ASSERT(ParseSysGeo(R"({"continent":{"id":111,"name":"Европа"}})", id).Empty());
        UNIT_ASSERT_VALUES_EQUAL(id, 111);

        UNIT_ASSERT(ParseSysGeo(R"({"metro_station":{"id":20365,"name":"Сходненская"}})", id).Empty());
        UNIT_ASSERT_VALUES_EQUAL(id, 20365);
    }

    Y_UNIT_TEST(GeoAddrAddressParser) {
        NGeobase::TId id;

        TGeoUtilStatus status = ParseGeoAddrAddress(R"({})", id);
        UNIT_ASSERT(status.Defined());
        UNIT_ASSERT_VALUES_EQUAL(status->Code(), EGeoUtilErrorCode::INVALID_DATA);

        status = ParseGeoAddrAddress(R"({"BestGeoId":-1,"BestInheritedId":-1,"City":"нигде","PossibleCityId":[],"Province":"нигдейская область"})", id);
        UNIT_ASSERT(status.Defined());
        UNIT_ASSERT_VALUES_EQUAL(status->Code(), EGeoUtilErrorCode::INVALID_DATA);

        UNIT_ASSERT(ParseGeoAddrAddress(R"({"BestGeoId":90,"BestInheritedId":90,"City":"сан-франциско","PossibleCityId":[],"Province":"калифорния"})", id).Empty());
        UNIT_ASSERT_VALUES_EQUAL(id, 90);

        UNIT_ASSERT(ParseGeoAddrAddress(R"({"BestGeoId":121382,"BestInheritedId":121382,"PossibleCityId":[],"Province":"префектура хоккайдо"})", id).Empty());
        UNIT_ASSERT_VALUES_EQUAL(id, 121382);

        UNIT_ASSERT(ParseGeoAddrAddress(R"({"BestGeoId":11309,"BestInheritedId":11309,"PossibleCityId":[],"Province":"красноярский край"})", id).Empty());
        UNIT_ASSERT_VALUES_EQUAL(id, 11309);

        UNIT_ASSERT(ParseGeoAddrAddress(R"({"BestGeoId":-1,"BestInheritedId":213,"Metro":"сходненская","PossibleCityId":[213]})", id).Empty());
        UNIT_ASSERT_VALUES_EQUAL(id, 213);

        UNIT_ASSERT(ParseGeoAddrAddress(R"({"BestGeoId":10068,"BestInheritedId":10068,"Country":"македония","PossibleCityId":[]})", id).Empty());
        UNIT_ASSERT_VALUES_EQUAL(id, 10068);
    }

    Y_UNIT_TEST(GeoIdParser) {
        NGeobase::TId id;

        for (const TString& invalidValue : {"-1", "-2", "-3"}) {
            TGeoUtilStatus status = ParseGeoId(invalidValue, id);
            UNIT_ASSERT(status.Defined());
            UNIT_ASSERT_VALUES_EQUAL(status->Code(), EGeoUtilErrorCode::INVALID_DATA);
            UNIT_ASSERT_VALUES_EQUAL(status->Message(), "Slot is invalid: " + invalidValue);
        }

        for (const TString& errorValue : {"101010101010101010101010101", "YOLO", ""}) {
            TGeoUtilStatus status = ParseGeoId(errorValue, id);
            UNIT_ASSERT(status.Defined());
            UNIT_ASSERT_VALUES_EQUAL(status->Code(), EGeoUtilErrorCode::INVALID_DATA);
        }

        for (const auto& [correctValue, correctId] : {
            std::make_pair("213", 213),
            std::make_pair("10503", 10503),
            std::make_pair("10098", 10098),
        }) {
            TGeoUtilStatus status = ParseGeoId(correctValue, id);
            UNIT_ASSERT(status.Empty());
            UNIT_ASSERT_VALUES_EQUAL(id, correctId);
        }
    }
}

} // namespace NAlice::NHollywood::NWeather
