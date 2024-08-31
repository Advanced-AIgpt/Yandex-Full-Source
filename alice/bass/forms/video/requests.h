#pragma once

#include "defs.h"
#include "utils.h"
#include "video_slots.h"

#include <alice/bass/libs/fetcher/neh.h>

#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <util/string/join.h>
#include <util/system/types.h>

namespace NBASS {
namespace NVideo {

struct TVideoClipsRequest {
    explicit TVideoClipsRequest(const TVideoSlots& slots);

    TStringBuf GetSearchQuery() const {
        return OverrideSearchQuery ? OverrideSearchQuery : Slots.SearchText.GetString();
    }

    const TVideoSlots& Slots;
    TString OverrideSearchQuery;

    TContentTypeFlags ContentType;
    TItemTypeFlags ItemType;

    i64 From = 0;
    i64 To = 20;

    NHttpFetcher::IMultiRequest::TRef MultiRequest = NHttpFetcher::WeakMultiRequest();

    TString MakeHashForSetup() const {
        return Join("_", GetSearchQuery(), Slots.BuildSearchQueryForWeb(), ContentType, ItemType, From, To);
    }
};

template <typename... Args>
TString ConstructRequestId(const TString& name, const TVideoClipsRequest& request, const TCgiParameters& cgis,  Args&... args) {
    return Join("_", name, request.MakeHashForSetup(), cgis.Print(), args...);
}

} // namespace NVideo
} // namespace NBASS
