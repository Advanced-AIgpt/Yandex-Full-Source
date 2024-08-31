OWNER(g:alice)

UNION()

INCLUDE(${ARCADIA_ROOT}/alice/begemot/lib/fst/data/fst_data_archive/resource.make)

FROM_SANDBOX(
    ${FST_ARCHIVE_RESOURCE_ID}
    OUT_NOAUTO
    fst/ru/date/date.fst
    fst/ru/date/flags.txt
    fst/ru/date/sequence.txt
    fst/ru/date/symbols.sym
)

END()
