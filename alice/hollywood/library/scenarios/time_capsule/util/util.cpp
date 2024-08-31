#include "util.h"

namespace NAlice::NHollywood::NTimeCapsule {

TString MakeSharedLinkImageUrl(const TString& avatarsId, bool full) {
    const TString postfix = full ? "/orig" : "/catalogue-banner-x1";

    return TString::Join("http://avatars.mds.yandex.net", avatarsId, postfix);
}

} // namespace NAlice::NHollywood::NTimeCapsule
