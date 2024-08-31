# coding: utf-8

from __future__ import unicode_literals

import pytest

from alice.nlg.library.python.codegen.call import resolve_param_values
from alice.nlg.library.python.codegen import errors
from alice.nlg.library.python.codegen import nodes


def kwargs(**kw):
    """Wraps the given keys and values into Keyword nodes.
    """
    return [
        nodes.Keyword(key, nodes.Const(value))
        for key, value in kw.iteritems()
    ]


def consts(*args):
    """Wraps the given values into Const nodes.
    """
    return [
        nodes.Const(arg)
        for arg in args
    ]


@pytest.mark.parametrize(
    'expected,params,defaults,args,kwargs',
    [
        (consts(1, 2), ['a', 'b'], [], consts(1, 2), []),
        (consts(1, 2), ['a', 'b'], [], consts(1), kwargs(b=2)),
        (consts(1, 2), ['a', 'b'], [], [], kwargs(a=1, b=2)),
        (consts(1, 2), ['a', 'b'], consts(2), consts(1), []),
        (consts(1, 2), ['a', 'b'], consts(2), [], kwargs(a=1)),
        (consts(1, 3), ['a', 'b'], consts(2), consts(1, 3), []),
        (consts(1, 3), ['a', 'b'], consts(2), consts(1), kwargs(b=3)),
        (consts(1, 3), ['a', 'b'], consts(2), consts(), kwargs(a=1, b=3)),
    ]
)
def test_resolve_param_values_good(expected, params, defaults, args, kwargs):
    assert expected == resolve_param_values('test', params, defaults, args, kwargs)


@pytest.mark.parametrize(
    'params,defaults,args,kwargs',
    [
        (['a', 'b'], [], consts(1), []),  # too few positional args
        (['a', 'b'], [], consts(1, 2, 3), []),  # too many positional args
        (['a'], [], consts(1), kwargs(a=2)),  # multiple values for a
        (['a'], [], consts(1), kwargs(b=2)),  # non-existent kwarg
    ]
)
def test_resolve_param_values_bad(params, defaults, args, kwargs):
    with pytest.raises(errors.CallResolutionError):
        resolve_param_values('test', params, defaults, args, kwargs)
