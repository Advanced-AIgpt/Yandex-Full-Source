OWNER(g:sda)

PY23_LIBRARY()

PEERDIR(
    alice/analytics/utils
)

PY_SRCS(
    NAMESPACE utils.tv
    app_version.py
)

END()