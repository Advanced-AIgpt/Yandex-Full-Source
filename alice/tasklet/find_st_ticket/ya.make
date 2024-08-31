PY3_PROGRAM()

OWNER(danichk)

PY_MAIN(tasklet.cli.main)

PY_SRCS(
    impl/__init__.py
)

TASKLET_REG(find_st_ticket py alice.tasklet.find_st_ticket.impl:FindStTicketImpl)

PEERDIR(
    alice/tasklet/find_st_ticket/proto
    tasklet/cli
    library/python/startrek_python_client
    tasklet/services/yav
    tasklet/services/ci
    contrib/python/aiohttp
)

END()
