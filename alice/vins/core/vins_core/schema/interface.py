class PbSerializable(object):
    _PB_CLS = None

    def to_pb(self):
        raise NotImplementedError()

    @classmethod
    def from_pb(cls, pb_obj):
        raise NotImplementedError()

    def to_bytes(self):
        pb_obj = self.to_pb()
        return pb_obj.SerializeToString()

    @classmethod
    def from_bytes(cls, bytes):
        if cls._PB_CLS is None:
            raise NotImplementedError()

        pb_obj = cls._PB_CLS()
        pb_obj.ParseFromString(bytes)
        return cls.from_pb(pb_obj)
