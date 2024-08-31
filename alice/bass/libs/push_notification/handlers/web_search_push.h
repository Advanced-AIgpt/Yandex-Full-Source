#pragma once

#include "handler.h"

namespace NBASS::NPushNotification {

class TWebSearchPush : public IHandlerGenerator {
public:
    TResultValue Generate(THandler& holder, TApiSchemeHolder scheme) final override;
};

} // namespace NBASS::NPushNotification
