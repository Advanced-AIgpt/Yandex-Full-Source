#pragma once

#include <alice/bass/forms/special_location.h>
#include <alice/bass/forms/vins.h>

namespace NBASS {

class TSaveAddressHandler: public IHandler {
public:
    enum class EModes {
        SaveAddress,
        ConnectDeviceToAddress
    };
public:
    TResultValue Do(TRequestHandler& r) override;

    // Switch request to return this form instead of original one.
    static TResultValue SetAsResponse(TContext& ctx, TSpecialLocation name, EModes mode = EModes::SaveAddress);

    static void Register(THandlersMap* handlers);

private:
    TResultValue SaveAddress(TContext& context, const NSc::TValue& address) const;
    TResultValue SaveDeviceConnectionToSpecialLocation(TContext& context, bool confirmation) const;
    TResultValue ProcessAddressSaving(TContext& context, bool hasConfirmation);
    TResultValue ResolveAddress(TContext& context, size_t resultIndex) const;

    // Switch request to return callback form (if any) instead of this one
    TResultValue SwitchToCallbackForm(TContext& ctx);
};

}
