# coding: utf-8

from __future__ import unicode_literals

import pytest

from alice.nlg.library.python.codegen.transformer import CoverageSegmentsCollectorVisitor


class MockNode(object):
    def __init__(self, lines):
        self.lines = lines

    def __str__(self):
        return str(self.lines)


@pytest.mark.parametrize(
    'nodes, expected_segments, description',
    [
        ([MockNode([1, 2, 3, 4])], [(0, 4)], "Simplest case"),
        ([MockNode([1, 2, 3, 4]), MockNode([1, 2, 3, 4])], [(0, 4)], "Two duplicate segments"),
        ([MockNode([1, 2, 3, 4, 5]), MockNode([7, 8, 9, 10])], [(0, 5), (6, 10)], "Two segments with a gap"),
        ([MockNode([9, 8, 7, 10]), MockNode([2, 1, 5, 4, 3])], [(0, 5), (6, 10)], "Two unsorted segments with a gap"),
        ([MockNode([1, 2, 3, 4, 5, 7, 8, 9, 10])], [(0, 5), (6, 10)], "One segment with a gap"),
        ([MockNode([1, 2, 3, 4, 5]), MockNode([6, 7, 8, 9, 10])], [(0, 10)], "Two adjacent segments"),
        ([MockNode([1, 2, 3, 4, 5]), MockNode([3, 4, 5, 6, 7])], [(0, 7)], "Two intersecting segments"),
        ([MockNode([1])], [(0, 1)], "One one-line segment"),
        ([MockNode([])], [], "Edge case"),
    ]
)
def test_coverage_segments_collector_visitor(nodes, expected_segments, description):
    visitor = CoverageSegmentsCollectorVisitor()
    for node in nodes:
        visitor.generic_visit(node)
    actual_segments = visitor.segments()
    assert expected_segments == actual_segments, description + ", input nodes={}".format(list(map(lambda n: "node=" + str(n), nodes)))
