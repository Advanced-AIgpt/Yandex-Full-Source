OWNER(g:alice)

UNION()

INCLUDE(${ARCADIA_ROOT}/alice/begemot/lib/fst/data/fst_data_archive/resource.make)

FROM_SANDBOX(
    ${FST_ARCHIVE_RESOURCE_ID}
    OUT_NOAUTO
    fst/ru/num/num.fst
    fst/ru/num/flags.txt
    fst/ru/num/sequence.txt
    fst/ru/num/symbols.sym
    fst/tr/num/num.fst
    fst/tr/num/flags.txt
    fst/tr/num/sequence.txt
    fst/tr/num/symbols.sym
)

END()
