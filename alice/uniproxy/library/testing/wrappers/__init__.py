from .vins_request import WrappedVinsRequest, WrappedVinsApplyRequest
from .vins import WrappedVoiceInput, WrappedTextInput


def reset_wrappers():
    WrappedVinsRequest.REQUESTS = []
    WrappedVinsApplyRequest.REQUESTS = []
    WrappedVoiceInput.PROCESSORS = []
    WrappedTextInput.PROCESSORS = []


__all__ = [
    WrappedVinsRequest,
    WrappedVinsApplyRequest,
    WrappedVoiceInput,
]
