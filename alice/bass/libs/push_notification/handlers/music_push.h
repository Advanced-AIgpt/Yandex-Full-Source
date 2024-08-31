#pragma once

#include "handler.h"

namespace NBASS::NPushNotification {

class TMusicPush : public IHandlerGenerator {
public:
    TResultValue Generate(THandler& holder, TApiSchemeHolder scheme) final override;
};

} // namespace NBASS::NPushNotification
