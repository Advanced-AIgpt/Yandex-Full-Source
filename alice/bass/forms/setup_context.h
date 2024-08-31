#pragma once

#include <alice/bass/forms/context/context.h>

namespace NBASS {

/** Context for setup requests.
 * Wiki: https://wiki.yandex-team.ru/Alice/backend/setupproto/
 */
class TSetupContext : public TContext {
public:
    using TPtr = TIntrusivePtr<TSetupContext>;

public:
    static TResultValue FromJson(const NSc::TValue& request, TInitializer init, TPtr* out);

    void SetFeasible(bool isFeasible);
    void AddSourceResponse(TStringBuf name, NHttpFetcher::TResponse::TRef response) override; // TContext
    void AddSourceFactorsData(TStringBuf source, const NSc::TValue& data) override; // TContext

    NSc::TValue& SetupMeta() {
        return SetupMetaJson;
    }

    void ToJson(NSc::TValue* form) const;

private:
    using TContext::TContext;

private:
    bool IsFeasible = true;
    THashMap<TString, NHttpFetcher::TResponse::TRef> SourceResponses;
    NSc::TValue SetupMetaJson;
};

} // namespace NBASS
