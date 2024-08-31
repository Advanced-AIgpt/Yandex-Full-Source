#pragma once

//
// HOLLYWOOD FRAMEWORK
// Internal request helper, not for pubic use
//

#include "request.h"

namespace NAlice::NHollywoodFw::NPrivate {

class TRequestHelper {
public:
    explicit TRequestHelper(const TRequest& request)
        : BaseRequestProto_(request.Client().BaseRequestProto_)
    {}
    const NScenarios::TScenarioBaseRequest& GetBaseRequestProto() const {
        return BaseRequestProto_;
    }
private:
    const NScenarios::TScenarioBaseRequest& BaseRequestProto_;
};

} // namespace NAlice::NHollywoodFw::NPrivate
