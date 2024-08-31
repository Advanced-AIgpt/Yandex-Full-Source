#pragma once

#include <alice/hollywood/library/base_hw_service/base_hw_service_handle.h>

namespace NAlice::NHollywood::NMusic::NCentaur {

class TCollectMainScreenPrepareHandle : public IHwServiceHandle {
public:
    const TString& Name() const override;
    void Do(THwServiceContext& ctx) const override;
};

} // namespace NAlice::NHollywood::NMusic::NCentaur
