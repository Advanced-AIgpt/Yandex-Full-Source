OWNER(g:sda)

PY2_LIBRARY()

PEERDIR(
    alice/analytics/utils
    alice/analytics/utils/auth/relative_lib
    alice/analytics/utils/relative_libs/relative_lib_json_utils
    library/python/nirvana
)

PY_SRCS(
    NAMESPACE utils.nirvana
    api.py
    op_caller.py
    op_input.py
)

END()