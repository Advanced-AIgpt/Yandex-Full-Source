#include "vsid.h"

#include <alice/hollywood/library/scenarios/music/music_backend_api/music_common.h>

#include <util/string/builder.h>

namespace NAlice::NHollywood::NMusic {

namespace {

constexpr TStringBuf CLIENT_TYPE = "STM";

}

TString MakeHollywoodVsid(IRng& rng, TInstant ts) {
    // https://wiki.yandex-team.ru/player/vsid/
    TStringBuilder  rv;
    rv.reserve(44 + 1 + 3 + 1 + 4 + 1 + 10);
    rv << GenerateRandomString(rng, 44) << 'x' << CLIENT_TYPE << 'x';
    rv << "0000"; // TODO(stupnik): get version from device state?
    rv << 'x' << ts.Seconds();
    return rv;
}

} // namespace NAlice::NHollywood::NMusic
