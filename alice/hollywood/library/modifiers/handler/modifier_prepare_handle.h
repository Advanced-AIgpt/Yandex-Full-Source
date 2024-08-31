#pragma once

#include <alice/hollywood/library/base_hw_service/base_hw_service_handle.h>
#include <alice/hollywood/library/modifiers/base_modifier/base_modifier.h>

namespace NAlice::NHollywood::NModifiers {

namespace NImpl {

void PrepareImpl(const NMegamind::TModifierRequest& request,
                 IModifierContext& ctx,
                 IExternalSourceRequestCollector& externalSourcesRequestCollector,
                 const TVector<TBaseModifierPtr>& modifiers);

} // namespace NImpl

class TModifierPrepareHandle : public IHwServiceHandle {
public:
    void Do(THwServiceContext& ctx) const override;
    void InitWithConfig(const THwServicesConfig& config, const TFsPath& resourcesBasePath) override;
    const TString& Name() const override;
private:
    TMaybe<TVector<TBaseModifierPtr>> Modifiers_;
};

} // namespace NAlice::NHollywood::NModifiers
