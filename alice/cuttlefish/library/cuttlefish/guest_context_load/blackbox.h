#pragma once

#include <alice/cuttlefish/library/cuttlefish/common/blackbox.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>

namespace NAlice::NCuttlefish::NAppHostServices {

    template <typename ...TArgs>
    TMaybe<TBlackboxClient::TOAuthResponse> ParseBlackboxResponse(TArgs&& ...args) {
        return TBlackboxResponseParser(ITEM_TYPE_GUEST_BLACKBOX_HTTP_RESPONSE).TryParse(std::forward<TArgs>(args)...);
    }
}
