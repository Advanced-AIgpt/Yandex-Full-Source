#pragma once

#include "navigator_intent.h"

namespace NBASS {

class TSoundSettingsNavigatorIntent : public INavigatorIntent {
public:
    TSoundSettingsNavigatorIntent(TContext& ctx, const TStringBuf muteFormName, const TStringBuf unmuteFormName)
        : INavigatorIntent(ctx, TStringBuf("set_setting") /* scheme */)
        , Mute(muteFormName)
        , Unmute(unmuteFormName)
    {}

private:
    TResultValue SetupSchemeAndParams() override;
    TDirectiveFactory::TDirectiveIndex GetDirectiveIndex() override;

private:
    TString Mute;
    TString Unmute;
};

}
