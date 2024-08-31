#include "answers.h"

#include <alice/bass/forms/urls_builder.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/library/music/common_special_playlists.h>

#include <library/cpp/cgiparam/cgiparam.h>

namespace NBASS::NMusic {

const THashMap<TStringBuf, NSc::TValue> PERSONAL_SPECIAL_PLAYLISTS_DATA {
    {
        "playlist_of_the_day",
        NSc::TValue::FromJson("{\"title\":\"Плейлист дня\",\"special_playlist\":\"daily\"}")
    },
    {
        "recent_tracks",
        NSc::TValue::FromJson("{\"title\":\"Премьера\",\"special_playlist\":\"premiere\"}")
    },
    {
        "missed_likes",
        NSc::TValue::FromJson("{\"title\":\"Тайник\",\"special_playlist\":\"secret\"}")
    },
    {
        "never_heard",
        NSc::TValue::FromJson("{\"title\":\"Дежавю\",\"special_playlist\":\"dejavu\"}")
    },
    {
        "year_top_2019",
        NSc::TValue::FromJson("{\"title\":\"Перемотка\'19\",\"special_playlist\":\"rewind\"}")
    },
    {
        "kids_rewind",
        NSc::TValue::FromJson("{\"title\":\"Детская перемотка\",\"special_playlist\":\"kidsRewind\"}")
    },
    {
        "summer_top",
        NSc::TValue::FromJson("{\"title\":\"Летняя перемотка\",\"special_playlist\":\"summerTop\"}")
    },
    {
        "rewind10",
        NSc::TValue::FromJson("{\"title\":\"Большая перемотка\",\"special_playlist\":\"rewind10\"}")
    },
    {
        "rewind20",
        NSc::TValue::FromJson("{\"title\":\"Мой 2020\",\"special_playlist\":\"rewind20\"}")
    },
    {
        "summer_top_2021",
        NSc::TValue::FromJson("{\"title\":\"Моё лето '21\",\"special_playlist\":\"summerTop2021\"}")
    },
    {
        "rewind21",
        NSc::TValue::FromJson("{\"title\":\"Мой 2021\",\"special_playlist\":\"rewind21\"}")
    },
    {
        "kinopoisk",
        NSc::TValue::FromJson("{\"title\":\"Киноплейлист\",\"special_playlist\":\"kinopoisk\"}")
    },
    {
        "origin",
        NSc::TValue::FromJson("{\"title\":\"Плейлист с Алисой\",\"special_playlist\":\"origin\"}")
    },
    {
        "podcasts",
        NSc::TValue::FromJson("{\"title\":\"Подкасты недели\",\"special_playlist\":\"podcasts\"}")
    },
    {
        "family",
        NSc::TValue::FromJson("{\"title\":\"Семейный плейлист\",\"special_playlist\":\"family\"}")
    },
    {
        "morningShow",
        NSc::TValue::FromJson("{\"title\":\"Утреннее шоу Алисы\",\"special_playlist\":\"morningShow\"}")
    },
};

void TYandexMusicAnswer::InitWithSearchAnswer(const NSc::TValue& value, bool autoplay) {
    NeedAutoplay = autoplay;

    const NSc::TValue& best = value.Get("best");
    if ((Answer = best.Get("result")).IsDict()) {
        AnswerType = best.Get("type").GetString();
    }
}

void TYandexMusicAnswer::InitWithShazamAnswer(const TString& section, const NSc::TValue& value, bool autoplay) {
    Answer = value;
    NeedAutoplay = autoplay;
    AnswerType = section;
}

void TYandexMusicAnswer::InitWithRelatedAnswer(const TString& section, const NSc::TValue& value, bool autoplay) {
    Answer = value;
    Answer["subtype"] = Answer["type"];
    NeedAutoplay = autoplay;
    AnswerType = section;
}

bool TYandexMusicAnswer::AnswerWithSpecialPlaylist(const TString& playlistName, bool autoplay, NSc::TValue* out) {
    if (const auto* playlist = NAlice::NMusic::GetCommonSpecialPlaylists().FindPtr(playlistName)) {
        Answer = playlist->Convert<NSc::TValue>();
    } else if (PERSONAL_SPECIAL_PLAYLISTS_DATA.contains(playlistName)) {
        Answer = PERSONAL_SPECIAL_PLAYLISTS_DATA.find(playlistName)->second;
    } else {
        return false;
    }
    NeedAutoplay = autoplay;
    AnswerType = Answer.Has("answer_type") ? Answer.Delete("answer_type").GetString() : TStringBuf("playlist");

    return ConvertAnswerToOutputFormat(out);
}

bool TYandexMusicAnswer::MakeSpecialAnswer(TStringBuf specialAnswerRawInfo, bool autoplay, NSc::TValue* out) {
    Answer = NSc::TValue::FromJson(specialAnswerRawInfo);
    if (!Answer.IsDict()) {
        return false;
    }
    NeedAutoplay = autoplay;
    AnswerType = Answer.Delete("answer_type");

    return ConvertAnswerToOutputFormat(out);
}

bool TYandexMusicAnswer::ConvertAnswerToOutputFormat(NSc::TValue* value) {
    if (AnswerType.empty()) {
        return false;
    }

    (*value)["type"] = AnswerType;
    return MakeOutputFromAnswer(value);
}

} // namespace NBASS::NMusic
