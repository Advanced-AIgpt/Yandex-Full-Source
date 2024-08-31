from .speedups import pyx_websocket_mask
import json
import rapidjson


__all__ = ["pyx_websocket_mask"]


orig_dumps = json.dumps


def __dumps(*args, **kwargs):
    if "cls" in kwargs:
        return orig_dumps(*args, **kwargs)

    kwargs.pop("separators", None)
    return rapidjson.dumps(*args, **kwargs)


def apply():
    import tornado.util
    import json

    tornado.util._websocket_mask = pyx_websocket_mask
    # json.loads = rapidjson.loads
    json.dumps = __dumps
