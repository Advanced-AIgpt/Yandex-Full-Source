#pragma once

#include "handler.h"

namespace NBASS::NPushNotification {

class TSimplePush : public IHandlerGenerator {
public:
    TResultValue Generate(THandler& holder, TApiSchemeHolder scheme) final override;
};

} // namespace NBASS::NPushNotification
