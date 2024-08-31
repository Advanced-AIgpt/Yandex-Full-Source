#!/usr/bin/env python
# encoding: utf-8
import sys


class Reporter(object):
    def __init__(self, cmpr, metrics):
        """
        Вывод сводных метрик
        :param Comparative cmpr:
        :param list[(str, Callable)] metrics: [('Human readable name', lambda store: calculated_value)]
        """
        self.cmpr = cmpr
        self.metrics = metrics

    def print_results(self):
        """Вывод результатов в консоль"""
        self._write(out=sys.stdout)

    def dump_to_file(self, path):
        """Вывод результатов в файл"""
        with open(path, 'w') as out:
            self._write(out)

    def _write(self, out):
        raise NotImplementedError


class WikiTableReporter(Reporter):
    """
    Вывод в формате вики-разметки
    """
    def _write(self, out):
        out.write('#|\n')
        for name, calc in self.metrics:
            out.write('|| {} '.format(name))
            for store in self.cmpr.stores.itervalues():
                out.write('| {} '.format(calc(store)))
            out.write('||\n')
        out.write('|#\n')

