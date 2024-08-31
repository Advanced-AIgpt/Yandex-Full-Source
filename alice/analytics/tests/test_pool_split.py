#!/usr/bin/env python
# encoding: utf-8
from unittest import TestCase

from utils.toloka.split import split_tasks, join_tasks


class PoolTest(TestCase):


    def probe_split_join(self, src_records, trg_assigns, opts):
        tasks, additions = split_tasks(src_records, **opts)  # fields
        for n, t in enumerate(tasks):
            t['knownSolutions'] = None
            t['outputValues'] = {'out': n}
        assigns, _counters = join_tasks(tasks, additions, validation={})
        self.assertEqual(assigns, trg_assigns)

    REC1 = [
        {'inp': 'inp1', 'context': 'ctx1'},
        {'inp': 'inp2', 'context': 'ctx2'},
    ]

    ASSIGN1 = [
        {'inp': 'inp1', 'context': 'ctx1', 'out': 0},
        {'inp': 'inp2', 'context': 'ctx2', 'out': 1},
    ]

    REC2 = [
        {'inp': 'inp1', 'context': 'ctx1'},
        {'inp': 'inp1', 'context': 'ctx2'},
        {'inp': 'inp2', 'context': 'ctx3'},
    ]

    ASSIGN2 = [
        {'inp': 'inp1', 'context': 'ctx1', 'out': 0},
        {'inp': 'inp1', 'context': 'ctx2', 'out': 0},
        {'inp': 'inp2', 'context': 'ctx3', 'out': 1},
    ]

    def test_split_join(self):
        self.probe_split_join(self.REC1, self.ASSIGN1, {'fields': ['inp']})
        self.probe_split_join(self.REC2, self.ASSIGN2, {'fields': ['inp'], 'group_matching': True})

