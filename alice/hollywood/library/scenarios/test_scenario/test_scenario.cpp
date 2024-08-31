/*
    TEST SCENARIO: dumping all available sources
    Usage: see https://a.yandex-team.ru/arcadia/alice/hollywood/library/scenarios/test_scenario/readme.md
*/

#include "test_scenario.h"
#include "test_scenario_scene.h"

#include <alice/hollywood/library/scenarios/test_scenario/nlg/register.h>
#include <alice/hollywood/library/util/service_context.h>

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/json/json.h>
#include <alice/library/scenarios/data_sources/data_sources.h>

#include <alice/megamind/protos/common/device_state.pb.h>
#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/megamind/protos/scenarios/request_meta.pb.h>

#include <util/string/split.h>

namespace NAlice::NHollywoodFw::NTestScenario {

namespace {

template <typename TProto>
void DumpProto(const TRunRequest& runRequest, const TProto* proto, const TString& prefix) {
    if (proto == nullptr) {
        LOG_DEBUG(runRequest.Debug().Logger()) << prefix << ": null";
    } else {
        LOG_DEBUG(runRequest.Debug().Logger()) << prefix << ": " << JsonStringFromProto(*proto);
    }
}

} // anonymous namespace

HW_REGISTER(TTestScenarioScenario);

TTestScenarioScenario::TTestScenarioScenario()
    : TScenario(NProductScenarios::TEST_SCENARIO)
{
    Register(&TTestScenarioScenario::Dispatch);
    RegisterScene<TTestScenarioFakeScene>([this]() {
        RegisterSceneFn(&TTestScenarioFakeScene::Main);
    });
    RegisterRenderer(&TTestScenarioScenario::RenderIrrelevant);

    SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NTestScenario::NNlg::RegisterAll);
    SetApphostGraph(ScenarioRequest() >> NodeRun() >> ScenarioResponse());
}

/*
    TestScenario dispatcher.

    Dump everything and return irrelevant answer
*/
TRetScene TTestScenarioScenario::Dispatch(
        const TRunRequest& runRequest,
        const TStorage& storage,
        const TSource& source) const
{
    Y_UNUSED(storage);

    // Make analytics intent equals frames names sequence
    const auto& semanticFrames = runRequest.Input().GetInputProto().GetSemanticFrames();
    if (!semanticFrames.empty()) {
        TString intent = semanticFrames[0].GetName();
        runRequest.AI().OverrideIntent(intent);
    }

    // Dump all incoming data
    NScenarios::TScenarioRunRequest runRequestProto;
    Y_ENSURE(source.GetSource(NHollywood::REQUEST_ITEM, runRequestProto), "Failed to get main request proto");

    // Get all keys with format test_scenario_logging=blablabla1,blablabla2,...
    TSet<TString> splittedWhat;
    runRequest.Flags().ForEachSubval([&splittedWhat](const TString& str, const TString& subval, const TMaybe<TString>& value) -> bool {
        Y_UNUSED(value);
        if (str == "test_scenario_logging") {
            TVector<TString> splitted;
            Split(subval, ",", splitted);
            for (const auto& it : splitted) {
                splittedWhat.insert(it);
            }
        }
        return true;
    });

    Y_ENSURE(!splittedWhat.empty(), "`test_scenario_logging` experiment is not set");
    for (const auto& it : splittedWhat) {
        // First looking for custom tags
        if (it == "meta") {
            // Dump RequestMeta proto
            DumpProto(runRequest, &runRequest.GetRequestMeta(), "META");
        } else if (it == "base") {
            // Dump Full TScenarioBaseRequest. See also special tags below
            DumpProto(runRequest, &runRequestProto.GetBaseRequest(), "BASE");
        } else if (it == "interfaces") {
            // Dump TInterfaces from TScenarioBaseRequest
            DumpProto(runRequest, &runRequestProto.GetBaseRequest().GetInterfaces(), "INTERFACES");
        } else if (it == "memento") {
            // Dump TInterfaces from TScenarioBaseRequest
            DumpProto(runRequest, &runRequestProto.GetBaseRequest().GetMemento(), "MEMENTO");
        } else if (it == "devicestate") {
            // Dump TInterfaces from TScenarioBaseRequest
            DumpProto(runRequest, &runRequestProto.GetBaseRequest().GetDeviceState(), "DEVICE_STATE");
        } else {
            // All other tags must be a DataSource
            EDataSourceType type = NScenarios::GetDataSourceContextType(it);
            if (type == EDataSourceType::UNDEFINED_DATA_SOURCE) {
                LOG_WARNING(runRequest.Debug().Logger()) << "Undefined tag: " << it;
            } else {
                DumpProto(runRequest, runRequest.GetDataSource(type), it);
            }
        }
    }

    TTestScenarioRenderIrrelevant args;
    args.SetUtterance(runRequest.Input().GetUtterance());
    return TReturnValueRenderIrrelevant(&TTestScenarioScenario::RenderIrrelevant, args);
}

/*
    Irrelevant renderer for TestSenario
    Used to send 'echo' to the answer (te source Utterance string)
*/
TRetResponse TTestScenarioScenario::RenderIrrelevant(
        const TTestScenarioRenderIrrelevant& renderArgs,
        TRender& render)
{
    render.CreateFromNlg("test_scenario", "echo", renderArgs);
    return TReturnValueSuccess();
}

}  // namespace NAlice::NHollywood::NTestScenario
