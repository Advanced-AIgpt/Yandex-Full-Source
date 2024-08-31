PY2MODULE(client PREFIX "")
EXPORTS_SCRIPT(client.exports)

OWNER(gusev-p)

BUILDWITH_CYTHON_CPP(
    _client.pyx
    --module-name client
    --cleanup 2
)

PEERDIR(
    library/cpp/eventlog
    alice/rtlog/client
    alice/rtlog/protos
)

STRIP()

END()
