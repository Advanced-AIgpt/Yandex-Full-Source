class MockAny(object):
    def __eq__(self, other):
        return True

    def __repr__(self):
        return '<mock.ANY>'


ANY = MockAny()


def process_mock_any(d):
    if isinstance(d, dict):
        res = dict()
        for k, v in d.iteritems():
            res[k] = process_mock_any(v)
        return res
    elif isinstance(d, list):
        return [process_mock_any(e) for e in d]
    elif d == '{any}':
        return ANY
    else:
        return d


def filter_dict(d, include=None, exclude=None):
    return {
        key: value
        for key, value in d.iteritems()
        if (include is None or key in include) and (exclude is None or key not in exclude)
    }


def map_to_lists_in_dict(func, d):
    return {
        key: [func(e) for e in value]
        for key, value in d.iteritems()
    }


def unicode_to_str(obj):
    if isinstance(obj, dict):
        return {
            unicode_to_str(key): unicode_to_str(value) for key, value in obj.iteritems()
        }
    elif isinstance(obj, list):
        return [unicode_to_str(element) for element in obj]
    elif isinstance(obj, tuple):
        return tuple([unicode_to_str(element) for element in obj])
    elif isinstance(obj, unicode):
        return obj.encode("utf-8")
    else:
        return obj


def tuple_to_list(obj):
    if isinstance(obj, dict):
        res = dict()
        for k, v in obj.iteritems():
            res[k] = tuple_to_list(v)
        return res
    elif isinstance(obj, tuple) or isinstance(obj, list):
        return [tuple_to_list(e) for e in obj]
    else:
        return obj


def sorted_values(d, sorted_fields):
    res = d.copy()
    for sorted_field in sorted_fields:
        if sorted_field in d:
            res[sorted_field] = list(sorted(d[sorted_field]))
    return res
