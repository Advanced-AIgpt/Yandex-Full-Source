from functools import wraps

from . import div_card
from . import div2_card


def action(func):
    @wraps(func)
    def _wrapper(*args, **kwargs):
        return div2_card._DivAction(func(*args, **kwargs))
    return _wrapper


class DivWrapper(object):
    def __init__(self, data):
        assert data, 'Div card is absent. Expected not None, but {data}'
        self.data = data

    def __getattr__(self, attr):
        orig_attr = self.data.__getattribute__(attr)
        if callable(orig_attr):
            def _wrap(*args, **kwargs):
                return orig_attr(*args, **kwargs)
            return _wrap
        return orig_attr


class DivIterableWrapper(DivWrapper):
    def __init__(self, data, items=None):
        self._items = items or data
        super().__init__(data)

    def __iter__(self):
        for item in self._items:
            yield self._Item(item)

    def __len__(self):
        return len(self._items)

    def __getitem__(self, index):
        return self._Item(self._items[index])

    @property
    def first(self):
        return self._Item(self._items[0])


class DivSeparatorWrapper(DivWrapper):
    def __getitem__(self, index):
        for item in self.data:
            if item.type == 'div-separator-block':
                index -= 1
            elif index < 0:
                return item


div_card_types = ['div_card', 'div2_card']


def DivCard(card):
    if card.type == div_card_types[0]:
        return div_card.DivCard(card)
    if card.type == div_card_types[1]:
        return div2_card.DivCard(card)
