#include "saas.h"
#include "ut_helper.h"

#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/apphost_request/protos/client.pb.h>
#include <alice/megamind/library/apphost_request/util.h>

#include <library/cpp/testing/unittest/registar.h>
#include <search/idl/meta.pb.h>

namespace {

using namespace NAlice;
using namespace NAlice::NMegamind;
using namespace NAlice::NMegamind::NTesting;

Y_UNIT_TEST_SUITE_F(AppHostMegamindSaasSkillDiscovery, TAppHostFixture) {
    Y_UNIT_TEST(SetupNotRequested) {
        auto ahCtx = CreateAppHostContext();

        {
            TString utterance;
            auto status = AppHostSaasSkillDiscoverySetup(ahCtx, utterance);
            UNIT_ASSERT_C(status.Defined(), "event with text_input but without text");
        }
    }

    Y_UNIT_TEST(SetupSuccess) {
        auto ahCtx = CreateAppHostContext();

        TString utterance = "hello";
        auto status = AppHostSaasSkillDiscoverySetup(ahCtx, utterance);
        UNIT_ASSERT(!status.Defined());
        NAppHostHttp::THttpRequest httpRequest;
        UNIT_ASSERT(ahCtx.TestCtx().GetOnlyProtobufItem(AH_ITEM_SAAS_SKILL_DISCOVERY_HTTP_REQUEST_NAME).Fill(&httpRequest));
        UNIT_ASSERT(!httpRequest.GetPath().Empty());
    }

    Y_UNIT_TEST(PostSetup) {
        auto ahCtx = CreateAppHostContext();

        {
            NAppHostHttp::THttpResponse response;
            NMetaProtocol::TReport report;
            auto& grouping = *report.AddGrouping();
            for (int i = 1; i <= 10; ++i) {
                auto& group = *grouping.AddGroup();
                auto& document = *group.AddDocument();
                document.SetRelevance(i);
                document.MutableArchiveInfo()->SetUrl(ToString(i));
            }
            TString content;
            Y_PROTOBUF_SUPPRESS_NODISCARD report.SerializeToString(&content);
            response.SetContent(content);
            response.SetStatusCode(200);
            ahCtx.TestCtx().AddProtobufItem(response, AH_ITEM_SAAS_SKILL_DISCOVERY_HTTP_RESPONSE_NAME, NAppHost::EContextItemKind::Input);
        }

        NScenarios::TSkillDiscoverySaasCandidates result;
        auto err = AppHostSaasSkillDiscoveryPostSetup(ahCtx, result);
        UNIT_ASSERT(!err.Defined());
        UNIT_ASSERT_VALUES_EQUAL(result.SaasCandidateSize(), 10);
    }
}

} // namespace
