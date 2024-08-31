LIBRARY()

OWNER(
    alexanderplat
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/common_nlg
    alice/hollywood/library/scenarios/music/nlg
)

COMPILE_NLG(
    common_ru.nlg
    music_what_is_playing_ru.nlg
    music_what_is_playing__play_ru.nlg
)

END()
