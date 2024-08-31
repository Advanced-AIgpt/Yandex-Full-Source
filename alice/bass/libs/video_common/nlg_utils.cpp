#include "utils.h"

#include <util/generic/hash_set.h>
#include <util/generic/maybe.h>

using namespace NAlice::NVideoCommon;

namespace NBASS::NVideoCommon {

TMaybe<TString> NlgTemplateIdByMegamindIntent(TStringBuf intent) {
    // TODO Add intents to nlg.
    static const THashSet<TStringBuf> INTENTS_UNSUPPORTED_BY_NLG = {
        QUASAR_VIDEO_PLAY_NEXT_VIDEO,
        QUASAR_VIDEO_PLAY_PREV_VIDEO,
    };
    if (INTENTS_UNSUPPORTED_BY_NLG.contains(intent)) {
        return Nothing();
    }

    TStringBuf baseIntent = intent;
    if (intent == SEARCH_VIDEO_ENTITY) {
        baseIntent = SEARCH_VIDEO;
    } else if (intent == QUASAR_SELECT_VIDEO_FROM_GALLERY_BY_TEXT
            || intent == QUASAR_SELECT_VIDEO_FROM_GALLERY_CALLBACK) {
        baseIntent = QUASAR_SELECT_VIDEO_FROM_GALLERY;
    }
    return TString{baseIntent.RNextTok('.')};
}

} // namespace NBASS::NVideoCommon
