PEERDIR(
    alice/megamind/mit/library
    alice/megamind/mit/library/common/names
    alice/megamind/mit/library/common/util/scenario
    alice/megamind/mit/library/request_builder
)

DEPENDS(
    alice/megamind/mit/library/apphost
    alice/megamind/scripts/run
    alice/megamind/server
)

DATA(
    arcadia/alice/megamind/configs/common/classification.pb.txt
    arcadia/alice/megamind/configs/dev/combinators
    arcadia/alice/megamind/configs/dev/megamind.pb.txt
    arcadia/alice/megamind/configs/dev/scenarios
    arcadia/alice/vins/packages
)

IF(MIT_GENERATOR)
    DATA(
        arcadia/apphost/conf
    )
ELSE()
    DATA(
        arcadia/apphost/conf/backends/ALICE
        arcadia/apphost/conf/verticals/ALICE
    )
    INCLUDE(${ARCADIA_ROOT}/apphost/conf/verticals/ALICE/_alice_external_backends.inc)
    INCLUDE(${ARCADIA_ROOT}/${MODDIR}/eventlog_data.inc)
ENDIF()

DATA(
    sbr://3069941764 # geodata6.bin
    sbr://2719599746 # megamind_formulas
    sbr://2077646575 # partial_preclf_model.cbm
)
