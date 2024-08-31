OWNER(g:alice)

UNION()

INCLUDE(${ARCADIA_ROOT}/alice/begemot/lib/fst/data/fst_data_archive/resource.make)

FROM_SANDBOX(
    ${FST_ARCHIVE_RESOURCE_ID}
    OUT_NOAUTO
    fst/ru/artist/artist.fst
    fst/ru/artist/flags.txt
    fst/ru/artist/maps.json
    fst/ru/artist/sequence.txt
    fst/ru/artist/symbols.sym
    fst/ru/artist/weights.json
)

END()
