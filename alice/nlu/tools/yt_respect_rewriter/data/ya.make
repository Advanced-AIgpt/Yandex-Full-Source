UNION()

INCLUDE(${ARCADIA_ROOT}/alice/nlu/py_libs/syntax_parser/model/resource.make)
INCLUDE(${ARCADIA_ROOT}/alice/nlu/py_libs/respect_rewriter/models/resource.make)

FROM_SANDBOX(
    FILE
    ${SYNTAX_PARSER_RESOURCE_ID}
    OUT_NOAUTO
    model.tar
)

FROM_SANDBOX(
    FILE
    ${EMBEDDINGS_RESOURCE_ID}
    OUT_NOAUTO
    embeddings.tar
)

FROM_SANDBOX(
    FILE
    ${CLASSIFIER_RESOURCE_ID}
    OUT_NOAUTO
    classifier_model.tar
)

END()
