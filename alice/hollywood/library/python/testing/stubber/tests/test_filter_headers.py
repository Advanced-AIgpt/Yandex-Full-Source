import logging
import pytest

from alice.hollywood.library.python.testing.stubber.stubber_server import filter_headers, ensure_filter_regex_is_valid

logger = logging.getLogger(__name__)


@pytest.mark.parametrize('headers, filters, expected', [
    (
        {'header-1': '1', 'header-2': '2', 'header-3': '3'},
        ['header-2', 'header-4'],
        {'header-1': '1', 'header-3': '3'}
    ),
    (
        {'Header-1': '1', 'header-2': '2', 'HEADER-3': '3'},
        ['header-2', 'header-4'],
        # Note that resulting keys are all lowercased
        {'header-1': '1', 'header-3': '3'}
    ),
    (
        {'header-1': '1', 'header-2': '2', 'header-3': '3'},
        [r'header-[23]'],
        {'header-1': '1'}
    ),
    (
        {'X-Header-1': '1', 'x-header-2': '2', 'X-HEADER-3': '3', 'x-header-a': 'a', },
        [r'x-header-(\d+)'],
        # Filter digit suffixes
        {'x-header-a': 'a'}
    ),
    (
        {'X-Header-1': '1', 'x-header-2': '2', 'X-HEADER-3': '3', 'x-header-a': 'a', },
        [r'x-header-(\D+)'],
        # Filter non-digit suffixes
        {'x-header-1': '1', 'x-header-2': '2', 'x-header-3': '3'}
    ),
    (
        {'X-Header-1': '1', 'x-header-2': '2', 'X-HEADER-3': '3', 'x-header-a': 'a', 'foo': 'bar'},
        [r'x-header-(\d+)', r'x-header-(\D+)'],
        {'foo': 'bar'},
    ),
    (
        {'X-Header-1': '1', 'x-header-2': '2', 'X-HEADER-3': '3', 'x-header-a': 'a', },
        [r'.*'],
        # Everything is filtered
        {},
    ),
])
def test_filter_headers(headers, filters, expected):
    actual = filter_headers(headers, filters)
    assert actual == expected


@pytest.mark.parametrize('headers, filters', [
    (
        {'header-1': '1', 'header-2': '2', 'header-3': '3'},
        ['Header-2'],
    ),
    (
        {'Header-1': '1', 'header-2': '2', 'HEADER-3': '3'},
        ['HEADER-2'],
    ),
    (
        {'header-1': '1', 'header-2': '2', 'header-3': '3'},
        ['header-(.*)', 'HeadeR-1'],
    ),
])
def test_invalid_filter_header(headers, filters):
    with pytest.raises(Exception):
        filter_headers(headers, filters)


@pytest.mark.parametrize('filter_regex, is_valid', [
    (r'.*', True),
    (r'x-header-a', True),
    (r'x-header-1', True),
    (r'x-header-(\d+)', True),
    (r'x-header-(\D+)', True),
    (r'X-Header-A', False),
    (r'X-Header-1', False),
    (r'X-Header-(\d+)', False),
    (r'X-Header-(\D+)', False),
    (r'X-HEADER-A', False),
    (r'X-HEADER-1', False),
    (r'X-HEADER-(\d+)', False),
    (r'X-HEADER-(\D+)', False),
    (r'X-header-(\D+)', False),
    (r'x-headeR', False),
])
def test_ensure_filter_regex_is_valid(filter_regex, is_valid):
    try:
        ensure_filter_regex_is_valid(filter_regex)
        assert is_valid, f'We expected non-valid filter_regex, but filter `{filter_regex}` did not raise exceptions'
    except Exception as err:
        assert not is_valid, f'We expected valid filter_regex, but exception is rased: {str(err)}'
