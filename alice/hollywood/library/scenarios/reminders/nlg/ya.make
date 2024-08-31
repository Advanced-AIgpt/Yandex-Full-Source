LIBRARY()

OWNER(
    petrk
    g:hollywood
)

COMPILE_NLG(
    cancelation_result_ar.nlg
    cancelation_result_en.nlg
    cancelation_result_ru.nlg
    common__ar.nlg
    common__en.nlg
    common__ru.nlg
    creation_result_ar.nlg
    creation_result_en.nlg
    creation_result_ru.nlg
    error_ar.nlg
    error_en.nlg
    error_ru.nlg
    # TODO(alexanderplat): add permission_ar.nlg
    permission_en.nlg
    permission_ru.nlg
    shoot_ar.nlg
    shoot_en.nlg
    shoot_ru.nlg

    vins/common_en.nlg
    vins/common_ru.nlg
    vins/create_reminder_en.nlg
    vins/create_reminder_ru.nlg
    vins/list_reminders_en.nlg
    vins/list_reminders_ru.nlg
    vins/list_reminders__scroll_stop_en.nlg
    vins/list_reminders__scroll_stop_ru.nlg
    vins/reminders__common_en.nlg
    vins/reminders__common_ru.nlg
)

PEERDIR(
    alice/hollywood/library/common_nlg
)

END()
