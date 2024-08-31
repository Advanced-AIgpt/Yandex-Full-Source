LIBRARY()

OWNER(
    g:hollywood
    khr2
)

PEERDIR(
    alice/hollywood/library/common_nlg
)

COMPILE_NLG(
    get_news_ar__cards.nlg
    get_news_ru__cards.nlg
    get_news_ar__phrases.nlg
    get_news_ru__phrases.nlg
    get_news_ar.nlg
    get_news_ru.nlg
)

END()

RECURSE_FOR_TESTS(ut)
