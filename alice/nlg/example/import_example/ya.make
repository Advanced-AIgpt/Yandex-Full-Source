LIBRARY()

OWNER(g:alice)

PEERDIR(
    alice/nlg/example/check_lib
)

COMPILE_NLG(
    inner_lib_ru.nlg
    render_dt_ru.nlg
)

END()
