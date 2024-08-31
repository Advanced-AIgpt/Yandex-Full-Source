LIBRARY()

OWNER(g:alice)

SRCS(
    archive_data_loader.cpp
    decoder.cpp
    fst_base.cpp
    fst_base_value.cpp
    fst_calc.cpp
    fst_custom.cpp
    fst_custom_hierarchy.cpp
    fst_date_time_range.cpp
    fst_date_time.cpp
    fst_float.cpp
    fst_geo.cpp
    fst_map_mixin.cpp
    fst_normalizer.cpp
    fst_num.cpp
    fst_post.cpp
    fst_time.cpp
    fst_units.cpp
    fst_weekdays.cpp
    prefix_data_loader.cpp
    tokenize.cpp
)

PEERDIR(
    alice/bitbucket/pynorm/normalize
    contrib/libs/re2
    library/cpp/archive
    library/cpp/json
    library/cpp/langs
    library/cpp/scheme
)

END()

RECURSE_FOR_TESTS(tests)
