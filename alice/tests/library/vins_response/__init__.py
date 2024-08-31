from .div_card import action, DivWrapper, DivIterableWrapper, DivSeparatorWrapper
from .hollywood_response import HollywoodResponse
from .vins_response import VinsResponse

__all__ = [
    'action', 'DivWrapper', 'DivIterableWrapper', 'DivSeparatorWrapper',
    'HollywoodResponse', 'VinsResponse',
]


class NoneResponse(object):
    def __getattr__(self, attr):
        return None
