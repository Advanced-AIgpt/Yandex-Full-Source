import collections.abc


def value_by_path(d, path):
    try:
        for i in path:
            d = d[i]
        return d
    except:
        return None


def dict_at_path(d, path):
    val = value_by_path(d, path)

    if not isinstance(val, collections.abc.Mapping):
        return None

    return val


def replace_tag_value(d, tag, repl):
    if isinstance(d, list):
        for l in d:
            replace_tag_value(l, tag, repl)
    elif isinstance(d, dict):
        for k, v in d.items():
            if k == tag and type(repl) == type(v):
                d[k] = repl
            else:
                replace_tag_value(v, tag, repl)
