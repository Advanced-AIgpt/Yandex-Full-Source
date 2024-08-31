# coding: utf-8

import attr

from .base import (
    BaseBlock,
    register,
)


@attr.s
class ScenarioDataBlock(BaseBlock):
    type = attr.ib()
    scenario_data = attr.ib()

    @classmethod
    def from_dict(cls, data):
        return cls(
            type=data['type'],
            scenario_data=data.get('data', {})
        )


register(ScenarioDataBlock, 'scenario_data')
