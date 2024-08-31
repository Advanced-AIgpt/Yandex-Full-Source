#include "video_provider.h"
#include "defs.h"

namespace NAlice::NVideoCommon {

bool IsPaidProvider(const TStringBuf provider) {
    return !IsInternetVideoProvider(provider);
}

bool IsInternetVideoProvider(const TStringBuf provider) {
    return provider == PROVIDER_YOUTUBE
           || provider == PROVIDER_YAVIDEO
           || provider == PROVIDER_YAVIDEO_PROXY
           || provider == PROVIDER_STRM;
}

bool IsDeprecatedProvider(const TStringBuf provider) {
    return provider == PROVIDER_IVI || provider == PROVIDER_OKKO || provider == PROVIDER_AMEDIATEKA;
}

} // namespace NAlice::NVideoCommon
