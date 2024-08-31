#!/usr/bin/env python
# encoding: utf-8
import sys
import json
from types import FunctionType

if sys.version_info[0] == 3:
    string_types = str,
else:
    string_types = basestring,


def parse_inputs(input_list, input_spec):
    """
    Парсинг и валидация данных, поданных на вход кубику.
    Заодно происходит валидация спецификации.
    :param list[nirvana.job_context.DataItem] input_list: Связи, подключённые ко входу.
    :param dict[str, dict[str, any]] input_spec: Спецификация связей для аргументов.
        Формат описан в call_as_operation
    :return: {"название_аргумента": значение_для_него}
    """
    validate_spec(input_spec)

    inp_values = {name: []
                  for name, item in input_spec.items()
                  if item.get('as_list')}

    fill_inp_values(input_list, inp_values, input_spec)

    validate_inputs(inp_values, input_spec)
    return inp_values


INP_SPEC_OPTIONS = {
    "link_name": lambda v: v is None or isinstance(v, string_types),
    "required": lambda v: isinstance(v, bool),
    "as_list": lambda v: isinstance(v, bool),
    "parser": lambda v: isinstance(v, FunctionType) or v in (None, 'path', 'json', 'text')
}


def validate_spec(input_spec):
    for arg_name, opts in input_spec.items():
        for opt, val in opts.items():
            try:
                is_valid = INP_SPEC_OPTIONS[opt]
            except KeyError:
                raise UserWarning('Unknown option "%s" for arg "%s"' % (opt, arg_name))
            else:
                if not is_valid(val):
                    raise UserWarning('Option "%s" for arg "%s" has invalid value: %s' % (opt, arg_name, val))


def validate_inputs(inp_values, input_spec):
    for name, opts in input_spec.items():
        if opts.get('required', True):
            if opts.get('as_list', False):
                if not inp_values[name]:
                    raise UserWarning('At least one value expected for arg "%s"' % name)
            else:
                if name not in inp_values:
                    raise UserWarning('Arg "%s" is required' % name)


def fill_inp_values(input_list, inp_values, input_spec):
    links2args = map_links_to_args(input_spec)

    for inp in input_list:
        link_name = inp.get_link_name()
        try:
            arg_name = links2args[link_name]
        except KeyError:
            if link_name is None:
                raise UserWarning('Unexpected connection without link_name')
            else:
                raise UserWarning('Unexpected link_name: %s' % link_name)
        else:
            opts = input_spec[arg_name]

        value = parse_value(inp, opts.get('parser'))

        if opts.get('as_list', False):
            inp_values[arg_name].append(value)
        else:
            if arg_name in inp_values:
                raise UserWarning('Duplicated connection for scalar arg "%s"' % arg_name)
            inp_values[arg_name] = value


def map_links_to_args(input_spec):
    links2args = {}
    for arg_name, opts in input_spec.items():
        link_name = opts.get('link_name', None)
        if link_name in links2args:
            raise UserWarning('Duplicated link names in specification: %s' % link_name)
        links2args[link_name] = arg_name
    return links2args


def parse_value(inp, parser_name):
    if parser_name is None or parser_name == 'path':
        return inp.get_path()
    elif parser_name == 'json':
        reader = json.load
    elif parser_name == 'text':
        reader = lambda inp: inp.read()
    elif isinstance(parser_name, FunctionType):
        reader = parser_name
    else:
        raise UserWarning('Unknown parser option: %s' % parser_name)

    with open(inp.get_path()) as f:
        return reader(f)
