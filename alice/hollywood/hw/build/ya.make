PY3TEST()

OWNER(g:hollywood)

SIZE(LARGE)

IF(AP OR ALL)
    DEPENDS(
        alice/hollywood/library/python/testing/app_host
    )
    DATA(
        arcadia/apphost/conf
    )
ENDIF()

IF(HW OR ALL)
    DEPENDS(
        alice/hollywood/scripts/run
    )
    INCLUDE(${ARCADIA_ROOT}/alice/hollywood/shards/all/for_it2.inc)
ENDIF()

IF(MM OR ALL)
    DEPENDS(
        alice/megamind/scripts/run
        alice/megamind/server
    )
    DATA(
        arcadia/alice/megamind/configs/common/classification.pb.txt
        arcadia/alice/megamind/configs/dev/combinators
        arcadia/alice/megamind/configs/dev/megamind.pb.txt
        arcadia/alice/megamind/configs/dev/scenarios
    )
ENDIF()

IF(UA OR HW OR MM OR ALL)
    DEPENDS(
        logbroker/unified_agent/bin
    )
ENDIF()

TAG(ya:fat ya:not_autocheck)

END()
