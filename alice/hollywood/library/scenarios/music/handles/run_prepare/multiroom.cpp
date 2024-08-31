#include "impl.h"

#include <alice/library/experiments/flags.h>

#include <alice/hollywood/library/multiroom/multiroom.h>
#include <alice/hollywood/library/scenarios/music/common.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_common.h>

namespace NAlice::NHollywood::NMusic::NImpl {

void TRunPrepareHandleImpl::TryFillActivateMultiroomSlot(TFrame& frame) {
    const TMultiroom multiroom{Request_};
    if (multiroom.IsDeviceInGroup() && frame.Name() == MUSIC_PLAY_FRAME && multiroom.IsActiveWithFrame(frame)) {
        frame.AddSlot(TSlot{TString{::NAlice::NMusic::SLOT_ACTIVATE_MULTIROOM},
                            TString{::NAlice::NMusic::SLOT_ACTIVATE_MULTIROOM_TYPE},
                            TSlot::TValue{"true"}});
    }
}

} // namespace NAlice::NHollywood::NMusic::NImpl
