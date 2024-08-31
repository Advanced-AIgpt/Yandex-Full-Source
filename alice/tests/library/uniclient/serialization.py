import json


__all__ = ['serialize', 'deserialize']


class _ObjectWrapper(object):
    def __init__(self, d):
        self.__dict__.update(d)

    def __repr__(self):
        return repr(self.__dict__)

    def __str__(self):
        return str(self.__dict__)

    def __getitem__(self, attr):
        if hasattr(self, attr):
            return getattr(self, attr)
        raise KeyError(attr)

    def __contains__(self, attr):
        return hasattr(self, attr)

    def get(self, attr, default=None):
        return getattr(self, attr, default)

    def __iter__(self):
        return iter(self.__dict__.items())

    def dict(self):
        def _dict(o):
            if isinstance(o, (list, tuple)):
                return [_dict(_) for _ in o]
            return o.dict() if isinstance(o, _ObjectWrapper) else o

        return {_: _dict(o) for _, o in self}


class DefaultEncoder(json.JSONEncoder):
    def default(self, o):
        if hasattr(o, 'dict'):
            return o.dict()
        else:
            return o.__dict__


def serialize(message_obj):
    return json.dumps(message_obj, cls=DefaultEncoder)


def deserialize(message_json):
    if not message_json:
        return None
    return json.loads(message_json, object_hook=_ObjectWrapper)
