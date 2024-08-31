OWNER(g:alice)

UNION()

INCLUDE(${ARCADIA_ROOT}/alice/begemot/lib/fst/data/fst_data_archive/resource.make)

FROM_SANDBOX(
    ${FST_ARCHIVE_RESOURCE_ID}
    OUT_NOAUTO
    fst/ru/datetime_range/datetime_range.fst
    fst/ru/datetime_range/flags.txt
    fst/ru/datetime_range/sequence.txt
    fst/ru/datetime_range/symbols.sym
)

END()
