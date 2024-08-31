from alice.uniproxy.library.utils.deepupdate import deepsetdefault


def test_deepsetdefault():
    x = {}

    for src in ({}, None, 123, "123"):
        deepsetdefault(x, src)
        assert x == {}

    deepsetdefault(x, {"a": 1, "b": {"b1": 1, "b2": 2}})
    assert x == {"a": 1, "b": {"b1": 1, "b2": 2}}

    deepsetdefault(x, {"x": {"y": "z"}})
    assert x == {"a": 1, "b": {"b1": 1, "b2": 2}, "x": {"y": "z"}}

    deepsetdefault(x, {"a": "AAA", "b": {"b3": 3}})
    assert x == {"a": 1, "b": {"b1": 1, "b2": 2, "b3": 3}, "x": {"y": "z"}}

    deepsetdefault(x, {"b": {"b3": "XXX"}, "c": {"c1": "CCC"}})
    assert x == {"a": 1, "b": {"b1": 1, "b2": 2, "b3": 3}, "c": {"c1": "CCC"}, "x": {"y": "z"}}
