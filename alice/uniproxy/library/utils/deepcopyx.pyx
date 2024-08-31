cdef dict __copiers = {}


cdef _copy_list(list l):
    ret = l.copy()
    for idx, item in enumerate(ret):
        cp = __copiers.get(type(item))
        if cp is not None:
            ret[idx] = cp(item)
    return ret


cdef _copy_dict(dict d):
    ret = d.copy()
    for key, value in ret.items():
        cp = __copiers.get(type(value))
        if cp is not None:
            ret[key] = cp(value)
    return ret


__copiers[list] = _copy_list
__copiers[dict] = _copy_dict


def deepcopy(x):
    cp = __copiers.get(type(x))
    if cp is None:
        return x
    else:
        return cp(x)
