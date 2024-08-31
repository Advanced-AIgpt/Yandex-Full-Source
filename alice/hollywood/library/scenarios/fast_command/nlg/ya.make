LIBRARY()

OWNER(nkodosov)

PEERDIR(
    alice/hollywood/library/common_nlg_localized
)

COMPILE_NLG(
    NLG_COMPILER_LOCALIZED_MODE

    clock_face.nlg
    pause_command.nlg
    power_off.nlg
    sound_common.nlg
    sound_get_level.nlg
    sound_mute.nlg
    sound_set_level.nlg
    sound_unmute.nlg
)

END()

RECURSE_FOR_TESTS(ut)
