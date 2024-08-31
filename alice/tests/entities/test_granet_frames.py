import os
import pytest
import yatest.common as yc

import alice.megamind.protos.common.frame_pb2 as frame_pb
import alice.protos.data.scenario.alice_show.selectors_pb2 as alice_show_selectors_pb2
from granet import Grammar, SourceTextCollection


@pytest.fixture(scope='module')
def grammar():
    # get grammar paths
    grammars_dir = yc.source_path('alice/nlu/data/ru/granet/')
    main_grnt_path = os.path.join(grammars_dir, 'main.grnt')

    # load grammar
    sources = SourceTextCollection()
    with open(main_grnt_path, 'r', encoding='utf8') as f:
        content = f.read().encode('utf8')
        sources.update_main_text(content)
    sources.add_all_from_path(grammars_dir.encode('utf8'))

    return Grammar(sources)


@pytest.fixture(scope='module')
def enum_types(grammar):
    result = {}

    # grammar's data is the descriptor of granet file
    data = grammar.data()

    # element is a nonterminal (starting with `$`, like `$FromThisPoint`, `MakeItSoFiller`, ...)
    # some elements have mappings to enum values
    for e in data.elements():
        # calculate enum type and its values
        def filter_values(arr):
            result = [data.string_pool(idx) for idx in arr]  # map index to string from string pool
            result = [v for v in result if v]  # filter empty values
            return set(result)  # leave unique values
        data_types = filter_values(e.get_data_types())
        data_values = filter_values(e.get_data_values())

        if not data_types:
            continue

        # elements should not have mapping into more than 1 data type
        assert len(data_types) == 1

        # add all enum values to the type
        data_type = list(data_types)[0].decode()
        if data_type not in result:
            result[data_type] = set()
        result[data_type].update(v.decode() for v in data_values)

    return result


@pytest.mark.parametrize('granet_type, proto_type', [
    ('custom.action_request', frame_pb.TActionRequestSlot.EValue),
    ('custom.age', alice_show_selectors_pb2.TAge.EValue),
    ('custom.day_part', alice_show_selectors_pb2.TDayPart.EValue),
    ('custom.fairytale_theme', frame_pb.TFairytaleThemeSlot.EValue),
    ('custom.need_similar', frame_pb.TNeedSimilarSlot.EValue),
    ('custom.order', frame_pb.TOrderSlot.EValue),
    ('custom.repeat', frame_pb.TRepeatSlot.EValue),
    ('custom.target_type', frame_pb.TTargetTypeSlot.EValue),
])
def test_enum_mapping(enum_types, granet_type, proto_type):
    # get values in granet and in proto
    granet_values = enum_types.get(granet_type, {})
    proto_values = set(k for k, v in proto_type.items() if v)

    # check that there is at least one value in both sets (smoke test)
    assert granet_values
    assert proto_values

    # the set of proto values should contain all granet values
    # (that is, proto values may be more "complete")
    assert proto_values.issuperset(granet_values)
