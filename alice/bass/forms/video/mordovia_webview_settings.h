#pragma once

#include <alice/bass/forms/context/fwd.h>

#include <library/cpp/cgiparam/cgiparam.h>

#include <util/generic/string.h>

namespace NAlice::NVideoCommon {

TString GetWebViewVideoHost(const NBASS::TContext& ctx);

TString GetWebViewVideoGalleryPath(const NBASS::TContext& ctx);
TString GetWebViewVideoGallerySplash(const NBASS::TContext& ctx);

TString GetWebViewFilmsGalleryPath(const NBASS::TContext& ctx);
TString GetWebViewFilmsGallerySplash(const NBASS::TContext& ctx);

TString GetWebViewVideoSingleCardPath(const NBASS::TContext& ctx);
TString GetWebViewVideoSingleCardSplash(const NBASS::TContext& ctx);

TString GetWebViewVideoDescriptionCardPath(const NBASS::TContext& ctx);
TString GetWebViewVideoDescriptionCardSplash(const NBASS::TContext& ctx);

TString GetWebViewSeasonsGalleryPath(const NBASS::TContext& ctx);
TString GetWebViewSeasonsGallerySplash(const NBASS::TContext& ctx);

TString GetWebViewChannelsPath(const NBASS::TContext& ctx);
TString GetWebViewChannelsSplash(const NBASS::TContext& ctx);

TString GetWebViewPromoSplash(const NBASS::TContext& ctx);

TString BuildWebViewVideoUrl(TStringBuf host, TStringBuf path, const TCgiParameters& params);
TString BuildWebViewVideoPath(TStringBuf path, const TCgiParameters& params);

} // namespace NAlice::NVideoCommon
