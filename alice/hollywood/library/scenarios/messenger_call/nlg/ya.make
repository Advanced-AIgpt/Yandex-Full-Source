LIBRARY()

PEERDIR(
    alice/hollywood/library/common_nlg
)

OWNER(
    akastornov
    g:hollywood
)

COMPILE_NLG(
    cards/contacts_card_ru.nlg
    messenger_call_ru.nlg
    phone_call_ru.nlg
)

END()
