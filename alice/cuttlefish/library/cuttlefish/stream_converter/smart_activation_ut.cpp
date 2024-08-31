#include "smart_activation.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/ymath.h>


using namespace NAlice::NCuttlefish;
using namespace NAlice::NCuttlefish::NAppHostServices;

class TSmartActivationRequestCreationTest: public TTestBase {
    UNIT_TEST_SUITE(TSmartActivationRequestCreationTest);
    UNIT_TEST(TestSimple);
    UNIT_TEST_SUITE_END();

public:
    void TestSimple() {
        TLogContext log(NAlice::NCuttlefish::SpawnLogFrame(), nullptr);

        NAliceProtocol::TRequestContext reqCtx;
        reqCtx.MutableAdditionalOptions()->MutableSpotterFeatures()->MutableVer1()->SetRawAvgRMS(45.5);

        NCachalotProtocol::TActivationAnnouncementRequest req;
        FillSmartActivation(req, reqCtx, "yach_user_228", "X*", "station_2", &log);

        UNIT_ASSERT(req.HasInfo());

        UNIT_ASSERT(req.GetInfo().HasUserId());
        UNIT_ASSERT_VALUES_EQUAL(req.GetInfo().GetUserId(), "yach_user_228");

        UNIT_ASSERT(req.GetInfo().HasDeviceId());
        UNIT_ASSERT_VALUES_EQUAL(req.GetInfo().GetDeviceId(), "X*");

        UNIT_ASSERT(req.GetInfo().HasSpotterFeatures());
        UNIT_ASSERT(req.GetInfo().GetSpotterFeatures().HasValidated());
        UNIT_ASSERT_VALUES_EQUAL(req.GetInfo().GetSpotterFeatures().GetValidated(), false);
        UNIT_ASSERT(req.GetInfo().GetSpotterFeatures().HasAvgRMS());
        UNIT_ASSERT(FuzzyEquals(req.GetInfo().GetSpotterFeatures().GetAvgRMS(), 45.5f * 3.275f));
    }
};

UNIT_TEST_SUITE_REGISTRATION(TSmartActivationRequestCreationTest)
