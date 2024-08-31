PY3_LIBRARY()

OWNER(g:wonderlogs)

PEERDIR(
    contrib/python/click
    contrib/python/protobuf
    mapreduce/yt/interface/protos
)

PY_SRCS(
    proto_checker.py
)

END()
