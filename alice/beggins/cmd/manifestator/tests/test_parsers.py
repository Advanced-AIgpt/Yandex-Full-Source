import pytest

from alice.beggins.cmd.manifestator.internal.parser import (
    StandardParser, UnmarkedParser, AnalyticsGeneralParser, AnalyticsBasketParser,
)
from alice.beggins.cmd.manifestator.internal.model import DataEntry


def test_standard_parser():
    sequence = [
        {'text': 'foo', 'target': 0},
        {'text': 'bar', 'target': 1},
        {'text': 'baz', 'target': 0},
    ]

    tested = list(StandardParser(source='liza').parse(sequence))
    expected = [
        DataEntry(text='foo', target=0, source='liza'),
        DataEntry(text='bar', target=1, source='liza'),
        DataEntry(text='baz', target=0, source='liza'),
    ]

    assert tested == expected


def test_unmarked_parser():
    sequence = [
        {'text': 'foo'},
        {'text': 'bar'},
        {'text': 'baz'},
    ]

    tested = list(UnmarkedParser(target=0, source='tanya').parse(sequence))
    expected = [
        DataEntry(text='foo', target=0, source='tanya'),
        DataEntry(text='bar', target=0, source='tanya'),
        DataEntry(text='baz', target=0, source='tanya'),
    ]

    assert tested == expected


def test_analytics_general_parser():
    sequence = [
        {'utterance': 'foo', 'is_positive': 'N'},
        {'utterance': 'bar', 'is_positive': 'Y'},
        {'utterance': 'baz', 'is_positive': 'N'},
    ]

    tested = list(AnalyticsGeneralParser(source='rita').parse(sequence))
    expected = [
        DataEntry(text='foo', target=0, source='rita'),
        DataEntry(text='bar', target=1, source='rita'),
        DataEntry(text='baz', target=0, source='rita'),
    ]

    assert tested == expected


def test_analytics_basket_parser():
    sequence = [
        {'text': 'foo', 'is_negative_query': 1},
        {'text': 'bar', 'is_negative_query': 0},
        {'text': 'baz', 'is_negative_query': 1},
    ]

    tested = list(AnalyticsBasketParser(source='lera').parse(sequence))
    expected = [
        DataEntry(text='foo', target=0, source='lera'),
        DataEntry(text='bar', target=1, source='lera'),
        DataEntry(text='baz', target=0, source='lera'),
    ]

    assert tested == expected


def test_bad_input_for_standard_parser():
    sequence = [
        {'text': 'foo', 'is_negative_query': 1},
        {'text': 'bar', 'is_negative_query': 0},
        {'text': 'baz', 'is_negative_query': 1},
    ]

    with pytest.raises(Exception):
        list(StandardParser(source='liza').parse(sequence))


def test_unsupported_values_for_standard_parser():
    sequence = [
        {'text': 'foo', 'target': 0},
        {'text': 'bar', 'target': 1},
        {'text': 'baz', 'target': 2},
    ]

    with pytest.raises(ValueError):
        list(StandardParser(source='liza').parse(sequence))
