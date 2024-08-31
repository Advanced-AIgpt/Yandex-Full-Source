#include <alice/hollywood/library/registry/hw_service_registry.h>

#include "cache_prepare_get.h"
#include "cache_process_get.h"

#include "cache_prepare_set.h"
#include "cache_process_set.h"

namespace NAlice::NHollywood::NMusic::NCache {

REGISTER_HOLLYWOOD_SERVICE(TCachePrepareGetHandle);
REGISTER_HOLLYWOOD_SERVICE(TCacheProcessGetHandle);

REGISTER_HOLLYWOOD_SERVICE(TCachePrepareSetHandle);
REGISTER_HOLLYWOOD_SERVICE(TCacheProcessSetHandle);

} // namespace NAlice::NHollywood::NMusic::NCache
