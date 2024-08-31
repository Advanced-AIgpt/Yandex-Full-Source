# coding: utf-8
from __future__ import unicode_literals

import pytest
import yaml

from yaml.constructor import ConstructorError

from personal_assistant.testing_framework import (
    process_file, Token, format_string, split_tokens, load_yaml, parse_placeholders
)
from consts import (
    APP_INFOS, TEST_FRAMEWORK_YAML, CORRECT_TEST_DATA_WITH_APP_INFOS, CORRECT_TEST_DATA_WITHOUT_APP_INFOS
)


def generate_real_test_data(yaml_test_data, test_name, app_infos=None):
    real_test_data = [
        (
            dialog_test_data.name,
            dialog_test_data.freeze_time,
            dialog_test_data.geo_info,
            dialog_test_data.app_info,
            dialog_test_data.device_state,
            [
                [
                    utterance_test_data.request,
                    utterance_test_data.text_regexp,
                    utterance_test_data.voice_regexp,
                    utterance_test_data.meta,
                    utterance_test_data.exact_meta_match,
                    utterance_test_data.suggests,
                    utterance_test_data.exact_suggests_match,
                    utterance_test_data.button_actions,
                    utterance_test_data.directives,
                    utterance_test_data.exact_directives_match,
                    utterance_test_data.vins_form,
                    utterance_test_data.experiments,
                    utterance_test_data.bass_answer
                ]
                for utterance_test_data in dialog_test_data.dialog
            ]
        )
        for dialog_test_data in process_file(yaml_test_data, test_name, app_infos=app_infos)
    ]

    return real_test_data


def test_framework():
    real_test_data = generate_real_test_data(TEST_FRAMEWORK_YAML, 'test', app_infos=APP_INFOS)
    assert CORRECT_TEST_DATA_WITH_APP_INFOS == real_test_data


def test_framework_without_app_infos():
    real_test_data = generate_real_test_data(TEST_FRAMEWORK_YAML, 'test')
    assert CORRECT_TEST_DATA_WITHOUT_APP_INFOS == real_test_data


def test_bad_framework():
    bad_test_framework_yaml = """
    case_group:
      case_name_indistinguishable_from_utterance:
        test_utterance: 'test_answer'
    """

    with pytest.raises(ValueError) as excinfo:
        next(process_file(bad_test_framework_yaml, 'test'))
    expected = 'Empty test data for utterance "case_name_indistinguishable_from_utterance" in test "test::case_group"'
    assert excinfo.value.message == expected


@pytest.mark.parametrize('string, res', [
    ('', []),
    ('{}', [Token('label', '')]),
    ('{}{}', [Token('label', ''), Token('label', '')]),
    ('a {l} b', [Token('str', 'a '), Token('label', 'l'), Token('str', ' b')]),
    ('{a}{b}', [Token('label', 'a'), Token('label', 'b')]),
    ('abc', [Token('str', 'abc')]),
])
def test_self_split(string, res):
    assert list(split_tokens(string)) == res


@pytest.mark.parametrize('string, labels, result', [
    ('{a}', {'a': 1}, '1'),
    ('a {b} c', {'b': 1}, 'a\\ 1\\ c'),
    ('a b', {}, 'a\\ b'),
    ('a {} b', {'': 1}, 'a\\ 1\\ b'),
])
def test_self_format_string(string, labels, result):
    assert format_string(string, labels) == result


@pytest.mark.parametrize('data, match, is_equal', [
    ('a: !Any a\nb: !Any b', {'a': 1, 'b': 2}, True),
    ('a: !Any\nb: !Any', {'a': 1, 'b': 2}, True),
    ('a: !Any a\nb: !Any a', {'a': 1, 'b': 2}, False),
])
def test_any(data, match, is_equal, tmpdir):
    assert (load_yaml(data) == match) == is_equal


@pytest.mark.parametrize('data, labels, expected', [
    ('!Placeholder a', {'a': 123}, 123),
    ('!Placeholder ["a", 123]', {}, 123),
    ('!Placeholder ["a", 123]', {'a': 666}, 666),
    pytest.param('!Placeholder a', {}, 123, marks=pytest.mark.xfail(raises=yaml.YAMLError, strict=True)),
])
def test_placeholder(data, labels, expected):
    assert load_yaml(data, labels) == expected


@pytest.mark.parametrize('data, expected', [
    ('', {}),
    ('a:b', {'a': 'b'}),
    ('a:b;c:d', {'a': 'b', 'c': 'd'}),
    ('abrakadabra', {}),
    ('key1:any value1;key2:any value2', {'key1': 'any value1', 'key2': 'any value2'})
])
def test_parse_placeholders(data, expected):
    assert parse_placeholders(data) == expected


def test_load_duplicate_keys():
    good_yaml = 'a: 1\nb:\n  c: 2\n  d: 2'
    bad_yaml = 'a: 1\nb:\n  c: 2\n  c: 2'
    assert load_yaml(good_yaml) == {'a': 1, 'b': {'c': 2, 'd': 2}}
    with pytest.raises(ConstructorError):
        load_yaml(bad_yaml)
