LIBRARY()

OWNER(
    {{username}}
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/common_nlg
)

COMPILE_NLG(
    {{scenario_name}}_ru.nlg
)

END()
