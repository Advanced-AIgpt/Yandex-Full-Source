#pragma once

#include "video_provider.h"

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NBASS::NVideo {
class TVoidProvider final : public TVideoClipsHttpProviderBase {
public:
    TVoidProvider(TContext& context, TStringBuf name)
        : TVideoClipsHttpProviderBase(context)
        , Name(name) {
    }

    // IVideoClipsProvider overrides:
    TStringBuf GetProviderName() const override {
        return Name;
    }

    bool IsUnauthorized() const override {
        return false;
    }

protected:
    // IVideoClipsProvider overrides:
    TPlayResult GetPlayCommandDataImpl(TVideoItemConstScheme /* item */,
                                       TPlayVideoCommandDataScheme /* commandData */) const override {
        return TPlayError{NVideoCommon::EPlayError::SERVICE_CONSTRAINT_VIOLATION};
    }

private:
    TString Name;
};
} // namespace NBASS::NVideo
