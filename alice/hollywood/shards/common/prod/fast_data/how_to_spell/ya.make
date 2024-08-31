UNION()

OWNER(
    igor-darov
    g:alice
)

RUN_PROGRAM(
    alice/hollywood/convert_proto
    --proto THowToSpellFastDataProto
    --to-binary
    alice/hollywood/shards/common/prod/fast_data/how_to_spell/how_to_spell.pb.txt
    how_to_spell.pb
    IN alice/hollywood/shards/common/prod/fast_data/how_to_spell/how_to_spell.pb.txt
    OUT how_to_spell.pb
)

END()
