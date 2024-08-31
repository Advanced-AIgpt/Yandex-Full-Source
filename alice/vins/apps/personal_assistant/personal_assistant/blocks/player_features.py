# coding: utf-8

import attr

from .base import BaseBlock, register


@attr.s
class PlayerFeaturesBlock(BaseBlock):
    type = attr.ib()
    restore_player = attr.ib()
    last_play_timestamp = attr.ib()

    @classmethod
    def from_dict(cls, data):
        try:
            res = cls(
                type=data['type'],
                restore_player=bool(data.get('restore_player', False)),
                last_play_timestamp=int(data['last_play_timestamp']),
            )
            return res
        except KeyError, e:
            raise ValueError('Could not find field %s' % e)


register(PlayerFeaturesBlock, 'player_features')
