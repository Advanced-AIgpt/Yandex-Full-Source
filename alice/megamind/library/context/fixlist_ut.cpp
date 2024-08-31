#include <alice/megamind/library/context/fixlist.h>

#include <alice/library/proto/proto.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/maybe.h>

Y_UNIT_TEST_SUITE_F(Fixlist, NUnitTest::TBaseFixture) {
    Y_UNIT_TEST(NoBegemotResponse) {
        NAlice::TFixlist fixlist;
        UNIT_ASSERT(!fixlist.IsRequestAllowedForScenario(TStringBuf("any_scenario")));

        fixlist = NAlice::TFixlist(::NBg::NProto::TAliceFixlistResult());
        UNIT_ASSERT(!fixlist.IsRequestAllowedForScenario(TStringBuf("any_scenario")));
    }

    Y_UNIT_TEST(UnknownMatch) {
        const auto aliceFixlistResult = NAlice::ParseProtoText<::NBg::NProto::TAliceFixlistResult>(R"(
Matches: {
    key: "some_scenario"
    value { }
}
)");
        NAlice::TFixlist fixlist(aliceFixlistResult);
        UNIT_ASSERT(fixlist.IsRequestAllowedForScenario(TStringBuf("any_scenario")));
    }

    Y_UNIT_TEST(RequestAllowedForScenario) {
        const auto aliceFixlistResult = NAlice::ParseProtoText<::NBg::NProto::TAliceFixlistResult>(R"(
Matches: {
    key: "general_fixlist"
    value {
        Intents: ["alice.vinsless.general_conversation"]
    }
}
)");
        NAlice::TFixlist fixlist(aliceFixlistResult);

        UNIT_ASSERT(fixlist.IsRequestAllowedForScenario(TStringBuf("alice.vinsless.general_conversation")));
        UNIT_ASSERT(!fixlist.IsRequestAllowedForScenario(TStringBuf("any_other_scenario")));
    }

}
