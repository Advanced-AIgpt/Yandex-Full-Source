#pragma once

#include <alice/begemot/lib/api/params/wizextra.h>
#include <search/begemot/apphost/context.h>
#include <util/generic/set.h>

namespace NAlice {

    // ~~~~ Is rule enabled ~~~~

    bool IsRuleEnabled(const NBg::NProto::TInternalContext& ctx, TStringBuf ruleName);
    bool IsAnyRuleEnabled(const NBg::NProto::TInternalContext& ctx, const TVector<TString>& ruleNames);

    // ~~~~ Wizextra utils ~~~~

    TString GetWizextraParam(const NBg::TInput& input, TStringBuf name);
    TVector<TString> GetWizextraParamAllValues(const NBg::TInput& input, TStringBuf name);
    bool HasWizextraFlag(const NBg::TInput& input, TStringBuf name);

    TSet<TString> GetEnabledAliceExperiments(const NBg::TInput& input);

} // namespace NAlice
