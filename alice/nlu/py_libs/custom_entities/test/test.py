from alice.nlu.py_libs import custom_entities
import tempfile
import yatest.common


def create_entity_string(string, type, value):
    return '\t'.join([string, type, value])


class TestCustomEntitySearcher:
    def test_constructor(self):
        with tempfile.NamedTemporaryFile('w') as f_strings, tempfile.NamedTemporaryFile('wb') as f_result:
            f_strings.write(create_entity_string('привет', 'test_entity', 'test_value') + '\n')
            f_strings.flush()
            build_automaton = yatest.common.binary_path('alice/nlu/tools/entities/custom/build_automaton/build_automaton')
            yatest.common.execute((build_automaton, f_strings.name), stdout=f_result)
            f_result.flush()
            searcher = custom_entities.CustomEntitySearcher(yatest.common.binary_path(f_result.name).encode('utf-8'))
        entities = searcher.search('скажи привет мир'.encode('utf-8').split())
        assert len(entities) == 1
        assert entities[0].type == 'test_entity'
        assert entities[0].value == 'test_value'
        assert entities[0].begin == 1
        assert entities[0].end == 2
