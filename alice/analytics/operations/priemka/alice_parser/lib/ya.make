OWNER(g:alice_analytics)

PY23_LIBRARY()

PEERDIR(
    alice/analytics/operations/priemka/alice_parser/utils
    alice/analytics/operations/priemka/alice_parser/visualize
    alice/analytics/operations/dialog/sessions
    alice/analytics/utils
    alice/analytics/utils/yt
    alice/analytics/utils/auth
    alice/analytics/tasks/VA-571

    statbox/nile
    statbox/qb2
    yql/library/python

    contrib/python/dateutil
    contrib/python/pytz
)

PY_SRCS(
    alice_parser.py
    yql_custom_funcs.py
    utils.py
    make_session.py
    prepare_for_render.py
    hashing.py
)

NO_CHECK_IMPORTS()

END()

RECURSE_ROOT_RELATIVE(
    alice/analytics/operations/priemka/alice_parser/tests
)
