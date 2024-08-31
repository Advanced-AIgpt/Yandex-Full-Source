# NOTE: This file will be included by a shard that wants the scenario's resources,
# so you must always load files by the absolute path and save them in the current directory.
# You can also make free use of FROM_SANDBOX statements.
# 
# You **cannot** use the FILES statement, but it's actually equivalent to several COPY_FILE statements
# so just use those instead.

UNION()

OWNER(g:hollywood)

COPY_FILE(
    alice/hollywood/library/scenarios/sssss/resources/sssss.txt sssss.txt
)

END()
