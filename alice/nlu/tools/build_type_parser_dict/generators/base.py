# coding: utf-8
import os

BASE_DIR = os.path.dirname(os.path.realpath(__file__))

class GeneratorBase(object):
    def __init__(self, name):
        self._name = name

        extra_dict_path = os.path.join(BASE_DIR, '..', 'extra_dicts', name + '.tsv')
        self._extra_dict_path = extra_dict_path if os.path.isfile(extra_dict_path) else None

    def generate(self):
        pattern_to_value = self._generate()
        if self._extra_dict_path is not None:
            with open(self._extra_dict_path) as f:
                for line in f:
                    pattern, value = line.split('\t')
                    pattern_to_value[pattern] = value
        return pattern_to_value

    def _generate(self):
        raise NotImplementedError()
