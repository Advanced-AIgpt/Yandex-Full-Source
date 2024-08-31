LIBRARY()

OWNER(
    klim-roma
    isiv
    petrk
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/common_nlg
    alice/hollywood/library/personal_data
)

COMPILE_NLG(
    voiceprint_common_ru.nlg
    voiceprint_enroll_ru.nlg
    voiceprint_enroll__collect_voice_ru.nlg
    voiceprint_enroll__finish_ru.nlg
    voiceprint_enroll_multiacc__finish_success_ru.nlg
    voiceprint_irrelevant_ru.nlg
    voiceprint_remove__confirm_ru.nlg
    voiceprint_remove__finish_ru.nlg
    voiceprint_remove__unknown_user_ru.nlg
    voiceprint_set_my_name_ru.nlg
    voiceprint_what_is_my_name_ru.nlg
)

END()
