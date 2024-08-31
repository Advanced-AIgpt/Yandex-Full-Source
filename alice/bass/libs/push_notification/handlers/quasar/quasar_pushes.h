#pragma once

#include <alice/bass/libs/push_notification/handlers/handler.h>
#include <alice/bass/libs/push_notification/scheme/scheme.sc.h>

#include <library/cpp/scheme/util/scheme_holder.h>
#include <library/cpp/scheme/domscheme_traits.h>

namespace NBASS::NPushNotification {

class TQuasarPushes final : public IHandlerGenerator {
public:
    TResultValue Generate(THandler& holder, TApiSchemeHolder scheme) final override;
};

} // namespace NBASS::NPushNotification
