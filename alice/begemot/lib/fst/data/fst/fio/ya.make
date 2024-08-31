OWNER(g:alice)

UNION()

INCLUDE(${ARCADIA_ROOT}/alice/begemot/lib/fst/data/fst_data_archive/resource.make)

FROM_SANDBOX(
    ${FST_ARCHIVE_RESOURCE_ID}
    OUT_NOAUTO
    fst/ru/fio/fio.fst
    fst/ru/fio/flags.txt
    fst/ru/fio/maps.json
    fst/ru/fio/sequence.txt
    fst/ru/fio/symbols.sym
)

END()
