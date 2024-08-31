UNITTEST_FOR(alice/nlu/libs/fst)

OWNER(g:alice)

SIZE(MEDIUM)

SRCS(
    common.cpp
    data.cpp
    decoder_ut.cpp
    fst_base_ut.cpp
    fst_calc_ut.cpp
    fst_custom_ut.cpp
    fst_date_time_range_ut.cpp
    fst_date_time_ru_ut.cpp
    fst_date_time_tr_ut.cpp
    fst_fio_ut.cpp
    fst_float_ut.cpp
    fst_geo_ut.cpp
    fst_num_ru_ut.cpp
    fst_num_tr_ut.cpp
    fst_time_ut.cpp
    fst_units_ut.cpp
    fst_weekdays_ut.cpp
)

DEPENDS(
    alice/begemot/lib/fst/data/denormalizer
    alice/begemot/lib/fst/data/fst/calc
    alice/begemot/lib/fst/data/fst/datetime
    alice/begemot/lib/fst/data/fst/datetime_range
    alice/begemot/lib/fst/data/fst/fio
    alice/begemot/lib/fst/data/fst/float
    alice/begemot/lib/fst/data/fst/num
    alice/begemot/lib/fst/data/fst/time
    alice/begemot/lib/fst/data/fst/units_time
    alice/begemot/lib/fst/data/fst/weekdays
    alice/begemot/lib/fst/data/normalizer
)

ARCHIVE_ASM(
    NAME SoftFstData
    DONTCOMPRESS
    fst/ru/soft/flags.txt
    fst/ru/soft/maps.json
    fst/ru/soft/sequence.txt
    fst/ru/soft/soft.fst
    fst/ru/soft/symbols.sym
)

ARCHIVE_ASM(
    NAME AlbumFstData
    DONTCOMPRESS
    fst/ru/album/album.fst
    fst/ru/album/flags.txt
    fst/ru/album/maps.json
    fst/ru/album/sequence.txt
    fst/ru/album/symbols.sym
    fst/ru/album/weights.json
)

ARCHIVE_ASM(
    NAME GeoFstData
    DONTCOMPRESS
    fst/ru/geo/geo.fst
    fst/ru/geo/flags.txt
    fst/ru/geo/maps.json
    fst/ru/geo/sequence.txt
    fst/ru/geo/symbols.sym
)

ARCHIVE_ASM(
    NAME FloatFstData
    DONTCOMPRESS
    fst/ru/float/float.fst
    fst/ru/float/flags.txt
    fst/ru/float/maps.json
    fst/ru/float/sequence.txt
    fst/ru/float/symbols.sym
)

INCLUDE(${ARCADIA_ROOT}/alice/begemot/lib/fst/data/fst_data_archive/resource.make)

FROM_SANDBOX(${FST_ARCHIVE_RESOURCE_ID} OUT_NOAUTO
    fst/ru/album/album.fst
    fst/ru/album/flags.txt
    fst/ru/album/maps.json
    fst/ru/album/sequence.txt
    fst/ru/album/symbols.sym
    fst/ru/album/weights.json
    fst/ru/float/flags.txt
    fst/ru/float/float.fst
    fst/ru/float/maps.json
    fst/ru/float/sequence.txt
    fst/ru/float/symbols.sym
    fst/ru/geo/flags.txt
    fst/ru/geo/geo.fst
    fst/ru/geo/maps.json
    fst/ru/geo/sequence.txt
    fst/ru/geo/symbols.sym
    fst/ru/soft/flags.txt
    fst/ru/soft/maps.json
    fst/ru/soft/sequence.txt
    fst/ru/soft/soft.fst
    fst/ru/soft/symbols.sym
)

END()
