#pragma once

#include "common.h"

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/scenarios/music/proto/music_hardcoded_arguments.pb.h>

#include <alice/megamind/protos/scenarios/request_meta.pb.h>

#include <alice/megamind/protos/scenarios/response.pb.h>

#include <memory>

namespace NAlice::NHollywood::NMusic {

namespace NImpl {

std::unique_ptr<NAlice::NScenarios::TScenarioRunResponse> MusicRenderDoImpl(
    const TScenario::THandleBase& handle,
    const NHollywood::TScenarioRunRequestWrapper& runRequest,
    const NJson::TJsonValue& bassResponse,
    const TMusicPromoConfig& promoConfig,
    NHollywood::TContext& ctx,
    TNlgWrapper& nlgWrapper);

std::unique_ptr<NAlice::NScenarios::TScenarioRunResponse> MusicHardcodedRenderDoImpl(
    const NHollywood::TScenarioRunRequestWrapper& runRequest,
    const TMorningShowProfile& morningShowProfile,
    const TMusicHardcodedArguments& applyArgs,
    NJson::TJsonValue bassResponse,
    NHollywood::TContext& ctx,
    TNlgWrapper& nlgWrapper);

} // namespace NImpl

class TBassMusicRenderHandle : public TScenario::THandleBase {
public:
    TBassMusicRenderHandle();

    TString Name() const override {
        return "render";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

class TBassMusicHardcodedRenderHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "render";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

} // namespace NAlice::NHollywood::NMusic
