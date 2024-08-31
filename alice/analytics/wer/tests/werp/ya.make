EXECTEST()

OWNER(g:asr_end_to_end)

RUN(NAME Regression
    werp --cwd transcriber --input ${ARCADIA_ROOT}/alice/analytics/wer/tests/werp/in --output werp_out.json
    CANONIZE werp_out.json
    DIFF_TOOL alice/analytics/wer/tests/werp_difftool/werp_difftool
)

DEPENDS(
    alice/analytics/wer/werp
    alice/analytics/wer/tests/werp_difftool
)

DATA(
    arcadia/alice/analytics/wer/tests/werp/in
    sbr://1920890105  # model + mt_transcribecmd in folder "transcriber"
)

SIZE(medium)
REQUIREMENTS(cpu:4 ram:32)

END()
