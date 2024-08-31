#include "common_special_playlists.h"

#include <library/cpp/scheme/scheme.h>

namespace NAlice::NMusic {
namespace {
const THashMap<TStringBuf, TSpecialPlaylistInfo> COMMON_SPECIAL_PLAYLISTS_DATA = {
    {
        TStringBuf("alice"),
        TSpecialPlaylistInfo{"Любимые песни Алисы", "1693", "music-blog", "103372440"}
    },
    {
        TStringBuf("chart"),
        TSpecialPlaylistInfo{"Чарт Яндекс Музыки", "1076", "yamusic-top", "414787002"}
    },
    {
        TStringBuf("ny_alice_playlist"),
        TSpecialPlaylistInfo{"Yet Another New Year", "4924870", "Алиса", "5533796", "electronic"}
    },
    {
        TStringBuf("new"),
        TSpecialPlaylistInfo{"Громкие новинки месяца", "1175", "music-blog", "103372440"}
    },
    {
        TStringBuf("jarahov"),
        TSpecialPlaylistInfo{"Новый год с Джараховым", "1000", "dlgreez.official", "737786368"}
    },
    {
        TStringBuf("gagarina"),
        TSpecialPlaylistInfo{"Новогодний плейлист от Полины", "1000", "gagara1987.official", "738309631"}
    },
    {
        TStringBuf("dakota"),
        TSpecialPlaylistInfo{"Новогоднее настроение от Дакоты", "1000", "ritadakota.official", "738175222"}
    },
    {
        TStringBuf("newyear2019"),
        TSpecialPlaylistInfo{"Новые песни на Новый год", "1947", "music-blog", "103372440"}
    },
    {
        TStringBuf("970829816:1039"),
        TSpecialPlaylistInfo{"Плейлист сказок для Алисы", "1039", "yamusic-podcast", "970829816"}
    },
    {
        TStringBuf("new_year_lalahey"),
        TSpecialPlaylistInfo{"Внутри Лапенко", "1002", "gr1zzzly", "39786586"}
    },
    {
        TStringBuf("nostalgia"),
        TSpecialPlaylistInfo{"Тектоник – это мой электрорай", "1003", "gr1zzzly", "39786586"}
    },
    {
        TStringBuf("meditation_basic"),
        TSpecialPlaylistInfo{"Ежедневная медитация", "10061461", "Практика", "8859435", "meditation"}
    },
    {
        TStringBuf("meditation_evening"),
        TSpecialPlaylistInfo{"Медитация перед сном", "10061464", "Практика", "8859435", "meditation"}
    },
    {
        TStringBuf("meditation_relax"),
        TSpecialPlaylistInfo{"Медитация для снижения стресса", "10061465", "Практика", "8859435", "meditation"}
    },
    {
        TStringBuf("ambient_sounds_default"),
        TSpecialPlaylistInfo{"Звуки природы", "1919", "music-blog", "103372440"}
    },
    {
        TStringBuf("sea_sounds"),
        TSpecialPlaylistInfo{"Шум моря", "1902", "music-blog", "103372440"}
    },
    {
        TStringBuf("rain_sounds"),
        TSpecialPlaylistInfo{"Шум дождя", "1957", "music-blog", "103372440"}
    },
    {
        TStringBuf("bird_sounds"),
        TSpecialPlaylistInfo{"Пение птиц", "1907", "music-blog", "103372440"}
    },
    {
        TStringBuf("bonfire_sounds"),
        TSpecialPlaylistInfo{"Звук костра", "1904", "music-blog", "103372440"}
    },
    {
        TStringBuf("wind_sounds"),
        TSpecialPlaylistInfo{"Шум ветра", "1906", "music-blog", "103372440"}
    },
    {
        TStringBuf("forest_sounds"),
        TSpecialPlaylistInfo{"Шум леса", "1905", "music-blog", "103372440"}
    },
    {
        TStringBuf("fairy_tales_default"),
        TSpecialPlaylistInfo{"Плейлист сказок для Алисы", "1039", "yamusic-podcast", "970829816"}
    },
    {
        TStringBuf("podcasts_default"),
        TSpecialPlaylistInfo{"Подкасты: топ-100", "1104", "yamusic-top", "414787002"}
    },
    {
        TStringBuf("podcasts_child"),
        TSpecialPlaylistInfo{"Подкасты для детей", "1064", "yamusic-podcast", "970829816"}
    },
    {
        TStringBuf("sound_of_stars"),
        TSpecialPlaylistInfo{"Музыка звёзд", "1000", "thesoundofstars", "1399409368"}
    },
};
} // namespace

const THashMap<TStringBuf, TSpecialPlaylistInfo>& GetCommonSpecialPlaylists() {
    return COMMON_SPECIAL_PLAYLISTS_DATA;
}

} // namespace NAlice::NMusic
