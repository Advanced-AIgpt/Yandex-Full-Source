#include "blackbox.h"

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NMegamind {

Y_UNIT_TEST_SUITE(TestBlackbox) {
    Y_UNIT_TEST(TestCreateBlackBoxData) {
        TBlackBoxFullUserInfoProto response;
        response.MutableUserInfo()->SetUid("123456");
        response.MutableUserInfo()->SetEmail("admin@yandex.ru");
        response.MutableUserInfo()->SetFirstName("Arkady");
        response.MutableUserInfo()->SetLastName("Volozh");
        response.MutableUserInfo()->SetPhone("+79160000000");
        response.MutableUserInfo()->SetIsStaff(true);
        response.MutableUserInfo()->SetHasYandexPlus(true);
        response.MutableUserInfo()->SetIsBetaTester(true);
        response.MutableUserInfo()->SetMusicSubscriptionRegionId(225u);

        const auto userInfo = CreateBlackBoxData(response);
        UNIT_ASSERT(userInfo.IsInitialized());
        UNIT_ASSERT_VALUES_EQUAL(CreateBlackBoxData(response).GetUid(), "123456");
        UNIT_ASSERT_VALUES_EQUAL(CreateBlackBoxData(response).GetEmail(), "admin@yandex.ru");
        UNIT_ASSERT_VALUES_EQUAL(CreateBlackBoxData(response).GetFirstName(), "Arkady");
        UNIT_ASSERT_VALUES_EQUAL(CreateBlackBoxData(response).GetLastName(), "Volozh");
        UNIT_ASSERT_VALUES_EQUAL(CreateBlackBoxData(response).GetPhone(), "+79160000000");
        UNIT_ASSERT(CreateBlackBoxData(response).GetIsStaff());
        UNIT_ASSERT(CreateBlackBoxData(response).GetHasYandexPlus());
        UNIT_ASSERT(CreateBlackBoxData(response).GetIsBetaTester());
        UNIT_ASSERT_VALUES_EQUAL(CreateBlackBoxData(response).GetMusicSubscriptionRegionId(), 225u);
    }
}

} // namespace NAlice::NMegamind
