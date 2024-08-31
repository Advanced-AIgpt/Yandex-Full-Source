LIBRARY()

OWNER(
    g:alice-time-capsule-scenario
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/common_nlg
)

COMPILE_NLG(
    time_capsule_delete_ru.nlg
    time_capsule_how_long_ru.nlg
    time_capsule_information_ru.nlg
    time_capsule_interrupt_approve_ru.nlg
    time_capsule_open_ru.nlg
    time_capsule_questions_ru.nlg
    time_capsule_rerecord_ru.nlg
    time_capsule_save_ru.nlg
    time_capsule_save_approve_ru.nlg
    time_capsule_start_approve_ru.nlg
    time_capsule_stop_ru.nlg

    error_ru.nlg
    suggests_ru.nlg
)

END()
