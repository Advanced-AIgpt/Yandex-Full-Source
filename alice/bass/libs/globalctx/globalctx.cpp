#include "globalctx.h"

void NBASS::IGlobalContext::InitHandlers() {
    if (HandlersInitialized) {
        return;
    }

    HandlersInitialized = true;
    DoInitHandlers();
}
