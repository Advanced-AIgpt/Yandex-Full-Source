#include <alice/hollywood/library/framework/framework_ut.h>
#include <alice/hollywood/library/framework/ut/proto/protos_ut.pb.h>
#include <alice/hollywood/library/framework/ut/nlg/register.h>

#include <alice/library/json/json.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywoodFw {

Y_UNIT_TEST_SUITE(HollywoodFrameworkBlackBox) {
    Y_UNIT_TEST(BlackBoxPUID) {
        TTestEnvironment env("dummy_scenario", "ru-ru");
        env.AddPUID("123");
        const TRunRequest& req = env.CreateRunRequest();
        UNIT_ASSERT_STRINGS_EQUAL(req.GetPUID(), "123");
    } // Y_UNIT_TEST(BlackBoxPUID)

} // Y_UNIT_TEST_SUITE(HollywoodFrameworkBlackBox)

} // namespace
