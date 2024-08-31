#include "video_search_rpc.h"

#include <alice/hollywood/library/registry/hw_service_registry.h>

namespace NAlice::NHollywood::NVideo {

REGISTER_HOLLYWOOD_SERVICE(TGetTvSearchResultHandle);
REGISTER_HOLLYWOOD_SERVICE(TGetTvSearchResultHandleFinalize);

} // namespace NAlice::NHollywood::NVideo
