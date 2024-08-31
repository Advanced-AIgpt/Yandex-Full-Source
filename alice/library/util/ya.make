LIBRARY()

OWNER(g:alice)

SRCS(
    min_heap.cpp
    rng.cpp
    search_convert.cpp
    status.cpp
    system_time.cpp
    variant.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
