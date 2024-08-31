OWNER(g:alice)

UNION()

INCLUDE(${ARCADIA_ROOT}/alice/begemot/lib/fst/data/fst_data_archive/resource.make)

FROM_SANDBOX(
    ${FST_ARCHIVE_RESOURCE_ID}
    OUT_NOAUTO
    fst/ru/geo/geo.fst
    fst/ru/geo/flags.txt
    fst/ru/geo/maps.json
    fst/ru/geo/sequence.txt
    fst/ru/geo/symbols.sym
)

END()
