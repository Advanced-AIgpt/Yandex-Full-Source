from collections.abc import Mapping

from alice.uniproxy.library.utils.experiments import conducting_experiment, mm_experiment_value
from alice.uniproxy.library.utils.experiments import safe_experiments_vins_format


def test_conducting_experiment():
    assert conducting_experiment('foo', {
        'request': {
            'experiments': {
                'foo': True
            }
        }
    })


def test_no_conducting_experiment():
    assert not conducting_experiment('bar', {
        'request': {
            'experiments': {
                'foo': True
            }
        }
    })


def test_safe_experiments_vins_format():
    warns = []

    def warn(s):
        warns.append(s)

    experiments = safe_experiments_vins_format(1, warn)
    assert isinstance(experiments, Mapping)
    assert len(experiments) == 0
    assert len(warns) == 1

    warns.clear()
    experiments = safe_experiments_vins_format(['a', 1, 'b', {}, None, 'c', [], ()], warn)
    assert isinstance(experiments, Mapping)
    assert len(experiments) == 3
    assert len(warns) == 1

    warns.clear()
    experiments = safe_experiments_vins_format(['a', ''], warn)
    assert isinstance(experiments, Mapping)
    assert len(experiments) == 2
    assert len(warns) == 0

    warns.clear()
    experiments = safe_experiments_vins_format({'a': 'a_', 'b': 'b_', 'c': None, 'd': 1, 'g': {}, 'h': []}, warn)
    assert isinstance(experiments, Mapping)


def test_mm_experiment_value():
    payload = {
        'request': {
            'experiments': {'a': '1', 'b': '1'}
        }
    }
    value = mm_experiment_value('b', payload)
    assert value is None

    payload['request']['experiments'] = {'a': '1', 'b=': '1'}
    value = mm_experiment_value('b', payload)
    assert value == ''

    payload['request']['experiments'] = {'a': '1', 'b': '1', 'b=42': '1'}
    uaas_flags = {'timeout=500': '1', 'abacaba': 'dabacaba'}
    value = mm_experiment_value('b', payload, uaas_flags)
    assert value == '42'

    value = int(mm_experiment_value('timeout', payload, uaas_flags))
    assert value == 500
