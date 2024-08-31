# This should be INCLUDEd to ya.make of every it2 tests target

IF(NOT NOCOMPILE)
    MESSAGE(Если вы перезапускаете тест, не меняя C++ код, то используйте флаг
            -DNOCOMPILE для более быстрого запуска)
ENDIF()

PEERDIR(
    alice/hollywood/library/framework/proto
    alice/hollywood/library/python/testing/it2
)

IF(NOT NOCOMPILE)
    DEPENDS(
        alice/hollywood/library/python/testing/app_host
    )
ENDIF()

IF(NOT ${HOLLYWOOD_SHARD})
    SET(HOLLYWOOD_SHARD all)
ENDIF()

IF(NOT NOCOMPILE)
    INCLUDE(${ARCADIA_ROOT}/alice/hollywood/shards/${HOLLYWOOD_SHARD}/for_it2.inc)
ENDIF()

IF(NOT CLANG_COVERAGE AND IT2_GENERATOR AND NOT NOCOMPILE)
    DEPENDS(
        alice/megamind/scripts/run
        alice/megamind/server
        apphost/tools/event_log_dump
    )
ENDIF()

DATA(
    arcadia/alice/megamind/configs/common/classification.pb.txt
    arcadia/alice/megamind/configs/dev/combinators
    arcadia/alice/megamind/configs/dev/megamind.pb.txt
    arcadia/alice/megamind/configs/dev/scenarios
)

IF(IT2_GENERATOR)
    DATA(
        arcadia/apphost/conf
    )
ELSE()
    DATA(
        arcadia/apphost/conf/backends/ALICE
        arcadia/apphost/conf/verticals/ALICE
    )
    INCLUDE(${ARCADIA_ROOT}/apphost/conf/verticals/ALICE/_alice_external_backends.inc)
ENDIF()
