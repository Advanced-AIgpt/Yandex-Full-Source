# coding: utf-8
import attr

__blocks = {}


def register(cls, type_):
    __blocks[type_] = cls


def get_block(block_type):
    if block_type in __blocks:
        return __blocks[block_type]
    else:
        return UnknownBlock


def parse_block(data):
    if 'error' in data:
        cls = ErrorBlock
    else:
        cls = get_block(data['type'])

    return cls.from_dict(data)


class BaseBlock(object):
    @classmethod
    def from_dict(cls, data):
        raise NotImplementedError

    def to_dict(self):
        return attr.asdict(self)


class UnknownBlock(BaseBlock):
    def __init__(self, data):
        self._data = data
        if 'type' not in data:
            self.type = 'unknown'

    @classmethod
    def from_dict(cls, data):
        return cls(data)

    def to_dict(self):
        return self._data

    def __str__(self):
        return self.__class__.__name__ + '(' + str(self.to_dict()) + ')'

    def __repr__(self):
        return str(self)

    def __getattr__(self, name):
        if name in self._data:
            return self._data[name]
        else:
            return None


@attr.s
class ErrorBlock(BaseBlock):
    type = attr.ib()
    error_type = attr.ib()
    data = attr.ib()
    message = attr.ib()

    @classmethod
    def from_dict(cls, data):
        return cls(
            type=data['type'],
            error_type=data['error']['type'],
            data=data.get('data'),
            message=data['error']['msg'],
        )
