OWNER(g:alice)

UNION()

INCLUDE(${ARCADIA_ROOT}/alice/begemot/lib/fst/data/fst_data_archive/resource.make)

FROM_SANDBOX(
    ${FST_ARCHIVE_RESOURCE_ID}
    OUT_NOAUTO
    fst/ru/time/time.fst
    fst/ru/time/flags.txt
    fst/ru/time/sequence.txt
    fst/ru/time/symbols.sym
)

END()
