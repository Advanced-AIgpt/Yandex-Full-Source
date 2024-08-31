#pragma once

#include <alice/hollywood/library/request/experiments.h>

#include <alice/library/logger/logger.h>
#include <alice/library/metrics/sensors.h>
#include <alice/library/util/rng.h>
#include <alice/megamind/protos/modifiers/modifier_request.pb.h>

#include <util/generic/hash.h>

namespace NAlice::NHollywood::NModifiers {

using TModifierFeatures = NMegamind::TModifierFeatures;
using TModifierBaseRequest = NMegamind::TModifierRequest_TBaseRequest;

class IModifierContext {
public:
    [[nodiscard]] virtual const TModifierFeatures& GetFeatures() const = 0;
    [[nodiscard]] virtual const TModifierBaseRequest& GetBaseRequest() const = 0;
    virtual TRTLogger& Logger() = 0;
    [[nodiscard]] virtual bool HasExpFlag(TStringBuf name) const = 0;
    [[nodiscard]] virtual const TExpFlags& ExpFlags() const = 0;
    virtual IRng& Rng() = 0;
    virtual NMetrics::ISensors& Sensors() = 0;
};

class TModifierContext : public IModifierContext {
public:
    TModifierContext(TRTLogger& logger, const TModifierFeatures& features, const TModifierBaseRequest& baseRequest,
                     IRng& rng, NMetrics::ISensors& sensors);

    [[nodiscard]] const TModifierFeatures& GetFeatures() const override;
    [[nodiscard]] const TModifierBaseRequest& GetBaseRequest() const override;
    TRTLogger& Logger() override;
    [[nodiscard]] bool HasExpFlag(TStringBuf name) const override;
    [[nodiscard]] const TExpFlags& ExpFlags() const override;
    IRng& Rng() override;
    NMetrics::ISensors& Sensors() override;
private:
    TRTLogger& Logger_;
    const TModifierFeatures& Features;
    const TModifierBaseRequest& BaseRequest;
    const TExpFlags ExpFlags_;
    IRng& Rng_;
    NMetrics::ISensors& Sensors_;
};

} // namespace NAlice::NHollywood::NModifiers
