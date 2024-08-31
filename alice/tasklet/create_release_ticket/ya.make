PY3_PROGRAM()

OWNER(danichk)

PY_MAIN(tasklet.cli.main)

PY_SRCS(
    impl/__init__.py
)

TASKLET_REG(create_release_ticket py alice.tasklet.create_release_ticket.impl:CreateReleaseTicketImpl)

PEERDIR(
    alice/tasklet/create_release_ticket/proto
    tasklet/cli
    alice/tools/yasm/client/library
    library/python/startrek_python_client
    tasklet/services/yav
    tasklet/services/ci
    contrib/python/aiohttp
)

END()
