#pragma once

#include <alice/library/logger/logger.h>
#include <alice/library/music/common_special_playlists.h>
#include <alice/memento/proto/api.pb.h>

namespace NAlice::NHollywood::NMusic {

const NAlice::NMusic::TSpecialPlaylistInfo& DefaultFairyTalePlaylist(
    TRTLogger& logger,
    const ru::yandex::alice::memento::proto::TUserConfigs& mementoUserConfigs,
    const bool bedtimeTales
);

} // namespace NAlice::NHollywood::NMusic
