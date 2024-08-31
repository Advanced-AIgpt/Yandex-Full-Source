#include "hwf_state.h"

namespace NAlice::NHollywoodFw {

namespace {

template <class TScenarioResponse>
void SaveHwfStateImpl(const TScenarioResponse& src, TScenarioResponse& dst) {
    const auto* srcBody = GetResponseBody(src);
    auto* dstBody = GetResponseBody(dst);

    if (srcBody == nullptr && dstBody == nullptr) {
        return;
    }

    Y_ENSURE(srcBody);
    Y_ENSURE(dstBody);

    if (srcBody->HasState()) {
        *dstBody->MutableState() = srcBody->GetState();
    } else if (dstBody->HasState()) {
        dstBody->ClearState();
    }
}

}

const NScenarios::TScenarioResponseBody* GetResponseBody(const NScenarios::TScenarioRunResponse& vinsRunResponse) {
    if (vinsRunResponse.HasResponseBody()) {
        return &vinsRunResponse.GetResponseBody();
    }
    if (vinsRunResponse.GetCommitCandidate().HasResponseBody()) {
        return &vinsRunResponse.GetCommitCandidate().GetResponseBody();
    }
    return nullptr;
}

NScenarios::TScenarioResponseBody* GetResponseBody(NScenarios::TScenarioRunResponse& vinsRunResponse) {
    if (vinsRunResponse.HasResponseBody()) {
        return vinsRunResponse.MutableResponseBody();
    }
    if (vinsRunResponse.HasCommitCandidate()) {
        return vinsRunResponse.MutableCommitCandidate()->MutableResponseBody();
    }
    return nullptr;
}

const NScenarios::TScenarioResponseBody* GetResponseBody(const NScenarios::TScenarioApplyResponse& vinsApplyResponse) {
    return vinsApplyResponse.HasResponseBody() ? &vinsApplyResponse.GetResponseBody() : nullptr;
}

NScenarios::TScenarioResponseBody* GetResponseBody(NScenarios::TScenarioApplyResponse& vinsApplyResponse) {
    return vinsApplyResponse.HasResponseBody() ? vinsApplyResponse.MutableResponseBody() : nullptr;
}

void SaveHwfState(const NScenarios::TScenarioRunResponse& src, NScenarios::TScenarioRunResponse& dst) {
    SaveHwfStateImpl(src, dst);
}

void SaveHwfState(const NScenarios::TScenarioApplyResponse& src, NScenarios::TScenarioApplyResponse& dst) {
    SaveHwfStateImpl(src, dst);
}

}
