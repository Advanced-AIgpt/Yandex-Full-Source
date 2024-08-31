from alice.uniproxy.library.utils.deepcopyx import deepcopy
from alice.uniproxy.library.utils.tree import value_by_path


def _get_msg_type(msg):
    hdr = value_by_path(msg, ("directive", "header")) or value_by_path(msg, ("event", "header"))
    if hdr is None:
        return None
    return (hdr.get("namespace", "") + "." + hdr.get("name", "")).lower()


def filter_by_whitelist(src, whitelist):
    """
    :param src: dict to be filtered
    :param whitelist: dict contains allowed fields to be copied
    :returns: filtered deepcopy of source dict
    """
    res = {}
    for key, value in src.items():
        white_value = whitelist.get(key)
        if white_value is None:
            continue
        if isinstance(white_value, dict):
            if isinstance(value, dict):
                res[key] = filter_by_whitelist(value, white_value)
        else:
            res[key] = deepcopy(value)
    return res


def filter_by_blacklist(src, blacklist):
    """
    :param src: dict to be filtered
    :param blacklist: dict contains forbidden fields to be filtered out
    :returns: filtered deepcopy of source dict
    """
    res = {}
    for key, value in src.items():
        black_value = blacklist.get(key)
        if black_value is None:
            res[key] = deepcopy(value)
            continue
        if isinstance(black_value, dict) and isinstance(value, dict):
            res[key] = filter_by_blacklist(value, black_value)
    return res


class BlackFilter:
    def __init__(self, blacklist):
        self.blacklist = blacklist

    def filter(self, src):
        return filter_by_blacklist(src, self.blacklist)


class WhiteFilter:
    def __init__(self, whitelist):
        self.whitelist = whitelist

    def filter(self, src):
        return filter_by_whitelist(src, self.whitelist)


# -------------------------------------------------------------------------------------------------
class VinsSensitiveDataFilter:
    FILTERS_BY_TYPE = {
        # requests sent to vins (probably partials)
        "VinsRequest": WhiteFilter({
            "type": True,
            "ForEvent": True,
            "Body": {
                "application": True,
                "header": True,
                "request": {
                    "event": {
                        "biometry_scoring": True
                    },
                    "device_state": {
                        "device_id": True
                    }
                }
            }
        })
    }

    FITLERS_BY_NAME = {
        # directives sent to clients (complete vins response)
        "vins.vinsresponse": WhiteFilter({
            "directive": {
                "header": True,
                "payload": {
                    "header": True
                }
            }
        }),
        # directives used to create TTS processor
        "tts.generate": BlackFilter({
            "event": {
                "payload": {
                    "text": True
                }
            }
        })
    }

    @classmethod
    def filtered_copy(cls, src):
        f = None

        t = src.get("type")
        if t is not None:
            f = cls.FILTERS_BY_TYPE.get(t)
        else:
            n = _get_msg_type(src)
            if n is not None:
                f = cls.FITLERS_BY_NAME.get(n)

        if f is None:
            return deepcopy(src)  # no appropriate filter

        return f.filter(src)
