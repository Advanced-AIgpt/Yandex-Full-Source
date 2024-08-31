#include "fairy_tales.h"

namespace NBASS::NMusic {

bool IsFairyTaleFilterGenre(const TContext& ctx) {
    const auto slotIsFairyTaleNew = ctx.GetSlot("is_fairy_tale_filter_genre");
    return !IsSlotEmpty(slotIsFairyTaleNew) &&
        (slotIsFairyTaleNew->Value.GetBool(false) || slotIsFairyTaleNew->Value.GetString("") == "true");
}

} // namespace NBASS::NMusic
