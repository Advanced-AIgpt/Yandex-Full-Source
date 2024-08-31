import pytest

import alice.hollywood.library.python.ammolib.parser as parser


@pytest.mark.parametrize("graphs_sources_param,expected", [
    ["FOO", {"": set(["FOO"])}],
    ["FOO.BAR", {"": set(["FOO", "BAR"])}],
    ["run:FOO.BAR", {"run": set(["FOO", "BAR"])}],
    ["run.apply:FOO.BAR", {"run": set(["FOO", "BAR"]), "apply": set(["FOO", "BAR"])}],
    ["run:FOO,apply:FOO.BAR", {"run": set(["FOO"]), "apply": set(["FOO", "BAR"])}],
])
def test_parse(graphs_sources_param, expected):
    result = parser.parse_graphs_sources_list(graphs_sources_param)
    assert expected == result


@pytest.mark.parametrize("filenames_prefix,graphs_sources_dict,expected", [
    ["prefix", {"run": set(["FOO"])}, [
        "prefix_run_FOO"
    ]],
    ["prefix", {"": set(["FOO"])}, [
        "prefix_FOO"
    ]],
    ["prefix", {"run": set(["FOO"]), "apply": set(["FOO", "BAR"])}, [
        "prefix_run_FOO",
        "prefix_apply_FOO",
        "prefix_apply_BAR",
    ]],
])
def test_make_output_filenames(filenames_prefix, graphs_sources_dict, expected):
    actual = parser.make_output_filenames(filenames_prefix, graphs_sources_dict)
    assert set(expected) == set(actual)
