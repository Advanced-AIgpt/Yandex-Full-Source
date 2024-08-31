from alice.hollywood.library.python.testing.hamcrest_ext import has_only_entries, non_empty_dict
from hamcrest import assert_that, equal_to


def test_has_only_entries():
    pyobj = {
        'a': 1,
        'b': 2,
        'c': 3,
    }
    assert_that(pyobj, has_only_entries({
        'a': 1,
        'b': 2,
        'c': equal_to(3),  # Same as just 3
    }))


def test_non_empty_dict():
    pyobj = {'a': 1}
    assert_that(pyobj, non_empty_dict())
