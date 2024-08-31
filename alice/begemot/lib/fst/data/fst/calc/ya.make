OWNER(g:alice)

UNION()

INCLUDE(${ARCADIA_ROOT}/alice/begemot/lib/fst/data/fst_data_archive/resource.make)

FROM_SANDBOX(
    ${FST_ARCHIVE_RESOURCE_ID}
    OUT_NOAUTO
    fst/ru/calc/calc.fst
    fst/ru/calc/flags.txt
    fst/ru/calc/sequence.txt
    fst/ru/calc/symbols.sym
)

END()
