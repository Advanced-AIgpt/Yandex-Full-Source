#include "playlists.h"

#include <alice/hollywood/library/scenarios/music/fairy_tales/child_age_settings.h>

#include <alice/memento/proto/user_configs.pb.h>

namespace NAlice::NHollywood::NMusic {

namespace {

using namespace NAlice::NMusic;

constexpr TSpecialPlaylistInfo PLAYLIST_DEFAULT {
    /* title = */ "Плейлист сказок для Алисы",
    /* kind = */ "1039",
    /* ownerLogin = */ "yamusic-podcast",
    /* ownerId = */ "970829816",
};
constexpr TSpecialPlaylistInfo PLAYLIST_BABIES {
    /* title = */ "Сказки для самых маленьких",
    /* kind = */ "1002",
    /* ownerLogin = */ "yamusic-podcast",
    /* ownerId = */ "970829816",
};
constexpr TSpecialPlaylistInfo PLAYLIST_KIDS {
    /* title = */ "Детские сказки",
    /* kind = */ "1872",
    /* ownerLogin = */ "yamusic-podcast",
    /* ownerId = */ "103372440",
};
constexpr TSpecialPlaylistInfo PLAYLIST_CHILDREN {
    /* title = */ "Сказки для детей 8–12 лет",
    /* kind = */ "1045",
    /* ownerLogin = */ "yamusic-podcast",
    /* ownerId = */ "970829816",
};
constexpr TSpecialPlaylistInfo PLAYLIST_BEDTIME {
    /* title = */ "Сказки на ночь",
    /* kind = */ "1007",
    /* ownerLogin = */ "yamusic-podcast",
    /* ownerId = */ "970829816",
};

} // namespace

const TSpecialPlaylistInfo& DefaultFairyTalePlaylist(
    TRTLogger& logger,
    const ru::yandex::alice::memento::proto::TUserConfigs& userConfigs,
    const bool bedtimeTales
) {
    if (bedtimeTales) {
        return PLAYLIST_BEDTIME;
    }
    if (ChildAgeIsSet(userConfigs.GetChildAge())) {
        LOG_DEBUG(logger) << "Child age: " << userConfigs.GetChildAge().GetAge();
        const auto childAge = userConfigs.GetChildAge().GetAge();
        if (childAge <= 3) {
            return PLAYLIST_BABIES;
        } else if (childAge <= 7) {
            return PLAYLIST_KIDS;
        } else {
            return PLAYLIST_CHILDREN;
        }
    }
    LOG_DEBUG(logger) << "Child age is not set";
    return PLAYLIST_DEFAULT;
}

} // namespace NAlice::NHollywood::NMusic
