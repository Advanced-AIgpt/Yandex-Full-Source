#include "music_config.h"

#include <util/generic/hash.h>
#include <util/string/cast.h>

namespace NAlice::NHollywood::NMusic {

TMusicConfig CreateMusicConfig(const TExpFlags& flags) {
    TMusicConfig rv;
    for (const auto&[key, value] : flags) {
        if (key.StartsWith(EXP_HW_MUSIC_THIN_CLIENT_PAGE_SIZE_PREFIX)) {
            i32 pageSize;
            auto valueStr = TStringBuf(key).SubStr(EXP_HW_MUSIC_THIN_CLIENT_PAGE_SIZE_PREFIX.size());
            if (TryFromString<i32>(valueStr, pageSize)) {
                rv.PageSize = pageSize;
            } else {
                ythrow yexception() << "Invalid value for experiment " << key;
            }
        } else if (key.StartsWith(EXP_HW_MUSIC_THIN_CLIENT_FIND_TRACK_IDX_PAGE_SIZE_PREFIX)) {
            i32 findTrackIdxPageSize;
            auto valueStr = TStringBuf(key).SubStr(EXP_HW_MUSIC_THIN_CLIENT_FIND_TRACK_IDX_PAGE_SIZE_PREFIX.size());
            if (TryFromString<i32>(valueStr, findTrackIdxPageSize)) {
                rv.FindTrackIdxPageSize = findTrackIdxPageSize;
            } else {
                ythrow yexception() << "Invalid value for experiment " << key;
            }
        }
    }
    return rv;
}

TMusicConfig CreateMusicConfig(const NHollywoodFw::TRequest::TFlags& flags) {
    TMusicConfig rv;
    flags.ForEachSubval([&rv](const TString& key, const TString& value, const TMaybe<TString>& /* dummy */) -> bool {
        if (key == EXP_HW_MUSIC_THIN_CLIENT_PAGE_SIZE_PREFIX) {
            i32 pageSize;
            if (TryFromString<i32>(value, pageSize)) {
                rv.PageSize = pageSize;
            } else {
                ythrow yexception() << "Invalid value for experiment " << key;
            }
        } else if (key == EXP_HW_MUSIC_THIN_CLIENT_FIND_TRACK_IDX_PAGE_SIZE_PREFIX) {
            i32 findTrackIdxPageSize;
            if (TryFromString<i32>(value, findTrackIdxPageSize)) {
                rv.FindTrackIdxPageSize = findTrackIdxPageSize;
            } else {
                ythrow yexception() << "Invalid value for experiment " << key;
            }
        }
        return true;
    });
    return rv;
}

} // namespace NAlice::NHollywood::NMusic
