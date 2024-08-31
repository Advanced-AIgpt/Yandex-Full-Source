# -*- coding: utf-8 -*-
"""
Rewriting of https://github.com/ipython/ipython/blob/master/IPython/core/completerlib.py
for Arcadia reality

"""

from __future__ import absolute_import, print_function, division, unicode_literals

import sys
import itertools
import pprint
import six

from importlib import import_module
from IPython.core.completerlib import is_importable
from __res import iter_py_modules


class ArcadiaModuleCompleter(object):
    def __init__(self):
        self.root_modules = None
        self.modules_cache = None

    def late_init(self, shell):
        root_modules = set()
        raw_cache = {}

        all_modules = itertools.chain(
            sys.builtin_module_names,
            iter_py_modules()
        )

        for name in all_modules:
            path = name.split('.')
            root_modules.add(path[0])

            prefix = path.pop(0)
            for element in path:
                if element == '__init__':
                    continue

                raw_cache.setdefault(prefix, set())
                raw_cache[prefix].add(element)

                prefix = prefix + '.' + element

        self.root_modules = tuple(root_modules)
        self.modules_cache = {
            k: tuple(v) for k, v in six.iteritems(raw_cache)
        }

        shell.log.debug(
            '%d root modules found for autocompletion:\n%s',
            len(self.root_modules),
            self.root_modules,
        )
        shell.log.debug(
            '%d modules (which have submodules) loaded to cache:\n%s',
            len(self.modules_cache),
            pprint.pformat(self.modules_cache)
        )

    def try_import(self, mod, only_modules=False):
        mod = mod.rstrip('.')
        try:
            m = import_module(mod)
        except BaseException:
            return []

        filename = getattr(m, '__file__', '') or ''
        m_is_init = (
            '__init__' in filename or
            # this is special not verified Arcadia heuristics
            filename == mod
        )

        completions = []

        module_variables = dir(m) + getattr(m, '__all__', [])
        if (
            not hasattr(m, '__file__') or
            (not only_modules) or
            m_is_init
        ):
            completions.extend([
                attr for attr in module_variables
                if is_importable(m, attr, only_modules)
            ])

        if m_is_init:
            submodules = self.modules_cache.get(mod, [])
            completions.extend(submodules)

        return list(set(completions))

    def __call__(self, shell, event):
        self.late_init(shell)

        words = event.line.split(' ')
        nwords = len(words)

        # from whatever <tab> -> 'import '
        if nwords == 3 and words[0] == 'from':
            return ['import ']

        # 'from xy<tab>' or 'import xy<tab>'
        if nwords < 3 and (words[0] in {'%aimport', 'import', 'from'}):
            if nwords == 1:
                return self.root_modules
            mod = words[1].split('.')
            if len(mod) < 2:
                return self.root_modules
            completion_list = self.try_import('.'.join(mod[:-1]), True)
            return ['.'.join(mod[:-1] + [el]) for el in completion_list]

        # 'from xyz import abc<tab>'
        if nwords >= 3 and words[0] == 'from':
            mod = words[1]
            return self.try_import(mod)
