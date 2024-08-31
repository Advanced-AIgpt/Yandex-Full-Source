OWNER(g:alice)

UNION()

INCLUDE(${ARCADIA_ROOT}/alice/begemot/lib/fst/data/fst_data_archive/resource.make)

FROM_SANDBOX(
    ${FST_ARCHIVE_RESOURCE_ID}
    OUT_NOAUTO
    fst/ru/float/float.fst
    fst/ru/float/flags.txt
    fst/ru/float/sequence.txt
    fst/ru/float/symbols.sym
)

END()
