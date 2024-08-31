
class DictObj:
    def __init__(self, d):
        self.__data__ = d

    def __getattr__(self, key):
        if self.__data__:
            result = self.__data__.get(key)
            if isinstance(result, dict):
                return DictObj(result)
            else:
                return result
        return None
