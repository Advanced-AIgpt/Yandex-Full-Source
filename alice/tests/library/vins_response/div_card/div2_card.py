from .action import _ActionClickMixin


class _DivAction(_ActionClickMixin):
    def __init__(self, action):
        self.action_url = action


class DivCard(object):
    def __init__(self, card):
        self._card = card
        self._o = self._card.body.states[0]
        setattr(self, self._o.div.type, self._o.div)

    @property
    def has_borders(self):
        return self._card.has_borders

    @property
    def log_id(self):
        return self._card.body.log_id

    @property
    def type(self):
        return self._o.div.type

    @property
    def raw(self):
        return self._o
