#include <library/cpp/testing/benchmark/bench.h>
#include <library/cpp/resource/resource.h>
#include <library/cpp/json/json_reader.h>

#include <util/stream/file.h>
#include <util/generic/guid.h>
#include <util/folder/path.h>

#include <alice/cuttlefish/library/experiments/experiments.h>
#include <alice/cuttlefish/library/experiments/session_context_proxy.h>
#include <alice/cuttlefish/library/experiments/utils.h>

using namespace NVoice::NExperiments;

namespace {



NJson::TJsonValue ReadJsonFromResource(const TStringBuf resourceName)
{
    NJson::TJsonValue val;
    NJson::ReadJsonTree(NResource::Find(resourceName), &val, true);
    return val;
}

NAliceProtocol::TSessionContext CreateSessionContext(const NJson::TJsonValue& event)
{
    NAliceProtocol::TSessionContext ctx;
    if (const TString* val = GetValueByPath<TString>(event, {"payload", "lang"}))
        SetLang(ctx, *val);
    if (const TString* val = GetValueByPath<TString>(event, {"payload", "vinsUrl"}))
        SetVinsUrl(ctx, *val);
    if (const TString* val = GetValueByPath<TString>(event, {"payload", "vins", "application", "app_id"}))
        SetAppId(ctx, *val);
    return ctx;
}

struct TContext {
    TContext()
        : Experiments(ReadJsonFromResource("/experiments.json"), ReadJsonFromResource("/macros.json"))
        , BigEvent(ReadJsonFromResource("/event_big.json"))
        , SmallEvent(ReadJsonFromResource("/event_small.json"))
        , SynchronizeStateEvent(ReadJsonFromResource("/event_synchronize_state.json"))
        , SessionContext(CreateSessionContext(SynchronizeStateEvent))
        , Patcher(Experiments.CreatePatcherForSession(SessionContext, SynchronizeStateEvent))
    { }

    TExperiments Experiments;
    NJson::TJsonValue BigEvent;
    NJson::TJsonValue SmallEvent;
    NJson::TJsonValue SynchronizeStateEvent;
    NAliceProtocol::TSessionContext SessionContext;
    TEventPatcher Patcher;

};

const TContext CONTEXT;

} // anonymous namespace


Y_CPU_BENCHMARK(CreateEventPatcher, iface) {
    for (size_t i = 0; i < iface.Iterations(); ++i) {
        Y_DO_NOT_OPTIMIZE_AWAY(
            CONTEXT.Experiments.CreatePatcherForSession(CONTEXT.SessionContext, CONTEXT.SynchronizeStateEvent)
        );
    }
}

Y_CPU_BENCHMARK(PatchBigEvent, iface) {
    NJson::TJsonValue event(CONTEXT.BigEvent);
    for (size_t i = 0; i < iface.Iterations(); ++i) {
        CONTEXT.Patcher.Patch(event, CONTEXT.SessionContext);
    }
}

Y_CPU_BENCHMARK(PatchSmallEvent, iface) {
    NJson::TJsonValue event(CONTEXT.SmallEvent);
    for (size_t i = 0; i < iface.Iterations(); ++i) {
        CONTEXT.Patcher.Patch(event, CONTEXT.SessionContext);
    }
}
