# coding: utf-8
from collections import defaultdict

__states = {}


def register_state(name):
    def wrap(cls):
        __states[name] = cls
        return cls
    return wrap


def get_state_by_name(name):
    return __states[name]


class BaseState(object):
    def __init__(self, conf):
        self._setup(conf)

    def pop(self, chat_id, name, default=None):
        res = self._get(chat_id, name)
        if res is not None:
            self._delete(chat_id, name)
        else:
            res = default

        return res

    # methods to override
    def _setup(self, conf):
        raise NotImplementedError

    def set(self, chat_id, name, value):
        raise NotImplementedError

    def get(self, chat_id, name, default=None):
        raise NotImplementedError

    def delete(self, chat_id, name):
        raise NotImplementedError

    def get_chat_ids(self):
        raise NotImplementedError


class ChatStateWrapper(object):
    def __init__(self, chat_id, state_obj):
        self._state = state_obj
        self._chat_id = chat_id

    def set(self, *args, **kwargs):
        return self._state.set(self._chat_id, *args, **kwargs)

    def get(self, *args, **kwargs):
        return self._state.get(self._chat_id, *args, **kwargs)

    def delete(self, *args, **kwargs):
        return self._state.delete(self._chat_id, *args, **kwargs)

    def pop(self, *args, **kwargs):
        return self._state.pop(self._chat_id, *args, **kwargs)


@register_state('inmemory')
class InmemoryState(BaseState):
    def _setup(self, conf):
        self._storage = defaultdict(lambda: defaultdict(dict))

    def set(self, chat_id, name, value):
        self._storage[chat_id][name] = value

    def get(self, chat_id, name, default=None):
        if name in self._storage[chat_id]:
            return self._storage[chat_id][name]
        else:
            return default

    def delete(self, chat_id, name):
        self._storage[chat_id].pop(name, None)

    def get_chat_ids(self):
        return list(self._storage.keys())
