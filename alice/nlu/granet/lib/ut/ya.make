UNITTEST_FOR(alice/nlu/granet/lib)

OWNER(
    samoylovboris
    g:alice_quality
)

SRCS(
    granet_tester.cpp
    part00_sandbox.cpp
    part01_basic.cpp
    part02_operators.cpp
    part03_wildcard.cpp
    part04_morphology.cpp
    part05_fillers.cpp
    part06_bag.cpp
    part07_forms.cpp
    part08_entities_external.cpp
    part08_entity_finder.cpp
    part09_files.cpp
    part10_ambiguouty.cpp
    part11_compound.cpp
    part12_explosion.cpp
    part13_issues.cpp
    part14_compiler_error.cpp
    part15_begemot_workflow.cpp
    part16_synonyms.cpp
)

PEERDIR(
    alice/nlu/libs/ut_utils
)

RESOURCE(
    data/collection.grnt        /granet/collection.grnt
    data/hello_world.grnt       /granet/hello_world.grnt
    data/music.grnt             /granet/music.grnt
    data/song_author.txt        /granet/song_author.txt
    data/song_name.txt          /granet/song_name.txt
    data/song_words.txt         /granet/song_words.txt
)

DEPENDS(
    alice/nlu/granet/lib/ut/data/batch
)

END()

RECURSE(
    data
)
