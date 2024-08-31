#pragma once

#include <alice/hollywood/library/combinators/combinators/centaur/widget_service.h>

#include <alice/hollywood/library/base_hw_service/base_hw_service_handle.h>
#include <alice/hollywood/library/combinators/request/request.h>

#include <alice/megamind/protos/scenarios/response.pb.h>
#include <alice/memento/proto/api.pb.h>
#include <alice/protos/data/scenario/centaur/main_screen.pb.h>


namespace NAlice::NHollywood::NCombinators {

class TCentaurMainScreenRunHandle : public IHwServiceHandle {
public:
    void Do(THwServiceContext& ctx) const override;
    const TString& Name() const override {
        return RUN_HANDLE_NAME;
    }
};

class TCentaurMainScreenFinalizeHandle : public IHwServiceHandle {
public:
    void Do(THwServiceContext& ctx) const override;
    const TString& Name() const override {
        return FINALIZE_HANDLE_NAME;
    }
};

class TCentaurMainScreenContinueHandle : public IHwServiceHandle {
public:
    void Do(THwServiceContext& ctx) const override;
    const TString& Name() const override {
        return CONTINUE_HANDLE_NAME;
    }
};

class TCentaurMainScreenCombinator {
public:
    explicit TCentaurMainScreenCombinator(THwServiceContext& ctx);
    void Run();
    void Continue();
    void Finalize();

private:
    THwServiceContext& Ctx;
    const NScenarios::TCombinatorRequest CombinatorRequest;
    TCombinatorRequestWrapper Request;
};

} // namespace NAlice::NHollywood::NCombinators
