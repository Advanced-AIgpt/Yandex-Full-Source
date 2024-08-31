import google.protobuf.json_format as json_format


class ProtoWrapper(object):
    proto_cls = None

    def __init__(self, *args, **kwargs):
        assert not(args and kwargs), 'use only args or only kwargs to set proto data'
        self._o = kwargs or (args[0] if args else {})
        if not isinstance(self._o, self.proto_cls):
            self._o = json_format.ParseDict(self._o, self.proto_cls())

    def __bool__(self):
        return bool(self._o)

    def __eq__(self, value):
        return self._o == value

    def __ne__(self, value):
        return self._o != value

    def __setattr__(self, key, value):
        obj = super() if key.startswith('_') else self._o
        obj.__setattr__(key, value)

    def __getattr__(self, key):
        obj = super() if key.startswith('_') else self._o
        return obj.__getattribute__(key)

    def dict(self):
        return json_format.MessageToDict(self._o)
