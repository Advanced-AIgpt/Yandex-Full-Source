#!/usr/bin/env python
# encoding: utf-8
"""
Тестовый скриптик, который должен "просто работать" с помощью кубика "run python from repo"
"""
from utils.nirvana.op_caller import call_as_operation


def worker(just_arg="for_overriding", inp_arg=None):
    return {'my_arg': just_arg, 'from_input': inp_arg}


if __name__ == '__main__':
    call_as_operation(worker, input_spec={'inp_arg': {'required': False,
                                                      'parser': 'json'}})
