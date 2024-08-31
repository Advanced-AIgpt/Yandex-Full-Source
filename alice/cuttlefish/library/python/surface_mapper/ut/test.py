from alice.cuttlefish.library.python.surface_mapper import (
    Mapper,
)

import pytest


@pytest.fixture(scope='function')
def mapper():
    yield Mapper()


def test_none(mapper):
    assert mapper.try_map('some.other.fake.app.id') is None


def test_try_map(mapper):
    res = mapper.try_map('ru.yandex.mobile.search.ipad.inhouse')
    assert res.surface == 'browser'
    assert res.vendor == 'yandex'
    assert res.main_platform == 'ios'
    assert res.main_type == 'beta'
    assert res.all_platforms == ['ios']
    assert res.all_types == ['beta']
    assert res.platforms == ['ios']
    assert res.types == ['beta']


def test_map(mapper):
    res = mapper.map('ru.yandex.quasar.app')
    assert res.surface == 'quasar'
    assert res.vendor == 'yandex'
    assert res.main_platform == 'linux'
    assert res.main_type == 'all'
    assert res.all_platforms == ['linux']
    assert res.all_types == ['all', 'public', 'prod', 'nonprod', 'beta', 'dev']
    assert res.platforms == ['linux']
    assert res.types == ['all']
