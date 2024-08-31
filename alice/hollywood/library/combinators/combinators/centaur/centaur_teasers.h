#pragma once

#include "defs.h"
#include <alice/hollywood/library/hw_service_context/context.h>
#include <alice/hollywood/library/combinators/request/request.h>
#include <alice/megamind/protos/scenarios/combinator_request.pb.h>
#include <alice/hollywood/library/base_hw_service/base_hw_service_handle.h>


namespace NAlice::NHollywood::NCombinators {

class TCentaurCombinatorRunHandle : public IHwServiceHandle {
public:
    void Do(THwServiceContext& ctx) const override;
    const TString& Name() const override {
        return CENTAUR_COMBINATOR_RUN_HANDLE_NAME;
    }
};

class TCentaurCombinatorContinueHandle : public IHwServiceHandle {
public: 
    void Do(THwServiceContext& ctx) const override;
    const TString& Name() const override {
        return CENTAUR_COMBINATOR_CONTINUE_HANDLE_NAME;
    }
};

class TCentaurCombinatorFinalizeHandle : public IHwServiceHandle {
public:
    void Do(THwServiceContext& ctx) const override;
    const TString& Name() const override {
        return CENTAUR_COMBINATOR_FINALIZE_HANDLE_NAME;
    }
};


class TCentaurTeasersCombinator {
public:
    explicit TCentaurTeasersCombinator(THwServiceContext& ctx);
    void Run();
    void Continue();
    void Finalize();

private:
    THwServiceContext& Ctx;
    const NScenarios::TCombinatorRequest CombinatorRequest;
    TCombinatorRequestWrapper Request;
};

} // namespace NAlice::NHollywood::NCombinators
