from alice.uniproxy.library.utils.tree import value_by_path
from alice.uniproxy.library.utils.tree import dict_at_path
from alice.uniproxy.library.utils.tree import replace_tag_value


def test_try_to_use_not_dict():
    t = 'tag'
    t_correct = 'tag'
    replace_tag_value(t, 'adsf', '1324')
    assert t == t_correct


def test_nothing_to_replace():
    t = {'tag': 'value'}
    t_correct = {'tag': 'value'}
    replace_tag_value(t, 'adsf', '1324')
    assert t == t_correct


def test_replace_all_tags_in_different_deeps():
    t = {'tag': [{'ftag': 1}, {'asdf': {'ftag': 2}}]}
    t_correct = {'tag': [{'ftag': 1234}, {'asdf': {'ftag': 1234}}]}
    replace_tag_value(t, 'ftag', 1234)
    assert t == t_correct


def test_replace_different_types():
    t = {'tag': 1}
    t_correct = {'tag': 1}
    replace_tag_value(t, 'ftag', 'asdf')
    assert t == t_correct


def test_replace_list():
    t = {'tag': [1, 2, 3]}
    t_correct = {'tag': [4, 5, 6]}
    replace_tag_value(t, 'tag', [4, 5, 6])
    assert t == t_correct


def test_tags_with_equal_names():
    t = {'tag': {'tag': {'tag': 'value'}}}
    t_correct = {'tag': {'tag': {'tag': 'v'}}}
    replace_tag_value(t, 'tag', 'v')
    assert t == t_correct


TEST_DATA = {
    'foo': {
        'bar': {
            'baz': 10,
            'xxx': 'string',
            'yyy': object(),
        }
    }
}


def test_value_by_path():
    assert value_by_path(TEST_DATA, ('foo', 'bar', 'baz')) == 10
    assert value_by_path(TEST_DATA, ('foo', 'bar', 'xxx')) == 'string'
    assert isinstance(value_by_path(TEST_DATA, ('foo', 'bar', 'yyy')), object)
    assert isinstance(value_by_path(TEST_DATA, ('foo', 'bar')), dict)
    assert isinstance(value_by_path(TEST_DATA, ()), dict)
    assert 'foo' in value_by_path(TEST_DATA, ())


def test_no_value_by_path():
    assert value_by_path(TEST_DATA, ('foo', 'bar', 'zzz')) is None
    assert value_by_path(TEST_DATA, ('foo', 'bar', 'zzz', 'yyy')) is None


def test_dict_at_path():
    assert isinstance(dict_at_path(TEST_DATA, ('foo',)), dict)
    assert isinstance(dict_at_path(TEST_DATA, ('foo', 'bar')), dict)
    assert isinstance(dict_at_path(TEST_DATA, ()), dict)


def test_no_dict_at_path():
    assert dict_at_path(TEST_DATA, ('foo', 'bar', 'baz')) is None
    assert dict_at_path(TEST_DATA, ('foo', 'bar', 'xxx')) is None
    assert dict_at_path(TEST_DATA, ('foo', 'bar', 'yyy')) is None
    assert dict_at_path(TEST_DATA, ('foo', 'xxx')) is None
