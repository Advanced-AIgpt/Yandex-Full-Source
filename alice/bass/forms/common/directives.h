#include <alice/bass/forms/context/context.h>

namespace NBASS {

bool TryAddOpenUriDirective(TContext& ctx, const TString& uri, const TStringBuf screen_id="");
bool TryAddShowPromoDirective(TContext& ctx);

}
