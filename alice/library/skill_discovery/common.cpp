#include "common.h"

#include <util/generic/utility.h>
#include <util/string/builder.h>
#include <util/string/util.h>

namespace NAlice::NSkillDiscovery {

double NormalizeRelev(double relev) {
    constexpr double MAX_RELEV = 1e8;
    if (relev > MAX_RELEV)
        relev -= MAX_RELEV;
    relev /= MAX_RELEV;
    return ClampVal(relev, 0.0, 1.0);
}

bool IsCommercialQuery(double commercialMx) {
    constexpr double commercialityThreshold = 0.3;
    return commercialMx >= commercialityThreshold;
}

TString CreateImageUrl(const TStringBuf avatarHost,
                       const TStringBuf imageId,
                       const TStringBuf imageType,
                       const TStringBuf ns,
                       double scaleFactor)
{
    TStringBuilder url;
    url << avatarHost;
    addIfNotLast(url, '/');
    url << "get-" << ns << '/' << imageId << '/' << imageType;

    if (imageType == IMAGE_TYPE_BIG || imageType == IMAGE_TYPE_MOBILE_LOGO || imageType == IMAGE_TYPE_SMALL) {
        // FIXME remove it as soon as it fixed in https://st.yandex-team.ru/PASKILLS-679#1525445533000
        if (imageType == IMAGE_TYPE_MOBILE_LOGO && scaleFactor > 2) {
            scaleFactor = 2.0;
        }
        url << scaleFactor;
    }
    return url;
}

} // NAlice::NSkillDiscovery
