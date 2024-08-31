#pragma once

#include <alice/hollywood/library/bass_adapter/bass_renderer.h>
#include <alice/hollywood/library/request/request.h>

#include <alice/library/logger/fwd.h>

#include <alice/megamind/protos/scenarios/response.pb.h>

#include <util/generic/variant.h>

#include <memory>

namespace NAlice::NHollywood::NMusic {

enum class EShowType {
    Morning /* "morning" */,
    Evening /* "evening" */,
};

class TMusicHardcodedIntent {
public:
    TMusicHardcodedIntent(TRTLogger& logger, const TString& name)
        : Logger(logger)
        , Name(name)
    {
    }

    const TString& GetName() const {
        return Name;
    }

    virtual ~TMusicHardcodedIntent() = default;

    virtual std::variant<TFrame, NScenarios::TScenarioRunResponse> PrepareMusicFrame(
            TScenarioHandleContext& ctx,
            const TScenarioRunRequestWrapper& request,
            TNlgWrapper& nlg) const = 0;

    virtual void RenderResponse(const NJson::TJsonValue& bassResponse,
                                TBassResponseRenderer& bassRenderer) const = 0;
    virtual const TString& ProductScenarioName() const = 0;

protected:
    TRTLogger& Logger;
    TString Name;
};

std::unique_ptr<TMusicHardcodedIntent> CreateMusicHardcodedIntent(
    TRTLogger& logger,
    const TScenarioRunRequestWrapper& request);

} // namespace NAlice::NHollywood::NMusic
