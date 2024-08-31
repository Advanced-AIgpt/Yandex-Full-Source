#include "requests.h"

namespace NBASS {
namespace NVideo {

namespace {

TItemTypeFlags FromContentType(TMaybe<EContentType> contentType) {
    if (!contentType.Defined()) {
        return ALL_ITEM_TYPES;
    }
    switch (contentType.GetRef()) {
    case EContentType::Movie:
        return EItemType::Movie;

    case EContentType::TvShow:
        return EItemType::TvShow;

    case EContentType::Video:
        return EItemType::Video;

    default:
        return ALL_ITEM_TYPES;
    }
}

} // namespace anonymous

TVideoClipsRequest::TVideoClipsRequest(const TVideoSlots& slots)
    : Slots(slots)
    , ContentType(Slots.ContentType.Defined() ? Slots.ContentType.GetEnumValue() : ALL_CONTENT_TYPES)
    , ItemType(FromContentType(Slots.ContentType.GetMaybe()))
{
}

} // namespace NVideo
} // namespace NBASS

