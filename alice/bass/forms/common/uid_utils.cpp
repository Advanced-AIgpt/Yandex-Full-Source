#include "uid_utils.h"

#include <alice/library/biometry/biometry.h>

namespace NBASS {

EUidAcquireType GetUidAcquireType(const TContext& ctx) {
    if (NAlice::NBiometry::HasBiometricsScores(ctx.Meta())) {
        return EUidAcquireType::BIOMETRY;
    }
    if (ctx.IsAuthorizedUser()) {
        return EUidAcquireType::BLACK_BOX;
    }
    return EUidAcquireType::UNAUTHORIZED;
}

TString GetUnauthorizedUid(const TContext& context) {
    TStringBuilder uidBuilder;
    uidBuilder << "uuid:" << TString{*context.Meta().UUID()};
    return uidBuilder;
}

} // namespace NBASS
