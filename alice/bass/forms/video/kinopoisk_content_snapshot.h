#pragma once

#include <alice/bass/libs/video_common/video.sc.h>

#include <library/cpp/scheme/domscheme_traits.h>
#include <library/cpp/scheme/util/scheme_holder.h>

#include <util/generic/singleton.h>

namespace NBASS {
namespace NVideo {

using TKinopoiskContentItem = TSchemeHolder<NBassApi::TKinopoiskContentItem<TSchemeTraits>>;

// Lookup for item by its Kinopoisk id
// returns:
//     ptr to found item or nullptr otherwise
const TKinopoiskContentItem* FindKpContentItemByKpId(TStringBuf kpId);

// Reads data about Kinopoisk content snapshot from disk (before first request)
void InitKpContentSnapshot();

} // namespace NVideo
} // namespace NBASS
