#include "rule_input.h"
#include <alice/begemot/lib/api/experiments/flags.h>

namespace NAlice {

    // ~~~~ Is rule enabled ~~~~

    bool IsRuleEnabled(const NBg::NProto::TInternalContext& ctx, TStringBuf ruleName) {
        for (const TDynamicField& mode : ctx.GetRuleModes()) {
            if (mode.GetName() == ruleName) {
                return true;
            }
        }
        return false;
    }

    bool IsAnyRuleEnabled(const NBg::NProto::TInternalContext& ctx, const TVector<TString>& ruleNames) {
        for (const TString& ruleName : ruleNames) {
            if (IsRuleEnabled(ctx, ruleName)) {
                return true;
            }
        }
        return false;
    }

    // ~~~~ Params from wizextra ~~~~

    TString GetWizextraParam(const NBg::TInput& input, TStringBuf name) {
        TString result;
        input.ForEachFactor(WIZEXTRA, [&](TStringBuf key, TStringBuf value, bool) {
            if (key == name) {
                result = TString(value);
            }
        });
        return result;
    }

    TVector<TString> GetWizextraParamAllValues(const NBg::TInput& input, TStringBuf name) {
        TVector<TString> result;
        input.ForEachFactor(WIZEXTRA, [&](TStringBuf key, TStringBuf value, bool) {
            if (key == name) {
                result.push_back(TString(value));
            }
        });
        return result;
    }

    bool HasWizextraFlag(const NBg::TInput& input, TStringBuf name) {
        bool result = false;
        input.ForEachFactor(WIZEXTRA, [&](TStringBuf key, TStringBuf value, bool) {
            if (key == name && !IsFalse(value)) {
                result = true;
            }
        });
        return result;
    }

    TSet<TString> GetEnabledAliceExperiments(const NBg::TInput& input) {
        TSet<TString> result;
        input.ForEachFactor(WIZEXTRA, [&](TStringBuf key, TStringBuf value, bool) {
            if (!IsFalse(value)) {
                result.insert(TString(key));
            }
        });
        const TString mmExperiments = GetWizextraParam(input, WIZEXTRA_KEY_ENABLED_MEGAMIND_EXPERIMENTS);
        for (const TStringBuf mmExperiment : StringSplitter(mmExperiments).Split(',').SkipEmpty()) {
            result.insert(TString(mmExperiment));
        }
        return result;
    }

    // ~~~~ Other ~~~~

} // namespace NAlice
