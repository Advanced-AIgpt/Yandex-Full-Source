#include "set_sound.h"

#include <alice/bass/forms/directives.h>

namespace NBASS {

TResultValue TSoundSettingsNavigatorIntent::SetupSchemeAndParams() {
    if (Context.FormName() == Mute) {
        Params.InsertUnescaped(TStringBuf("value"), TStringBuf("Alerts"));
    } else if (Context.FormName() == Unmute) {
        Params.InsertUnescaped(TStringBuf("value"), TStringBuf("All"));
    } else {
        Context.AddErrorBlock(
            TError(
                TError::EType::NOTSUPPORTED
            )
        );
        return TResultValue();
    }

    Params.InsertUnescaped(TStringBuf("name"), TStringBuf("soundNotifications"));

    return TResultValue();
}

TDirectiveFactory::TDirectiveIndex TSoundSettingsNavigatorIntent::GetDirectiveIndex() {
    return GetAnalyticsTagIndex<TNavigatorSetSettingsDirective>();
}

}
