# This should be included to ya.make of every it2 tests target
IF (NOT YA_IDE_IDEA)
PEERDIR(
    alice/hollywood/library/python/testing/it2
    contrib/python/PyHamcrest
    alice/kronstadt/it2
)

# -------- MEGAMIND --------- #
PEERDIR(
    alice/megamind/protos/scenarios
)

DATA(
    arcadia/alice/megamind/configs/common/classification.pb.txt
    arcadia/alice/megamind/configs/dev/combinators
    arcadia/alice/megamind/configs/dev/megamind.pb.txt
    arcadia/alice/megamind/configs/dev/scenarios
    arcadia/alice/megamind/protos/common
    arcadia/alice/megamind/protos/scenarios
    arcadia/alice/protos/api/renderer
)

# build megamind runner
IF(NOT CLANG_COVERAGE AND IT2_GENERATOR)
    DEPENDS(
        alice/megamind/scripts/run
        alice/megamind/server
    )
ENDIF()
# -------- MEGAMIND --------- #

# -------- APPHOST --------- #
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

# build apphost runner
DEPENDS(
    alice/hollywood/library/python/testing/app_host
)
# -------- APPHOST --------- #
ENDIF()
