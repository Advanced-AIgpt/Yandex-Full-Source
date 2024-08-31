from alice.uniproxy.library.processors.vins import VoiceInput, TextInput


class WrappedVoiceInput(VoiceInput):
    PROCESSORS = []

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        WrappedVoiceInput.PROCESSORS.append(self)


class WrappedTextInput(TextInput):
    PROCESSORS = []

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        WrappedTextInput.PROCESSORS.append(self)
