#include <alice/hollywood/library/framework/framework_ut.h>

#include <alice/hollywood/library/framework/core/codegen/gen_directives.pb.h>
#include <alice/hollywood/library/framework/core/codegen/gen_server_directives.pb.h>

#include <alice/megamind/protos/scenarios/directives.pb.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywoodFw {

Y_UNIT_TEST_SUITE(DirectivesCodegen) {
    Y_UNIT_TEST(DirectivesSimple) {
        TDirectivesWrapper wrp;

        // Adding simple directive
        wrp.AddStartBluetoothDirective();

        // Adding complex directive
        NAlice::NScenarios::TCloseDialogDirective d;
        wrp.AddCloseDialogDirective(std::move(d));

        // Make a final response
        NAlice::NScenarios::TScenarioResponseBody response;
        wrp.BuildAnswer(response);

        // Now checking that result response has two directives
        UNIT_ASSERT_EQUAL(response.GetLayout().GetDirectives().size(), 2);
        UNIT_ASSERT(response.GetLayout().GetDirectives()[0].HasStartBluetoothDirective());
        UNIT_ASSERT(response.GetLayout().GetDirectives()[1].HasCloseDialogDirective());
    } // Y_UNIT_TEST(DirectivesSimple)

    Y_UNIT_TEST(ServerDirectives) {
        TServerDirectivesWrapper wrp;

        NAlice::NScenarios::TUpdateDatasyncDirective d;
        d.SetKey("key");
        wrp.AddUpdateDatasyncDirective(std::move(d));

        // Make a final response
        NAlice::NScenarios::TScenarioResponseBody response;
        wrp.BuildAnswer(response);

        // Now checking that result response has two directives
        UNIT_ASSERT_EQUAL(response.GetServerDirectives().size(), 1);
        UNIT_ASSERT(response.GetServerDirectives()[0].HasUpdateDatasyncDirective());
        UNIT_ASSERT_STRINGS_EQUAL(response.GetServerDirectives()[0].GetUpdateDatasyncDirective().GetKey(), "key");
    } // Y_UNIT_TEST(ServerDirectives)
} // Y_UNIT_TEST_SUITE(DirectivesCodegen)

} // namespace
