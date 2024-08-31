import collections.abc
from .deepcopyx import deepcopy


# --------------------------------------------------------------------------------------------------------------------
def deepupdate(recipient, donor, copy=True):

    def _update(target, source):
        if not isinstance(target, collections.abc.Mapping):
            target = {}
        if isinstance(source, collections.abc.Mapping):
            for key, value in source.items():
                if isinstance(value, collections.abc.Mapping):
                    target[key] = _update(target.get(key, {}), value)
                else:
                    target[key] = deepcopy(value)
        return target

    if copy:
        result = deepcopy(recipient)
        return _update(result, donor)
    else:
        return _update(recipient, donor)


def deepsetdefault(target, source):
    if not (isinstance(source, collections.abc.Mapping) and isinstance(target, collections.abc.Mapping)):
        return
    for key, value in source.items():
        if isinstance(value, collections.abc.Mapping):
            deepsetdefault(target.setdefault(key, {}), value)
        else:
            target.setdefault(key, deepcopy(value))
