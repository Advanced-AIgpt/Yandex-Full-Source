#include <alice/hollywood/library/scenarios/video/video_dispatcher.h>

#include <alice/hollywood/library/framework/framework_ut.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywoodFw::NVideo {

Y_UNIT_TEST_SUITE(VideoScenario) {

    Y_UNIT_TEST(RandomNumberDispatcher1) {
        // TODO [DD] This unit test is not working properly with ReturnValueDo()
        /*
        TTestEnvironment testData("video", "ru-ru");
        TDeviceState ds;
        *testData.RunRequest.MutableBaseRequest()->MutableDeviceState() = ds;

        UNIT_ASSERT(testData >> TTestApphost("run/pre_select") >> testData);
        UNIT_ASSERT(testData >> TTestApphost("run/select") >> testData);
        UNIT_ASSERT(testData >> TTestApphost("main") >> testData);

        // TODO [DD] This check is disabled because Dispatch() doesn't return irrevelant
        // UNIT_ASSERT(testData.IsIrrelevant());
        // UNIT_ASSERT(testData.ContainsText(" попозже, пожалуйста."));
        */
    }
} // Y_UNIT_TEST_SUITE

} // namespace NAlice::NHollywoodFw::NRandomNumber
