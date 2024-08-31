# -*- coding: utf-8 -*-

from __future__ import absolute_import, print_function, division, unicode_literals

import warnings

from nirvana.job_context import context as get_real_context, JOB_CONTEXT_JSON

from .compat import Path, lru_cache, cached_property


__all__ = [
    'is_nirvana',
    'set_expected_inputs',
    'get_inputs',
    'get_named_inputs',
    'get_custom_outputs',
    'get_input',
    'get_named_input',
    'get_custom_output',
]


class JupyterNirvanaOperation(object):
    input_name = 'data'
    input_prefix = 'input_'
    custom_output_prefix = 'custom_output_'
    job_context_path = Path(JOB_CONTEXT_JSON)

    def __init__(self):
        self._expected_inputs_number = None
        self._expected_inputs_names = set()

    @cached_property
    def is_nirvana(self):
        return self.job_context_path.exists()

    @cached_property
    def context(self):
        if not self.is_nirvana:
            raise RuntimeError('trying to acquire nirvana-context outside nirvana operation')

        return get_real_context()

    def set_expected_inputs(self, number, names=None):
        assert isinstance(number, int)
        assert isinstance(names, (type(None), list, tuple, set))

        self._expected_inputs_number = number
        if names:
            self._expected_inputs_names.update(names)

        if self.is_nirvana:
            real_inputs = len(self.get_inputs())
            if real_inputs != self._expected_inputs_number:
                raise RuntimeError(
                    'block expected {} links to "data" input, got {}'
                    .format(self._expected_inputs_names, real_inputs)
                )

            if self._expected_inputs_names:
                real_named_inputs = set(self.get_named_inputs())
                if self._expected_inputs_names != real_named_inputs:
                    raise RuntimeError(
                        'block expected named links {}, but got another: {}'
                        .format(tuple(names), tuple(self.get_named_inputs()))
                    )

    @lru_cache(maxsize=None)
    def get_inputs(self):
        if not self.is_nirvana:
            warnings.warn(
                "method `jupytercloud.nirvana.get_inputs()` returns empty list "
                "when invoked not at Nirvana",
                RuntimeWarning
            )
            return ()

        raw_inputs = self.context.inputs
        if raw_inputs.has(self.input_name):
            data_inputs = raw_inputs.get_list(self.input_name)
            return tuple(Path(path) for path in data_inputs)

        return ()

    @lru_cache(maxsize=None)
    def get_named_inputs(self):
        if not self.is_nirvana:
            warnings.warn(
                "method `jupytercloud.nirvana.get_named_inputs()` returns empty dict "
                "when invoked not at Nirvana",
                RuntimeWarning
            )
            return {}

        raw_inputs = self.context.inputs
        named_inputs = {}
        if raw_inputs.has(self.input_name):
            for item in raw_inputs.get_item_list(self.input_name):
                if item.has_link_name():
                    named_inputs[item.get_link_name()] = Path(item.get_path())

        return named_inputs

    @lru_cache(maxsize=None)
    def get_custom_outputs(self):
        if not self.is_nirvana:
            warnings.warn(
                "method `jupytercloud.nirvana.get_custom_outputs()` returns empty list "
                "when invoked not at Nirvana",
                RuntimeWarning
            )
            return []

        custom_outputs = []

        raw_outputs = self.context.outputs
        # XXX: this is only way to get dynamic list of outputs
        for output_name in raw_outputs._items._data:
            if output_name.startswith(self.custom_output_prefix):
                _, _, number = output_name.partition(self.custom_output_prefix)
                if number.isdigit():
                    # path = '/slot/sandbox/d/out/custom_output_0/custom_output_0'
                    path = raw_outputs.get(output_name)
                    custom_outputs.append(Path(path))

        return tuple(sorted(custom_outputs))

    def get_input(self, number, local_path=None):
        """
        NB: нумерация входов в нирване начинается с 0.
        """
        assert isinstance(number, int)
        assert isinstance(local_path, (type(None), str, Path))

        if not self.is_nirvana:
            if not local_path:
                raise RuntimeError(
                    '`jupytercloud.nirvana.get_input` method requires `local_path` '
                    'argument when invoked not at Nirvana'
                )

            if self._expected_inputs_number and number >= self._expected_inputs_number:
                raise RuntimeError(
                    'trying to get local data input #{} '
                    'after invoking `set_expected_inputs({})` '
                    '(in Nirvana input numbering starts with 0)'
                    .format(number, self._expected_inputs_number)
                )

            return Path(local_path)

        inputs = self.get_inputs()

        if number >= len(inputs):
            raise RuntimeError(
                'trying to get data input #{} when operation have {} data inputs overall '
                '(in Nirvana input numbering starts with 0)'
                .format(number, len(inputs))
            )

        expected_input_name = '{}{}'.format(self.input_prefix, number)
        for input_path in inputs:
            if input_path.name == expected_input_name:
                return input_path

        # NB: it may raise only when using "bad" nirvana block,
        # which should not happen at all
        raise RuntimeError(
            'failed to find input file {} at operation data inputs'
            .format(expected_input_name)
        )

    def get_named_input(self, name, local_path=None):
        assert isinstance(name, str)
        assert isinstance(local_path, (type(None), str, Path))

        if not self.is_nirvana:
            if not local_path:
                raise RuntimeError(
                    '`jupytercloud.nirvana.get_named_input` method requires `local_path` '
                    'argument when invoked not at Nirvana'
                )

            if self._expected_inputs_names and name not in self._expected_inputs_names:
                raise RuntimeError(
                    'trying to get named local data input `{}`'
                    'after invoking `set_expected_inputs(..., names={})`'
                    .format(name, self._expected_inputs_names)
                )

            return Path(local_path)

        named_inputs = self.get_named_inputs()
        if name not in named_inputs:
            raise RuntimeError(
                'trying to get named data input `{}` '
                'when operation have only next data inputs: {}'
                .format(name, tuple(named_inputs))
            )

        return named_inputs[name]

    def get_custom_output(self, number, local_path=None):
        """
        NB: нумерация кастомных выходов в нашем кубике начинается
        с 0 для единообразия со входами нирваны.

        """
        assert isinstance(number, int)
        assert isinstance(local_path, (type(None), str, Path))

        if not self.is_nirvana:
            if not local_path:
                raise RuntimeError(
                    '`jupytercloud.nirvana.get_custom_output` method requires `local_path` '
                    'argument when invoked not at Nirvana'
                )

            return Path(local_path)

        custom_outputs = self.get_custom_outputs()

        if len(custom_outputs) < number + 1:
            raise RuntimeError(
                'trying to get data input #{} when operation have {} custom outputs overall'
                .format(number, len(custom_outputs))
            )

        expected_custom_output_name = '{}{}'.format(self.custom_output_prefix, number)
        for custom_output_path in custom_outputs:
            if custom_output_path.name == expected_custom_output_name:
                return custom_output_path

        # NB: it may raise only when using "bad" nirvana block,
        # which should not happen at all
        raise RuntimeError(
            'failed to find output file {} at operation custom outputs'
            .format(expected_custom_output_name)
        )


_JUPYTER_NIRVANA_OPERATION = JupyterNirvanaOperation()

set_expected_inputs = _JUPYTER_NIRVANA_OPERATION.set_expected_inputs
get_inputs = _JUPYTER_NIRVANA_OPERATION.get_inputs
get_named_inputs = _JUPYTER_NIRVANA_OPERATION.get_named_inputs
get_custom_outputs = _JUPYTER_NIRVANA_OPERATION.get_custom_outputs
get_input = _JUPYTER_NIRVANA_OPERATION.get_input
get_named_input = _JUPYTER_NIRVANA_OPERATION.get_named_input
get_custom_output = _JUPYTER_NIRVANA_OPERATION.get_custom_output


def is_nirvana():
    return _JUPYTER_NIRVANA_OPERATION.is_nirvana
