#include <alice/hollywood/library/personal_data/personal_data.h>
#include <alice/library/client/client_info.h>
#include <alice/library/client/protos/client_info.pb.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywood {

namespace {

inline const TString PERS_ID = "PersId-87650091-675aa30e-24ed12c6-4c75586b";

} // namespace

Y_UNIT_TEST_SUITE(ToPersonalDataKeyTestSuite) {

Y_UNIT_TEST(ToPersonalDataKeyUserNameTest) {
    TClientInfoProto proto;
    TClientInfo clientInfo{proto};
    auto key = ToPersonalDataKey(clientInfo, NAlice::NDataSync::EUserSpecificKey::UserName);
    UNIT_ASSERT_STRINGS_EQUAL(key, "/v1/personality/profile/alisa/kv/user_name");
}

Y_UNIT_TEST(ToPersonalDataKeyGuestUidTest) {
    TClientInfoProto proto;
    TClientInfo clientInfo{proto};
    auto key = ToPersonalDataKey(clientInfo, NAlice::NDataSync::EUserSpecificKey::GuestUID);
    UNIT_ASSERT_STRINGS_EQUAL(key, "/v1/personality/profile/alisa/kv/guest_uid");
}

Y_UNIT_TEST(ToPersonalDataKeyLocationTest) {
    TClientInfoProto proto;
    proto.SetDeviceModel("foobar");
    proto.SetDeviceId("1234567890");
    TClientInfo clientInfo{proto};
    auto key = ToPersonalDataKey(clientInfo, NAlice::NDataSync::EUserDeviceSpecificKey::Location);
    UNIT_ASSERT_STRINGS_EQUAL(key, "/v1/personality/profile/alisa/kv/foobar_1234567890_location");
}

Y_UNIT_TEST(ToPersonalDataKeyEnrollmentUserNameTest) {
    TClientInfoProto proto;
    TClientInfo clientInfo{proto};
    auto key = ToPersonalDataKey(clientInfo, NAlice::NDataSync::EEnrollmentSpecificKey::UserName, PERS_ID);
    UNIT_ASSERT_STRINGS_EQUAL(key, "/v1/personality/profile/alisa/kv/enrollment__PersId-87650091-675aa30e-24ed12c6-4c75586b__user_name");
}

Y_UNIT_TEST(ToPersonalDataKeyEnrollmentGenderTest) {
    TClientInfoProto proto;
    TClientInfo clientInfo{proto};
    auto key = ToPersonalDataKey(clientInfo, NAlice::NDataSync::EEnrollmentSpecificKey::Gender, PERS_ID);
    UNIT_ASSERT_STRINGS_EQUAL(key, "/v1/personality/profile/alisa/kv/enrollment__PersId-87650091-675aa30e-24ed12c6-4c75586b__gender");
}

Y_UNIT_TEST(ToPersonalDataKeyEnrollmentSpecificKeyPersIdRequired) {
    TClientInfoProto proto;
    TClientInfo clientInfo{proto};
    UNIT_ASSERT_EXCEPTION_C(ToPersonalDataKey(clientInfo, NAlice::NDataSync::EEnrollmentSpecificKey::UserName),
                            yexception,
                            "Non-empty persId for enrollment-specific DataSync key is expected");
}

}

} // namespace NAlice::NHollywood
