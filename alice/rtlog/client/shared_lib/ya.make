DLL(rtlog)

OWNER(gusev-p)

EXPORTS_SCRIPT(rtlog.exports)

SRCS(
    rtlog.cpp
)

PEERDIR(
    alice/rtlog/client
)

CFLAGS(-DFROM_IMPL=1)

END()
