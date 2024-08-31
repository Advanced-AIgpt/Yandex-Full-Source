# coding: utf-8
from __future__ import unicode_literals

import pytest

from vins_core.dm.request import Experiments
from vins_core.dm.request import configure_experiment_flags


def test_experiments_contains_deprecation():
    experiments = Experiments(('foo', 'bar', 'baz'))
    with pytest.raises(DeprecationWarning):
        assert 'foo' in experiments

    with pytest.raises(DeprecationWarning):
        for exp in experiments:
            assert exp


@pytest.mark.parametrize('cur,mod,expected', [
    ({'env_flag': '1'}, {'flag': ''}, {'env_flag': '1', 'flag': ''}),
    ({'env_flag': '1'}, {'flag': None}, {'env_flag': '1', 'flag': None}),
    ({'env_flag': '1'}, {}, {'env_flag': '1'}),
    ({'env_flag': '1'}, {'env_flag': None}, {'env_flag': None}),
    ({'env_flag': None}, {'env_flag': '1'}, {'env_flag': '1'})
])
def test_configure_experiment_flags(cur, mod, expected):
    assert configure_experiment_flags(Experiments(cur), mod).to_dict() == expected
