OWNER(g:alice)

UNION()

INCLUDE(${ARCADIA_ROOT}/alice/begemot/lib/fst/data/fst_data_archive/resource.make)

FROM_SANDBOX(
    ${FST_ARCHIVE_RESOURCE_ID}
    OUT_NOAUTO
    fst/ru/weekdays/weekdays.fst
    fst/ru/weekdays/flags.txt
    fst/ru/weekdays/sequence.txt
    fst/ru/weekdays/symbols.sym
)

END()
