OWNER(
    smirnovpavel
    g:alice_quality
)

PACKAGE()

PEERDIR(
    alice/nlu/tools/build_type_parser_dict/extra_dicts
    alice/nlu/tools/build_type_parser_dict/generators
)

END()

RECURSE(
    converter
    extra_dicts
    filters
    generators
)
