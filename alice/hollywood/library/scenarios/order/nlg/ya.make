LIBRARY()

OWNER(g:hollywood)

PEERDIR(
    alice/hollywood/library/common_nlg
)

COMPILE_NLG(
    order_ru.nlg
    notification_ru.nlg
    error_ru.nlg
)

END()