#pragma once

#include <alice/bass/forms/vins.h>

namespace NBASS {

// Form with no slots that VINS sends for general conversation cases.
// In replay only search fallback is expected.
class TGeneralConversationFormHandler : public IHandler {
public:
    TGeneralConversationFormHandler();

    TResultValue Do(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);

    static TResultValue SetAsResponse(TContext& ctx);
};

}
