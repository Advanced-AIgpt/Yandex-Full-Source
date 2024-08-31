#!/usr/bin/env python
# encoding: utf-8
import re
from os.path import join

from parse_conf import Conf
from parse_layout import LayoutConf
from tree import MergedIntentTree


class ProjectConf(Conf):
    CONF_TYPE = 'projects'

    _merged_tree = None  # Маркер отсутствия готового кэша

    def get_id(self):
        return self.get_conf()['prj_id']

    def toloka_env(self):
        conf_toloka_env = self.get_conf().get('toloka_env', None)
        if conf_toloka_env is None:
            if self.get_conf().get('is_sandbox', True):
                return 'sandbox'
            else:
                return 'prod'
        assert (conf_toloka_env in ['sandbox', 'prod', 'yang']), 'Invalid variable "toloka_env" value: %s' % conf_toloka_env
        return conf_toloka_env

    def get_owner(self):
        return self.get_conf()['owner']

    # Дерево интентов

    def get_raw_tree(self):
        return self.get_conf()['tree']

    def get_merged_tree(self):
        if self._merged_tree is None:
            self._merged_tree = MergedIntentTree(self)
        return self._merged_tree

    @staticmethod
    def is_layout(node):
        return isinstance(node, basestring) and node.strip().startswith('$')

    @staticmethod
    def layout_key(node):
        return node.strip().strip('$')

    def list_used_layouts(self):
        layouts = []
        def _traverse(tree):
            for key, sub in tree.iteritems():
                if self.is_layout(key):
                    layouts.append(self.layout_key(key))
                    continue

                if self.is_layout(sub):
                    layouts.append(self.layout_key(sub))
                    continue

                if isinstance(sub, dict):
                    _traverse(sub)

        _traverse(self.get_raw_tree())
        assert len(layouts) == len(set(layouts)), 'Included layouts must be unique'
        return map(LayoutConf, layouts)

    def dict_used_layouts(self):
        return {l.key: l for l in self.list_used_layouts()}

    def get_price_intervals(self):
        return self.get_conf().get('price_intervals')

    def get_visible_fields(self):
        return self.get_conf().get('visible_fields', ['dialog'])
