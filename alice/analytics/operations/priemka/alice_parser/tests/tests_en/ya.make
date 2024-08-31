OWNER(g:alice_analytics)

PY2TEST()

TEST_SRCS(
    test_make_session_en.py
    test_translations.py
)

ENV(LANGUAGE=en)

DATA(
    sbr://2401519850=test_make_session_en  # 03_all_ue2e_baskets_intents_directives.in.json
)

# TODO: check needed peerdirs
PEERDIR(
    contrib/python/pytest
    statbox/nile
    statbox/nile_debug
    statbox/qb2
    yql/library/python

    alice/analytics/operations/priemka/alice_parser/utils
    alice/analytics/operations/priemka/alice_parser/lib
    alice/analytics/operations/priemka/alice_parser/visualize
    alice/analytics/utils/testing_utils
)

END()
