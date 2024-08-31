# This should be INCLUDEd to ya.make of every it2 tests target that uses localhost BASS

ENV(IT2_BASS_INCLUDED=true)

IF(NOT NOCOMPILE)
    DEPENDS(
        alice/bass/bin
        alice/bass/scripts/run
    )
ENDIF()

DATA(
    arcadia/alice/bass/configs/localhost_config.json
)
