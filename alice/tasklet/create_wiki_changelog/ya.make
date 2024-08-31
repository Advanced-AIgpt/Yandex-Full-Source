PY3_PROGRAM()

OWNER(danichk)

PY_MAIN(tasklet.cli.main)

PY_SRCS(
    impl/__init__.py
)

TASKLET_REG(create_wiki_changelog py alice.tasklet.create_wiki_changelog.impl:CreateWikiChangelogImpl)

PEERDIR(
    alice/tasklet/create_wiki_changelog/proto
    tasklet/cli
    alice/tools/yasm/client/library
    tasklet/services/yav
    tasklet/services/ci
    contrib/python/aiohttp
)

END()
