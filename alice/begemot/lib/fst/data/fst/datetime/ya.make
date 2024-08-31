OWNER(g:alice)

UNION()

INCLUDE(${ARCADIA_ROOT}/alice/begemot/lib/fst/data/fst_data_archive/resource.make)

FROM_SANDBOX(
    ${FST_ARCHIVE_RESOURCE_ID}
    OUT_NOAUTO
    fst/ru/datetime/datetime.fst
    fst/ru/datetime/flags.txt
    fst/ru/datetime/sequence.txt
    fst/ru/datetime/symbols.sym
    fst/tr/datetime/datetime.fst
    fst/tr/datetime/flags.txt
    fst/tr/datetime/sequence.txt
    fst/tr/datetime/symbols.sym
)

END()
