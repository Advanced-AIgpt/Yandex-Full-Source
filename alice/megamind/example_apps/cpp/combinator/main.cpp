#include <alice/megamind/protos/analytics/combinators/combinator_analytics_info.pb.h>
#include <alice/megamind/protos/scenarios/combinator_request.pb.h>
#include <alice/megamind/protos/scenarios/combinator_response.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <apphost/api/service/cpp/service.h>

using namespace NAppHost;

namespace {

    TString AH_ITEM_COMBINATOR_REQUEST_NAME = "combinator_request_apphost_type";
    TString AH_ITEM_COMBINATOR_RESPONSE_NAME = "combinator_response_apphost_type";

    void CombinatorChooseOne(IServiceContext& ctx) {
        Cerr << "=== CombinatorChooseOne ===" << Endl;
        if (!ctx.HasProtobufItem(AH_ITEM_COMBINATOR_REQUEST_NAME)) {
            Cerr << "No combinator item in apphost context" << Endl;
            return;
        }
        const auto request = ctx.GetOnlyProtobufItem<NAlice::NScenarios::TCombinatorRequest>(AH_ITEM_COMBINATOR_REQUEST_NAME);

        if (request.GetScenarioResponses().empty()) {
            Cerr << "No scenario responses in request to choose" << Endl;
            return;
        }

        NAlice::NScenarios::TCombinatorResponse resp;
        const auto& firstScenario = *request.GetScenarioResponses().begin();
        *resp.AddUsedScenarios() = firstScenario.first;
        *resp.MutableResponse() = firstScenario.second;

        resp.MutableCombinatorsAnalyticsInfo()->SetCombinatorProductName("CombinatorChooseOne");
        ctx.AddProtobufItem(resp, AH_ITEM_COMBINATOR_RESPONSE_NAME);
    }

    void CombinatorEmptyResponse(IServiceContext& ctx) {
        Cerr << "=== CombinatorEmptyResponse ===" << Endl;
        if (!ctx.HasProtobufItem(AH_ITEM_COMBINATOR_REQUEST_NAME)) {
            Cerr << "No combinator item in apphost context" << Endl;
            return;
        }
        const auto request = ctx.GetOnlyProtobufItem<NAlice::NScenarios::TCombinatorRequest>(AH_ITEM_COMBINATOR_REQUEST_NAME);

        NAlice::NScenarios::TCombinatorResponse resp;
        resp.MutableCombinatorsAnalyticsInfo()->SetCombinatorProductName("CombinatorEmptyResponse");
        ctx.AddProtobufItem(resp, AH_ITEM_COMBINATOR_RESPONSE_NAME);
    }

    void CombinatorNoResponse(IServiceContext& ctx) {
        Cerr << "=== CombinatorNoResponse ===" << Endl;
        if (!ctx.HasProtobufItem(AH_ITEM_COMBINATOR_REQUEST_NAME)) {
            Cerr << "No combinator item in apphost context" << Endl;
            return;
        }
    }

} // namespace

int main(int argc, const char** argv) {
    ui16 port = 22334;
    TString combinatorName = "ExampleCombinator";

    if (argc > 1) {
        port = FromString<ui16>(argv[1]);
    }
    if (argc > 2) {
        combinatorName = argv[2];
    }

    TLoop loop;

    loop.EnableGrpc(port, {
        .ReusePort = true,
        .Threads = 1
    });
    Cerr << "Starting combinators on grpc port " << port << Endl;
    Cerr << "Combinator name =  " << combinatorName << Endl;

    loop.Add(port - 1, "/combinator_choose_one", CombinatorChooseOne);
    loop.Add(port - 1, "/combinator_empty_response", CombinatorEmptyResponse);
    loop.Add(port - 1, "/combinator_no_response", CombinatorNoResponse);

    // Start one thread to handle requests.
    loop.Loop(1);

    return 0;
}
