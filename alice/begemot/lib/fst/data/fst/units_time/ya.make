OWNER(g:alice)

UNION()

INCLUDE(${ARCADIA_ROOT}/alice/begemot/lib/fst/data/fst_data_archive/resource.make)

FROM_SANDBOX(
    ${FST_ARCHIVE_RESOURCE_ID}
    OUT_NOAUTO
    fst/ru/units_time/units_time.fst
    fst/ru/units_time/flags.txt
    fst/ru/units_time/sequence.txt
    fst/ru/units_time/symbols.sym
)

END()
