#include "client_biometry.h"

#include <alice/megamind/protos/guest/guest_options.pb.h>

#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/string.h>

namespace NAlice::NHollywood {

namespace {

const TString UID_SAMPLE = "1234567890";
const TString OAUTH_TOKEN_SAMPLE = "AAAuio";
const TString PERS_ID_SAMPLE = "PersId-123";

} // namespace

Y_UNIT_TEST_SUITE(ValidateGuestOptionsDataSourceSuite) {

Y_UNIT_TEST(ValidMatch) {
    NAlice::TGuestOptions guestOptions;
    guestOptions.SetStatus(NAlice::TGuestOptions::Match);
    guestOptions.SetYandexUID(UID_SAMPLE);
    guestOptions.SetOAuthToken(OAUTH_TOKEN_SAMPLE);
    guestOptions.SetPersId(PERS_ID_SAMPLE);
    guestOptions.SetIsOwnerEnrolled(false);

    UNIT_ASSERT(ValidateGuestOptionsDataSource(TRTLogger::NullLogger(), guestOptions));

    guestOptions.SetIsOwnerEnrolled(true);
    UNIT_ASSERT(ValidateGuestOptionsDataSource(TRTLogger::NullLogger(), guestOptions));
}

Y_UNIT_TEST(ValidNoMatch) {
    NAlice::TGuestOptions guestOptions;
    guestOptions.SetStatus(NAlice::TGuestOptions::NoMatch);
    guestOptions.SetIsOwnerEnrolled(false);
    
    UNIT_ASSERT(ValidateGuestOptionsDataSource(TRTLogger::NullLogger(), guestOptions));

    guestOptions.SetIsOwnerEnrolled(true);
    UNIT_ASSERT(ValidateGuestOptionsDataSource(TRTLogger::NullLogger(), guestOptions));
}

Y_UNIT_TEST(InvalidMatchEmptyUid) {
    NAlice::TGuestOptions guestOptions;
    guestOptions.SetStatus(NAlice::TGuestOptions::Match);
    guestOptions.SetOAuthToken(OAUTH_TOKEN_SAMPLE);
    guestOptions.SetPersId(PERS_ID_SAMPLE);
    guestOptions.SetIsOwnerEnrolled(false);

    UNIT_ASSERT(!ValidateGuestOptionsDataSource(TRTLogger::NullLogger(), guestOptions));

    guestOptions.SetYandexUID("");
    UNIT_ASSERT(!ValidateGuestOptionsDataSource(TRTLogger::NullLogger(), guestOptions));

    guestOptions.SetIsOwnerEnrolled(true);
    UNIT_ASSERT(!ValidateGuestOptionsDataSource(TRTLogger::NullLogger(), guestOptions));

    guestOptions.ClearYandexUID();
    UNIT_ASSERT(!ValidateGuestOptionsDataSource(TRTLogger::NullLogger(), guestOptions));
}

Y_UNIT_TEST(InvalidMatchEmptyOAuthToken) {
    NAlice::TGuestOptions guestOptions;
    guestOptions.SetStatus(NAlice::TGuestOptions::Match);
    guestOptions.SetYandexUID(UID_SAMPLE);
    guestOptions.SetPersId(PERS_ID_SAMPLE);
    guestOptions.SetIsOwnerEnrolled(false);

    UNIT_ASSERT(!ValidateGuestOptionsDataSource(TRTLogger::NullLogger(), guestOptions));

    guestOptions.SetOAuthToken("");
    UNIT_ASSERT(!ValidateGuestOptionsDataSource(TRTLogger::NullLogger(), guestOptions));


    guestOptions.SetIsOwnerEnrolled(true);
    UNIT_ASSERT(!ValidateGuestOptionsDataSource(TRTLogger::NullLogger(), guestOptions));

    guestOptions.ClearOAuthToken();
    UNIT_ASSERT(!ValidateGuestOptionsDataSource(TRTLogger::NullLogger(), guestOptions));
}

Y_UNIT_TEST(InvalidMatchEmptyPersId) {
    NAlice::TGuestOptions guestOptions;
    guestOptions.SetStatus(NAlice::TGuestOptions::Match);
    guestOptions.SetYandexUID(UID_SAMPLE);
    guestOptions.SetOAuthToken(OAUTH_TOKEN_SAMPLE);
    guestOptions.SetIsOwnerEnrolled(false);

    UNIT_ASSERT(!ValidateGuestOptionsDataSource(TRTLogger::NullLogger(), guestOptions));

    guestOptions.SetPersId("");
    UNIT_ASSERT(!ValidateGuestOptionsDataSource(TRTLogger::NullLogger(), guestOptions));

    guestOptions.SetIsOwnerEnrolled(true);
    UNIT_ASSERT(!ValidateGuestOptionsDataSource(TRTLogger::NullLogger(), guestOptions));

    guestOptions.ClearPersId();
    UNIT_ASSERT(!ValidateGuestOptionsDataSource(TRTLogger::NullLogger(), guestOptions));
}

Y_UNIT_TEST(InvalidMatchNoIsOwnerEnrolled) {
    NAlice::TGuestOptions guestOptions;
    guestOptions.SetStatus(NAlice::TGuestOptions::Match);
    guestOptions.SetYandexUID(UID_SAMPLE);
    guestOptions.SetOAuthToken(OAUTH_TOKEN_SAMPLE);
    guestOptions.SetPersId(PERS_ID_SAMPLE);

    UNIT_ASSERT(!ValidateGuestOptionsDataSource(TRTLogger::NullLogger(), guestOptions));
}

Y_UNIT_TEST(InvalidNoMatchNoIsOwnerEnrolled) {
    NAlice::TGuestOptions guestOptions;
    guestOptions.SetStatus(NAlice::TGuestOptions::NoMatch);
    
    UNIT_ASSERT(!ValidateGuestOptionsDataSource(TRTLogger::NullLogger(), guestOptions));
}

}

} // namespace NAlice::NHollywood
