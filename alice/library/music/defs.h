#pragma once

#include <alice/library/frame/description.h>

#include <util/generic/hash.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>

namespace NAlice::NMusic {

inline constexpr TStringBuf SLOT_ACTION_REQUEST = "action_request";
inline constexpr TStringBuf SLOT_ACTIVATE_MULTIROOM = "activate_multiroom";
inline constexpr TStringBuf SLOT_ACTIVITY = "activity";
inline constexpr TStringBuf SLOT_ALARM_ID = "alarm_id";
inline constexpr TStringBuf SLOT_ALBUM = "album";
inline constexpr TStringBuf SLOT_AMBIENT_SOUND = "ambient_sound";
inline constexpr TStringBuf SLOT_ANSWER = "answer";
inline constexpr TStringBuf SLOT_ARTIST = "artist";
inline constexpr TStringBuf SLOT_DECLINE = "decline";
inline constexpr TStringBuf SLOT_DISABLE_AUTOFLOW = "disable_autoflow";
inline constexpr TStringBuf SLOT_DISABLE_HISTORY = "disable_history";
inline constexpr TStringBuf SLOT_DISABLE_MULTIROOM = "disable_multiroom";
inline constexpr TStringBuf SLOT_DISABLE_NLG = "disable_nlg";
inline constexpr TStringBuf SLOT_EPOCH = "epoch";
inline constexpr TStringBuf SLOT_FAIRY_TALE = "fairy_tale";
inline constexpr TStringBuf SLOT_FAIRY_TALE_TYPE = "string";
inline constexpr TStringBuf SLOT_FM_RADIO = "fm_radio";
inline constexpr TStringBuf SLOT_FM_RADIO_FREQ = "fm_radio_freq";
inline constexpr TStringBuf SLOT_FROM = "from";
inline constexpr TStringBuf SLOT_GENERATIVE_STATION = "generative_station";
inline constexpr TStringBuf SLOT_GENRE = "genre";
inline constexpr TStringBuf SLOT_IS_FAIRY_TALE_FILTER_GENRE = "is_fairy_tale_filter_genre";
inline constexpr TStringBuf SLOT_IS_FAIRY_TALE_FILTER_GENRE_TYPE = "string";
inline constexpr TStringBuf SLOT_LANGUAGE = "language";
inline constexpr TStringBuf SLOT_LOCATION = "location";
inline constexpr TStringBuf SLOT_LOCATION_ROOM = "location_room";
inline constexpr TStringBuf SLOT_LOCATION_GROUP = "location_group";
inline constexpr TStringBuf SLOT_LOCATION_DEVICE = "location_device";
inline constexpr TStringBuf SLOT_LOCATION_EVERYWHERE = "location_everywhere";
inline constexpr TStringBuf SLOT_LOCATION_SMART_SPEAKER_MODEL = "location_smart_speaker_model";
inline constexpr TStringBuf SLOT_MISSING_TYPE = "missing_type";
inline constexpr TStringBuf SLOT_MODE = "mode";
inline constexpr TStringBuf SLOT_MOOD = "mood";
inline constexpr TStringBuf SLOT_MORNING_SHOW_CONFIG = "morning_show_config";
inline constexpr TStringBuf SLOT_MORNING_SHOW_TYPE = "morning_show_type";
inline constexpr TStringBuf SLOT_NEED_SIMILAR = "need_similar";
inline constexpr TStringBuf SLOT_NOVELTY = "novelty";
inline constexpr TStringBuf SLOT_OBJECT_ID = "object_id";
inline constexpr TStringBuf SLOT_OBJECT_TYPE = "object_type";
inline constexpr TStringBuf SLOT_OFFSET_SEC = "offset_sec";
inline constexpr TStringBuf SLOT_OFFSET = "offset";
inline constexpr TStringBuf SLOT_ORDER = "order";
inline constexpr TStringBuf SLOT_PERSONALITY = "personality";
inline constexpr TStringBuf SLOT_PLAYLIST = "playlist";
inline constexpr TStringBuf SLOT_PLAY_SINGLE_TRACK = "play_single_track";
inline constexpr TStringBuf SLOT_PLAYER_TYPE = "player_type";
inline constexpr TStringBuf SLOT_PLAYER_ACTION_TYPE = "player_action_type";
inline constexpr TStringBuf SLOT_RADIO_SEEDS = "radio_seeds";
inline constexpr TStringBuf SLOT_REPEAT = "repeat";
inline constexpr TStringBuf SLOT_REWIND_TYPE = "rewind_type";
inline constexpr TStringBuf SLOT_SEARCH_TEXT = "search_text";
inline constexpr TStringBuf SLOT_SEARCH_TEXT_TYPE = "string";
inline constexpr TStringBuf SLOT_SET_PAUSE = "set_pause";
inline constexpr TStringBuf SLOT_STATION = "station";
inline constexpr TStringBuf SLOT_STREAM = "stream";
inline constexpr TStringBuf SLOT_SPECIAL_ANSWER_INFO = "special_answer_info";
inline constexpr TStringBuf SLOT_SPECIAL_PLAYLIST = "special_playlist";
inline constexpr TStringBuf SLOT_START_FROM_TRACK_ID = "start_from_track_id";
inline constexpr TStringBuf SLOT_TIME = "time";
inline constexpr TStringBuf SLOT_TRACK_OFFSET_INDEX = "track_offset_index";
inline constexpr TStringBuf SLOT_TRACK_VERSION = "track_version";
inline constexpr TStringBuf SLOT_TRACK = "track";
inline constexpr TStringBuf SLOT_TRACK_INDEX = "track_index";
inline constexpr TStringBuf SLOT_VOCAL = "vocal";

inline constexpr TStringBuf SLOT_ACTION_REQUEST_TYPE = "action_request";
inline constexpr TStringBuf SLOT_ACTIVATE_MULTIROOM_TYPE = "activate_multiroom";
inline constexpr TStringBuf SLOT_ACTIVITY_TYPE = "activity";
inline constexpr TStringBuf SLOT_AMBIENT_SOUND_TYPE = "custom.ambient_sound";
inline constexpr TStringBuf SLOT_CATALOG_SECTION_TYPE = "catalog_section";
inline constexpr TStringBuf SLOT_EPOCH_TYPE = "epoch";
inline constexpr TStringBuf SLOT_GENERATIVE_STATION_TYPE = "custom.music.generative_station";
inline constexpr TStringBuf SLOT_GENRE_TYPE = "genre";
inline constexpr TStringBuf SLOT_HARDCODED_MUSIC_TYPE = "hardcoded_music";
inline constexpr TStringBuf SLOT_LANGUAGE_TYPE = "language";
inline constexpr TStringBuf SLOT_LOCATION_DEVICE_TYPE = "user.iot.device";
inline constexpr TStringBuf SLOT_LOCATION_GROUP_TYPE = "user.iot.group";
inline constexpr TStringBuf SLOT_LOCATION_ROOM_TYPE = "user.iot.room";
inline constexpr TStringBuf SLOT_LOCATION_EVERYWHERE_TYPE = "user.iot.multiroom_all_devices";
inline constexpr TStringBuf SLOT_LOCATION_SMART_SPEAKER_MODEL_TYPE = "user.iot.smart_speaker_model";
inline constexpr TStringBuf SLOT_MOOD_TYPE = "mood";
inline constexpr TStringBuf SLOT_MORNING_SHOW_TYPE_TYPE = "morning_show_type";
inline constexpr TStringBuf SLOT_MORNING_SHOW_CONFIG_TYPE = "string";
inline constexpr TStringBuf SLOT_MUSIC_RESULT_TYPE = "music_result";
inline constexpr TStringBuf SLOT_NEED_SIMILAR_TYPE = "geo_adjective";
inline constexpr TStringBuf SLOT_NOVELTY_TYPE = "novelty";
inline constexpr TStringBuf SLOT_OFFSET_TYPE = "offset";
inline constexpr TStringBuf SLOT_ORDER_TYPE = "order";
inline constexpr TStringBuf SLOT_PERSONALITY_TYPE = "personality";
inline constexpr TStringBuf SLOT_RADIO_SEEDS_TYPE = "string";
inline constexpr TStringBuf SLOT_REPEAT_TYPE = "repeat";
inline constexpr TStringBuf SLOT_SPECIAL_ANSWER_INFO_TYPE = "special_answer_info";
inline constexpr TStringBuf SLOT_SPECIAL_PLAYLIST_TYPE = "special_playlist";
inline constexpr TStringBuf SLOT_STREAM_TYPE = "custom.music.stream";
inline constexpr TStringBuf SLOT_VOCAL_TYPE = "vocal";

inline constexpr TStringBuf MUSIC_PLAY = "personal_assistant.scenarios.music_play";
inline constexpr TStringBuf MUSIC_PLAY_LESS = "personal_assistant.scenarios.music_play_less";
inline constexpr TStringBuf MUSIC_PLAY_MORE = "personal_assistant.scenarios.music_play_more";
inline constexpr TStringBuf MUSIC_PLAY_FIXLIST = "personal_assistant.scenarios.music_play_fixlist";
inline constexpr TStringBuf MUSIC_FAIRY_TALE = "personal_assistant.scenarios.music_fairy_tale";
inline constexpr TStringBuf MUSIC_AMBIENT_SOUND = "personal_assistant.scenarios.music_ambient_sound";
inline constexpr TStringBuf MUSIC_PODCAST = "personal_assistant.scenarios.music_podcast";
inline constexpr TStringBuf RADIO_PLAY = "personal_assistant.scenarios.radio_play";
inline constexpr TStringBuf RADIO_PLAY_POST = "personal_assistant.scenarios.radio_play_post";
inline constexpr TStringBuf MUSIC_SING_SONG = "personal_assistant.scenarios.music_sing_song";

inline constexpr TStringBuf MUSIC_PLAYER_CHANGE_TRACK_NUMBER = "alice.music.change_track_number";
inline constexpr TStringBuf MUSIC_PLAYER_CHANGE_TRACK_VERSION = "alice.music.change_track_version";
inline constexpr TStringBuf MUSIC_PLAYER_CONTINUE = "alice.music.continue";
inline constexpr TStringBuf PLAYER_NEXT_TRACK = "personal_assistant.scenarios.player.next_track";
inline constexpr TStringBuf PLAYER_PREV_TRACK = "personal_assistant.scenarios.player.previous_track";
inline constexpr TStringBuf PLAYER_CONTINUE = "personal_assistant.scenarios.player.continue";
inline constexpr TStringBuf PLAYER_WHAT_IS_PLAYING = "personal_assistant.scenarios.player.what_is_playing";
inline constexpr TStringBuf PLAYER_SEND_SONG_TEXT = "alice.music.send_song_text";
inline constexpr TStringBuf PLAYER_SONGS_BY_THIS_ARTIST = "alice.music.songs_by_this_artist";
inline constexpr TStringBuf PLAYER_WHAT_ALBUM_IS_THIS_SONG_FROM = "alice.music.what_album_is_this_song_from";
inline constexpr TStringBuf PLAYER_WHAT_IS_THIS_SONG_ABOUT = "alice.music.what_is_this_song_about";
inline constexpr TStringBuf PLAYER_WHAT_YEAR_IS_THIS_SONG = "alice.music.what_year_is_this_song";
inline constexpr TStringBuf PLAYER_LIKE = "personal_assistant.scenarios.player.like";
inline constexpr TStringBuf PLAYER_DISLIKE = "personal_assistant.scenarios.player.dislike";
inline constexpr TStringBuf PLAYER_SHUFFLE = "personal_assistant.scenarios.player.shuffle";
inline constexpr TStringBuf PLAYER_UNSHUFFLE = "personal_assistant.scenarios.player.unshuffle";
inline constexpr TStringBuf PLAYER_REPLAY = "personal_assistant.scenarios.player.replay";
inline constexpr TStringBuf PLAYER_REWIND = "personal_assistant.scenarios.player.rewind";
inline constexpr TStringBuf PLAYER_REPEAT = "personal_assistant.scenarios.player.repeat";

inline constexpr TStringBuf MUSIC_PLAY_OBJECT = "quasar.music_play_object";

inline constexpr TStringBuf ATTENTION_UNVERIFIED_PLAYLIST = "unverified_playlist";
inline constexpr TStringBuf ATTENTION_USED_SAVED_PROGRESS = "used_saved_progress";
inline constexpr TStringBuf ATTENTION_CAN_START_FROM_THE_BEGINNING = "can_start_from_the_beginning";

inline constexpr TStringBuf ERROR_CODE_EXTRA_PROMO_PERIOD_AVAILABLE = "extra_promo_period_available";
inline constexpr TStringBuf ERROR_CODE_PROMO_AVAILABLE = "promo_available";

} // namespace NAlice::NMusic
