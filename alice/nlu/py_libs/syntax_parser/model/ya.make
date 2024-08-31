UNION()

INCLUDE(${ARCADIA_ROOT}/alice/nlu/py_libs/syntax_parser/model/resource.make)

FROM_SANDBOX(
    ${SYNTAX_PARSER_RESOURCE_ID}
    OUT_NOAUTO
    embedder.pb
    model.pb
    parser_config.json
    vocab/config.json
    vocab/grammar_value_tags.txt
    vocab/head_tags.txt
    vocab/lemmatizer_info.json
    vocab/token_characters.txt
)

END()
