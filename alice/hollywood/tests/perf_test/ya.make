PY3TEST()

OWNER(g:hollywood)

SIZE(LARGE)

USE_RECIPE(alice/hollywood/library/python/hollywood_recipe/hollywood_recipe)

TEST_SRCS(test_run_yandex-tank.py)

TAG(
    ya:fat ya:force_sandbox ya:sandbox_coverage
)

REQUIREMENTS(
    cpu:16
    sb_vault:S3_KEY=value:LOAD:aws-access-key
    sb_vault:S3_SECRET=value:LOAD:aws-secret-access-key
    network:full
)

DATA(
    sbr://1554867251  #1.ammo
    arcadia/alice/hollywood/tests/perf_test
    arcadia/alice/hollywood/shards
)

DEPENDS(
    load/projects/yandex-tank-package
    alice/hollywood/library/python/hollywood_recipe
    alice/hollywood/scripts/run
    alice/hollywood/shards/all/server
    alice/hollywood/shards/all/prod/resources
    alice/hollywood/shards/all/prod/common_resources
    alice/hollywood/shards/all/prod/fast_data
)

PEERDIR(
    contrib/python/pytest
    contrib/python/PyYAML
    load/projects/yatool_perf_test/lib
)


END()
