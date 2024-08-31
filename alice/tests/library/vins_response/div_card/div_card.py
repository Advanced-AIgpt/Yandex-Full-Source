from cached_property import cached_property

from .action import _ActionClickMixin


class _DivBlock(object):
    _type = None

    def __init__(self, o):
        if hasattr(o, 'type'):
            assert o.type == self._type, f'expect {o.type}, but {self._type}'
        self._o = o

    @property
    def type(self):
        return self._o.type


class _Iterable(object):
    def __init__(self, items, *cls, default_cls=None):
        self._items = items
        self._cls = {_._type: _ for _ in cls}
        self._default_type = default_cls._type if default_cls else None

    def __len__(self):
        return len(self._items)

    def __iter__(self):
        for _ in self._items:
            yield self._make(_)

    def _make(self, o):
        cls_type = o.get('type', self._default_type)
        cls = self._cls.get(cls_type)
        return cls(o) if cls else o

    def __getitem__(self, index):
        return self._make(self._items[index])

    @property
    def first(self):
        return self[0] if self._items else None


class _IterableDivBlock(_DivBlock, _Iterable):
    def __init__(self, o, items, *cls, **kcls):
        _DivBlock.__init__(self, o)
        _Iterable.__init__(self, items, *cls, **kcls)


class _DivAction(_ActionClickMixin):
    @cached_property
    def action_url(self):
        return self._o.get('action', {}).get('url')

    @cached_property
    def log_id(self):
        return self._o.get('action', {}).get('log_id')


class DivImage(_DivBlock, _DivAction):
    _type = 'div-image-block'

    @property
    def image_url(self):
        return self._o.image.image_url


class DivButton(_DivBlock, _DivAction):
    _type = 'div-button-block'

    @property
    def text(self):
        return self._o.text


class DivUniversal(_DivBlock, _DivAction):
    _type = 'div-universal-block'

    @property
    def title(self):
        return self._o.title

    @property
    def text(self):
        return self._o.text

    @property
    def side_element(self):
        return self._o.side_element.element


class DivButtons(_IterableDivBlock):
    _type = 'div-buttons-block'

    def __init__(self, o):
        super().__init__(o, o.items, DivButton, default_cls=DivButton)

    def __call__(self, title):
        for b in self:
            if title in b.text:
                return b


class DivTable(_IterableDivBlock, _DivAction):
    _type = 'div-table-block'

    class _Cell(DivImage):
        _type = 'cell_element'

        @property
        def text(self):
            return self._o.text

    class _Row(_IterableDivBlock):
        _type = 'row_element'

        def __init__(self, o):
            super().__init__(o, o.cells, DivTable._Cell, default_cls=DivTable._Cell)

    def __init__(self, o):
        super().__init__(o, o.rows, DivTable._Row)

    @property
    def first(self):
        return super().first.first


class DivFooter(DivImage):
    _type = 'div-footer-block'

    @property
    def text(self):
        return self._o.text

    @property
    def action(self):
        return self._o.action


class DivContainer(_IterableDivBlock, _DivAction):
    _type = 'div-container-block'

    def __init__(self, o):
        super().__init__(o, o.children, DivContainer, DivButtons, DivFooter, DivGallery, DivImage, DivTable, DivUniversal)
        for _ in o.children:
            setattr(self, _.type.split('-')[1], self._make(_))

    @property
    def raw(self):
        return self._o


class DivGallery(_IterableDivBlock):
    _type = 'div-gallery-block'

    class _Tail(_DivBlock, _DivAction):
        @property
        def text(self):
            return self._o.get('text')

        @property
        def icon_url(self):
            return self._o.icon.get('image_url')

    def __init__(self, o):
        super().__init__(o, o.items, DivContainer)

    @cached_property
    def tail(self):
        if 'tail' in self._o:
            return DivGallery._Tail(self._o['tail'])


class DivTabs(_IterableDivBlock):
    _type = 'div-tabs-block'

    class _Item(_DivBlock, _DivAction):
        @cached_property
        def content(self):
            return DivContainer(self._o.content)

        @property
        def title(self):
            return self._o.title.text

    def __init__(self, o):
        super().__init__(o, o.items, DivTabs._Item, default_cls=DivTabs._Item)


class DivCard(_Iterable, _DivAction):
    def __init__(self, card):
        self._card = card
        self._o = self._card.body.states[0]
        super().__init__(self._o.blocks, DivContainer, DivButtons, DivFooter, DivGallery, DivImage, DivTable, DivTabs, DivUniversal)
        for _ in self._o.blocks:
            setattr(self, _.type.split('-')[1], self._make(_))

    @property
    def raw(self):
        return self._o
