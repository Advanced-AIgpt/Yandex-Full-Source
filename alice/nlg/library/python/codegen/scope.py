# coding: utf-8

from __future__ import unicode_literals

from collections import OrderedDict, namedtuple


class ScopeError(Exception):
    pass


ScopedVariable = namedtuple('ScopedVariable', 'name, counter, param')


class LexicalScope(object):
    """Implements lexical scope.
    Disambiguates names using counters so that we don't have to rely
    on different languages' scoping rules, in particular on rules of shadowing.
    """
    def __init__(self, names=(), parent=None):
        self.parent = parent
        self.names = OrderedDict(names)  # to reduce dump-trans-ast variance

    def get_name_info(self, name):
        scope = self
        while scope:
            info = scope.names.get(name)
            if info is not None:
                return info
            scope = scope.parent
        return None

    def add_name(self, name, param):
        if name in self.names:
            # already in the current scope
            return

        counter = 0
        if self.parent is not None:
            prev = self.parent.get_name_info(name)
            if prev is not None:
                counter = prev.counter + 1

        self.names[name] = ScopedVariable(name, counter, param)

    def __repr__(self):
        return 'LexicalScope({!r})'.format(self.names)


class DynamicScope(object):
    """A scope that never remembers a single name.
    Used as a stub so that visit_Assign and the like don't create local variables
    at the top level.
    """
    def __init__(self):
        self.parent = None
        self.names = {}

    def get_name_info(self, name):
        return

    def add_name(self, name, param):
        return

    def __repr__(self):
        return 'DynamicScope()'
