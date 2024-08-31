LIBRARY()

OWNER(d-dima)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/framework/helpers/nlu_features
    alice/hollywood/library/scenarios/search/proto
    alice/library/search_result_parser
    alice/protos/data/scenario
)

SRCS(
    proc_calculator.cpp
    proc_calories.cpp
    proc_factdist.cpp
    proc_factentity.cpp
    proc_factrich.cpp
    proc_factsuggest.cpp
    proc_factold.cpp
    proc_generic.cpp
    proc_location.cpp
    proc_porno.cpp
    processors.cpp
)

END()
