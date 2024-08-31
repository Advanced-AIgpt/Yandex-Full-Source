#pragma once

#include <alice/megamind/library/analytics/analytics_info.h>
#include <alice/megamind/library/context/context.h>
#include <alice/megamind/library/request/request.h>
#include <alice/megamind/library/scenarios/features/features.h>
#include <alice/megamind/library/session/protos/state.pb.h>
#include <alice/megamind/protos/common/frame.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <util/generic/array_ref.h>
#include <util/generic/scope.h>
#include <util/generic/yexception.h>

#include <functional>
#include <type_traits>

namespace NAlice {

template <typename TStateProto>
struct TTypedLightScenarioEnv {
    TTypedLightScenarioEnv(const IContext& ctx, const TRequest& request,
                           TArrayRef<const TSemanticFrame> requestFrames, TStateProto& state,
                           NMegamind::TAnalyticsInfoBuilder& analyticsInfoBuilder,
                           NMegamind::TUserInfoBuilder& userInfoBuilder)
        : Ctx{ctx}
        , Request{request}
        , RequestFrames{requestFrames}
        , State{state}
        , AnalyticsInfoBuilder{analyticsInfoBuilder}
        , UserInfoBuilder{userInfoBuilder} {
    }

    const IContext& Ctx;
    const TRequest& Request;
    TArrayRef<const TSemanticFrame> RequestFrames;
    TStateProto& State;
    NMegamind::TAnalyticsInfoBuilder& AnalyticsInfoBuilder;
    NMegamind::TUserInfoBuilder& UserInfoBuilder;
};

template <typename TStateProto>
struct TTypedScenarioEnv : public TTypedLightScenarioEnv<TStateProto> {
    TTypedScenarioEnv(const IContext& ctx, const TRequest& request,
                      TArrayRef<const TSemanticFrame> requestFrames, TStateProto& state,
                      NMegamind::TScenarioFeatures& features, NMegamind::TAnalyticsInfoBuilder& analyticsInfoBuilder,
                      NMegamind::TUserInfoBuilder& userInfoBuilder)
        : TTypedLightScenarioEnv<TStateProto>{ctx, request, requestFrames, state, analyticsInfoBuilder, userInfoBuilder}
        , Ctx{ctx}
        , Features{features} {
    }

    const IContext& Ctx;
    NMegamind::TScenarioFeatures& Features;
};

using TLightScenarioEnv = TTypedLightScenarioEnv<TState>;
using TScenarioEnv = TTypedScenarioEnv<TState>;

template <typename TStateProto, typename TFn>
typename std::result_of_t<TFn(TStateProto&)> WithTypedState(TState& state, const TFn& fn) {
    TStateProto s;
    Y_ENSURE(state.GetState().Is<TStateProto>());

    state.GetState().UnpackTo(&s);
    auto finalizer = [&state, &s]() {
        if (s.IsInitialized()) {
            state.MutableState()->PackFrom(s);
        } else {
            ythrow yexception() << "Bad protobuf package in state";
        }
    };

    if constexpr (std::is_same_v<decltype(fn(s)), void>) {
        fn(s);
        finalizer();
    } else {
        const auto r = fn(s);
        finalizer();
        return r;
    }
}

template <typename TStateProto, typename TFn>
typename std::result_of_t<TFn(TTypedScenarioEnv<TStateProto>&)> WithTypedEnv(TScenarioEnv& env, const TFn& fn) {
    TStateProto state;
    if (env.State.HasState()) {
        Y_ENSURE(env.State.GetState().Is<TStateProto>());
        env.State.GetState().UnpackTo(&state);
    }

    auto finalizer = [&env, &state]() {
        Y_ENSURE(state.IsInitialized(), "Bad protobuf package in state");
        env.State.MutableState()->PackFrom(state);
    };

    TTypedScenarioEnv<TStateProto> scenarioEnv{
        env.Ctx, env.Request, env.RequestFrames, state, env.Features, env.AnalyticsInfoBuilder, env.UserInfoBuilder};
    if constexpr (std::is_same_v<decltype(fn(scenarioEnv)), void>) {
        fn(scenarioEnv);
        finalizer();
    } else {
        const auto result = fn(scenarioEnv);
        finalizer();
        return result;
    }
}

template <typename TStateProto, typename TFn>
typename std::result_of_t<TFn(TTypedLightScenarioEnv<TStateProto>&)> WithTypedEnv(TLightScenarioEnv& env,
                                                                                  const TFn& fn) {
    TStateProto state;
    if (env.State.HasState()) {
        Y_ENSURE(env.State.GetState().Is<TStateProto>());
        env.State.GetState().UnpackTo(&state);
    }

    auto finalizer = [&env, &state]() {
        Y_ENSURE(state.IsInitialized(), "Bad protobuf package in state");
        env.State.MutableState()->PackFrom(state);
    };

    TTypedLightScenarioEnv<TStateProto> scenarioEnv{env.Ctx, env.Request, env.RequestFrames, state,
                                                    env.AnalyticsInfoBuilder, env.UserInfoBuilder};
    if constexpr (std::is_same_v<decltype(fn(scenarioEnv)), void>) {
        fn(scenarioEnv);
        finalizer();
    } else {
        const auto result = fn(scenarioEnv);
        finalizer();
        return result;
    }
}

} // namespace NAlice
