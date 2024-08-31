LIBRARY()

OWNER(g:hollywood)

PEERDIR(
    alice/hollywood/library/common_nlg
)

COMPILE_NLG(
    game_suggest_ru.nlg
    movie_akinator_div_cards_ru.nlg
    movie_akinator_ru.nlg
    movie_suggest_ru.nlg
)

END()
