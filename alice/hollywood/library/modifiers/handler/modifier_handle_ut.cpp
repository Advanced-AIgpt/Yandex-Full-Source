#include "modifier_apply_handle.h"

#include <alice/hollywood/library/modifiers/testing/fake_modifier.h>
#include <alice/hollywood/library/modifiers/testing/mock_modifier_context.h>

#include <alice/library/proto/proto.h>
#include <alice/library/unittest/mock_sensors.h>

#include <library/cpp/testing/common/env.h>
#include <library/cpp/testing/gmock_in_unittest/gmock.h>
#include <library/cpp/testing/unittest/registar.h>

#include <apphost/lib/service_testing/service_testing.h>

#include <util/charset/utf8.h>
#include <util/generic/string.h>
#include <util/stream/file.h>

namespace {

using namespace testing;
using namespace NAlice;
using namespace NAlice::NHollywood;
using namespace NAlice::NHollywood::NModifiers;
using namespace NAlice::NHollywood::NModifiers::NImpl;

Y_UNIT_TEST_SUITE(ModifierHandler) {
    Y_UNIT_TEST(WriteMetrics) {
        NMegamind::TModifierRequest request = NMegamind::TModifierRequest::default_instance();
        TMockModifierContext ctx;

        StrictMock<TMockSensors> sensors;
        EXPECT_CALL(ctx, Sensors).WillRepeatedly(ReturnRef(sensors));
        EXPECT_CALL(ctx, Logger).WillRepeatedly(ReturnRef(TRTLogger::NullLogger()));

        EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{{"name", "modifiers_wins_per_second"},
                                                          {"modifier_name", "modifier2"}}));

        TVector<TBaseModifierPtr> modifiers(Reserve(3));
        modifiers.push_back(std::make_unique<TFakeModifier>(/* modifierType = */ "modifier1",
                                                            /* applyResult= */ Nothing(), /* enabled= */ false));
        modifiers.push_back(std::make_unique<TFakeModifier>(/* modifierType = */ "modifier2",
                                                            /* applyResult= */ Nothing(), /* enabled= */ true));
        modifiers.push_back(std::make_unique<TFakeModifier>(
            /* modifierType= */ "modifier3", /* applyResult= */ TNonApply{TNonApply::EType::NotApplicable},
            /* enabled= */ true));

        ApplyImpl(NAppHost::NService::TTestContext(), request, ctx, modifiers);
    }
}

} // namespace
