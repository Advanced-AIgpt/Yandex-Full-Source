PY3_PROGRAM()

PY_SRCS(
  MAIN main.py
  prepare_htmls.py
)

PEERDIR(
    voicetech/common/lib
    contrib/python/dominate
    yt/python/client
)


END()
