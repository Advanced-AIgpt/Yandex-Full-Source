MUSIC_VIDEO_SEARCH = {
    'uniproxy_512_and_x_yandex_vins_ok_as_good_response': True,
    'mm_raise_error_on_failed_scenarios': 'Video;HollywoodMusic;alice.vinsless.music;alice.vinsless.music.general',
    'tunneller_profile_video': 'weak_consistency__video__desktop__hamster',
    'mm_move_tunneller_responses_from_scenarios': 'Video',
    'tunneller_profile': 'weak_consistency__web__desktop__production__tier0_tier1',
    'tunneller_analytics_info': True,
}

NO_RETRY = {}

DEFAULT_PROFILE = NO_RETRY
RETRIES_PROFILES_CONFIG = {
    'music_video_search': MUSIC_VIDEO_SEARCH,
    'no_retry': NO_RETRY,
}


def get_profile_by_name_or_default(name):
    return RETRIES_PROFILES_CONFIG.get(name, DEFAULT_PROFILE)
