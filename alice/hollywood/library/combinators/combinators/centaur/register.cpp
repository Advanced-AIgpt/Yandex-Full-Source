#include "centaur_teasers.h"
#include "centaur_main_screen.h"

#include <alice/hollywood/library/registry/hw_service_registry.h>

namespace NAlice::NHollywood::NCombinators {

REGISTER_HOLLYWOOD_SERVICE(TCentaurCombinatorRunHandle);
REGISTER_HOLLYWOOD_SERVICE(TCentaurCombinatorContinueHandle);
REGISTER_HOLLYWOOD_SERVICE(TCentaurCombinatorFinalizeHandle);
REGISTER_HOLLYWOOD_SERVICE(TCentaurMainScreenRunHandle);
REGISTER_HOLLYWOOD_SERVICE(TCentaurMainScreenContinueHandle);
REGISTER_HOLLYWOOD_SERVICE(TCentaurMainScreenFinalizeHandle);

} // namespace NAlice::NHollywood::NCombinators
