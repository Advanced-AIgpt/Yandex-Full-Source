from library.python import resource
from random import choice


BACKGROUND_AUDIO_DATAS = {
    key: val for key, val in resource.iteritems(prefix="/backgrounds/", strip_prefix=True)
}
BACKGROUND_AUDIO_KEYS = list(BACKGROUND_AUDIO_DATAS.keys())


def get_background_audio_data(key=None):
    if key is None:
        key = choice(BACKGROUND_AUDIO_KEYS)
    return BACKGROUND_AUDIO_DATAS.get(key, b"")
