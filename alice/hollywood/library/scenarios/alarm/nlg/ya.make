LIBRARY()

OWNER(
    g:alice-alarm-scenario
    g:hollywood
    g:alice
)

PEERDIR(
    alice/hollywood/library/common_nlg
    alice/hollywood/library/scenarios/music/nlg
)

COMPILE_NLG(
    alarm__common_ar.nlg
    alarm__common_ru.nlg
    alarm_cancel_ar.nlg
    alarm_cancel_ru.nlg
    alarm_how_long_ar.nlg
    alarm_how_long_ru.nlg
    # TODO(alexanderplat): add alarm_how_to_set_sound_ar.nlg
    alarm_how_to_set_sound_ru.nlg
    alarm_morning_show_error_ar.nlg
    alarm_morning_show_error_ru.nlg
    alarm_reset_sound_ar.nlg
    alarm_reset_sound_ru.nlg
    alarm_set_ar.nlg
    alarm_set_ru.nlg
    alarm_set_alice_show_ar.nlg
    alarm_set_alice_show_ru.nlg
    alarm_set_with_alice_show_ar.nlg
    alarm_set_with_alice_show_ru.nlg
    alarm_set_morning_show_ar.nlg
    alarm_set_morning_show_ru.nlg
    # TODO(alexanderplat): add alarm_set_sound_ar.nlg
    alarm_set_sound_ru.nlg
    alarm_set_with_morning_show_ar.nlg
    alarm_set_with_morning_show_ru.nlg
    alarm_show_ar.nlg
    alarm_show_ru.nlg
    alarm_sound_set_level_ar.nlg
    alarm_sound_set_level_ru.nlg
    alarm_stop_playing_ru.nlg
    alarm_play_alice_show_ar.nlg
    alarm_play_alice_show_ru.nlg
    alarm_play_morning_show_ar.nlg
    alarm_play_morning_show_ru.nlg
    alarm_what_sound_is_set_ar.nlg
    alarm_what_sound_is_set_ru.nlg
    alarm_what_sound_level_is_set_ar.nlg
    alarm_what_sound_level_is_set_ru.nlg

    timer__common_ar.nlg
    timer__common_ru.nlg
    timer_cancel_ar.nlg
    timer_cancel_ru.nlg
    timer_how_long_ar.nlg
    timer_how_long_ru.nlg
    timer_pause_ar.nlg
    timer_pause_ru.nlg
    timer_resume_ar.nlg
    timer_resume_ru.nlg
    timer_set_ar.nlg
    timer_set_ru.nlg
    timer_show_ar.nlg
    timer_show_ru.nlg

    alarm_fallback_ar.nlg
    alarm_fallback_ru.nlg
    suggests_ar.nlg
    suggests_ru.nlg
)

END()
