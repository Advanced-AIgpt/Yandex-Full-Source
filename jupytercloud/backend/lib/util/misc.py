import asyncio
from copy import deepcopy
from itertools import zip_longest
from pathlib import Path

from traitlets import Instance
from traitlets.config import LoggingConfigurable
from yarl import URL


class OAuthToken(Instance):
    klass = str
    default_value = ''

    def __init__(self, kw=None, **kwargs):
        if kw is None:
            kw = {}

        super().__init__(kw=kw, **kwargs)

    def validate(self, obj, value):
        if isinstance(value, str):
            return value
        self.error(obj, value)

    def get_headers(self):
        return {
            'Authorization': f'OAuth {self.value}'
        }


class Url(Instance):
    klass = URL

    default_value = URL('')
    info_text = 'URL to anything'

    def __init__(self, kw=None, **kwargs):
        if kw is None:
            kw = {}

        super().__init__(kw=kw, **kwargs)

    def validate(self, obj, value):
        if isinstance(value, URL):
            return value
        elif isinstance(value, str):
            return URL(value)
        self.error(obj, value)


class HasTraitsReprMixin:
    def __repr__(self):
        # why traitlets does not have nice default repr mechanism?...

        trait_values = self.get_trait_values()
        if 'config' in trait_values:
            trait_values['config'] = '<secret>'

        items = ('='.join((k, repr(v))) for k, v in trait_values.items())

        return '{}({})'.format(self.__class__.__name__, ', '.join(items))

    def get_trait_values(self):
        return {name: trait.get(self) for name, trait in self.traits().items()}

    def get_safe_trait_values(self):
        result = self.get_trait_values()
        for trait in LoggingConfigurable.class_traits():
            result.pop(trait, None)

        for trait, trait_type in self.class_traits().items():
            if isinstance(trait_type, Instance):
                result.pop(trait, None)

        return result

    def update(self, **kwargs):
        new = deepcopy(self)
        for k, v in kwargs.items():
            setattr(new, k, v)

        return new

    def as_dict(self):
        result = self.get_trait_values()
        for trait in LoggingConfigurable.class_traits():
            result.pop(trait, None)

        for trait, value in result.items():
            if isinstance(value, HasTraitsReprMixin):
                result[trait] = value.as_dict()

            elif isinstance(value, dict):
                result[trait] = {
                    k: v.as_dict() if isinstance(v, HasTraitsReprMixin) else v
                    for k, v in value.items()
                }

            elif isinstance(value, (list, tuple)):
                result[trait] = [
                    v.as_dict() if isinstance(v, HasTraitsReprMixin) else v
                    for v in value
                ]

        return result


def grouper(iterable, n, fillvalue=None):
    """Collect data into fixed-length chunks or blocks.

    grouper('ABCDEFG', 3, 'x') --> ABC DEF Gxx
    """
    args = [iter(iterable)] * n
    return (filter(None, _) for _ in zip_longest(*args, fillvalue=fillvalue))


async def cancel_task_safe(task):
    # NB: Таким образом таски отменяются в коде Jupyterhub
    # с комментариям про пущую помощь garbage collector-у

    task.cancel()
    try:
        await task
    except asyncio.CancelledError:
        pass


class cached_class_property:
    def __init__(self, func):
        self.func = func

    def __get__(self, obj, cls):
        if obj is None:
            return self

        cache_name = self.__class__.__name__
        if not hasattr(cls, cache_name):
            setattr(cls, cache_name, {})

        cache = getattr(cls, cache_name)

        cache_key = '__' + cls.__name__ + self.func.__name__
        if cache_key not in cache:
            cache[cache_key] = self.func(obj)

        return cache[cache_key]


class MessageList(list):
    """
    Специальный список, в который пушатся некие события.

    Особенность в том, что при итерации по нему, итерирование идет
    с начала, и асинхронно повисает на ожидании следующего события.

    Это нужно, чтобы при обновлении страницы с лентой событий
    вычитывания списка началось с начала.

    Обязательно нужно закрыть список через .close, чтобы
    те, кто висит на асинхронной итерации, перестали бы на ней висеть.

    """

    def __init__(self):
        self._list = []
        self._waiters = []
        self._closed = False

    def close(self):
        self._closed = True

        for waiter in self._waiters:
            waiter.set_result(None)

    def append(self, value):
        self._list.append(value)

        for waiter in self._waiters:
            if waiter.done():
                waiter.result().append(value)
            else:
                waiter.set_result([value])

    async def __aiter__(self):
        for value in self._list:
            yield value

        while not self._closed:
            waiter = asyncio.Future()
            self._waiters.append(waiter)

            try:
                res = await waiter
                if res is None:
                    break

                for _ in res:
                    yield _

            except asyncio.CancelledError:
                waiter.cancel()
                break
            except:
                waiter.cancel()
                raise
            finally:
                self._waiters.remove(waiter)


def get_arcadia_root():
    path = Path.cwd()

    while path.name:
        if (path / '.arcadia.root').exists():
            return path

        path = path.parent

    raise RuntimeError('failed to find arcadia root')
