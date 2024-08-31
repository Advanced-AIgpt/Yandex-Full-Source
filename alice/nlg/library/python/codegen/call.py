# coding: utf-8

from __future__ import unicode_literals

from alice.nlg.library.python.codegen import errors


_PARAM_MISSING = object()  # marks a formal parameter with no corresponding argument


def resolve_param_values(name, params, defaults, args, kwargs):
    assert len(params) >= len(defaults)

    if len(args) > len(params):
        raise errors.CallResolutionError(
            'Too many arguments for {!r}'.format(name)
        )

    # initialize param values with the given args, pad to the number of params
    values = args + [_PARAM_MISSING] * (len(params) - len(args))
    assert len(params) == len(values)

    # put each kwarg in its place
    for keyword in kwargs:
        if keyword.key not in params:
            raise errors.CallResolutionError(
                'Invalid argument {!r} for {!r}'.format(keyword.key, name)
            )

        # Python forbids multiple param values, and so do we
        index = params.index(keyword.key)
        if values[index] is not _PARAM_MISSING:
            raise errors.CallResolutionError(
                'Multiple values for a keyword argument {!r} in {!r}'.format(
                    keyword.key,
                    name,
                )
            )

        values[index] = keyword.value

    # finally, put defaults into their places where no values were provided
    for default_index, default in enumerate(defaults):
        index = len(params) - len(defaults) + default_index
        if values[index] is _PARAM_MISSING:
            values[index] = default

    if any(arg is _PARAM_MISSING for arg in values):
        raise errors.CallResolutionError(
            'Too few arguments for {!r}'.format(name)
        )

    return values
