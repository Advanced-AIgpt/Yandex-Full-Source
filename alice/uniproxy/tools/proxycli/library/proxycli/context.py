import weakref
import collections


class Context:
    pass


class Context(object):
    def __init__(self, name: str = None, tag: str = None, data: dict = {}, parent: Context = None):
        super().__init__()
        self.__ctx_tag__ = tag
        self.__ctx_name__ = name if name else 'root'
        self.__ctx_parent__ = weakref.proxy(parent) if parent else None
        self.__ctx_children__ = collections.defaultdict(list)

        for key, value in data.items():
            setattr(self, key, value)

    def __str__(self) -> str:
        return self.__ctx_name__

    def __getattr__(self, key):
        if self.__ctx_parent__ is not None:
            return getattr(self.__ctx_parent__, key)
        return None

    def __delattr__(self, key):
        if key in self.__dict__:
            super().__delattr__(key)

    def __call__(self, prefix: str, data: dict = {}, tag=None):
        """ creates child context """
        index = len(self.__ctx_children__[prefix])
        tag = tag if tag else prefix
        ctx = Context(f'{self.__ctx_name__}.{prefix}.{index}', tag=tag, data=data, parent=self)
        self.__ctx_children__[prefix].append(ctx)
        return ctx

    def __getitem__(self, tag):
        """ gets one of the parent tagged contexts """
        ctx = self
        while ctx and ctx.__ctx_tag__ != tag:
            ctx = ctx.__ctx_parent__
        return ctx

    def get(self, key, default=None):
        return self.__dict__.get(
            key,
            self.__ctx_parent__.get(key, default) if self.__ctx_parent__ else default
        )

    def get_current(self, key, default=None):
        return self.__dict__.get(key, default)

    def to_dict(self):
        a = {
            'name': self.__ctx_name__,
            'tag': self.__ctx_tag__,
            'children': {
                k: [
                    x.to_dict() for x in v
                ] for k, v in self.__ctx_children__.items()
            },
            'data': {
                k: str(v) for k, v in self.__dict__.items() if not k.startswith('__')
            }
        }
        return a

    def itertags(self, tag, depth=0):
        if self.__ctx_tag__ == tag:
            yield self.__ctx_name__, self, depth

        for key, items in self.__ctx_children__.items():
            for item in items:
                yield from item.itertags(tag, depth + 1)

    def iterprefix(self, prefix):
        for k, items in self.__ctx_children__.items():
            if k == prefix:
                for item in items:
                    yield item.__ctx_name__, item
                    yield from item.iterprefix(prefix)
            else:
                for item in items:
                    yield from item.iterprefix(prefix)

    def iterdata(self):
        yield from {
            k: str(v) for k, v in self.__dict__.items() if not k.startswith('__')
        }.items()

    def set_tag(self, tag):
        self.__ctx_tag__ = tag
