#include "simple_push.h"

namespace NBASS::NPushNotification {

TResultValue TSimplePush::Generate(THandler& handler, TApiSchemeHolder scheme) {
    if (scheme->Event() == TStringBuf("reminder")) {
        handler.SetInfo("reminder", "server_action", "simple_title", "simple_body", "simple_url", "service_name_or_other_tag", 360, "simple_quasar_payload", "bass-default-push");
        handler.AddSelf();
        handler.AddCustom("ru.yandex.mobile.inhouse");
        return ResultSuccess();
    }
    return TError{TError::EType::INVALIDPARAM,
                  TStringBuilder{} << "no handler found for '" << scheme->Service() << "' and event '" << scheme->Event() << '\''
    };
}

} // namespace NBASS::NPushNotification
