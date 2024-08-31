#include "impl.h"

#include <alice/hollywood/library/scenarios/music/music_sdk_handles/common.h>

namespace NAlice::NHollywood::NMusic::NImpl {

TMaybe<TString> TRunPrepareHandleImpl::TryGetSubgraphFlag() {
    // FIXME(sparkle): support fixlists at MusicSDK https://st.yandex-team.ru/HOLLYWOOD-633
    if (!FindFrame(MUSIC_PLAY_FIXLIST_FRAME)) {
        if (NMusicSdk::ShouldInvokeSubgraph(Request_)) {
            return NMusicSdk::SUBGRAPH_APPHOST_FLAG;
        }
    }

    return Nothing();
}

} // namespace NAlice::NHollywood::NMusic::NImpl
