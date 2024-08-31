import operator


def compare(lhs, rhs, func):
    for key, value in lhs.items():
        rhs_value = rhs.get(key)
        if value and rhs_value and not func(value, rhs_value):
            return False
    return True


class Version(object):
    def __init__(self, versions={}, **kwargs):
        self.__dict__.update(versions)
        self.__dict__.update(kwargs)

    def __repr__(self):
        return repr(self.__dict__)

    def __compare(self, other, func):
        other_dict = other if isinstance(other, dict) else other.__dict__
        return compare(self.__dict__, other_dict, func)

    def __lt__(self, other):
        return self.__compare(other, operator.lt)

    def __le__(self, other):
        return self.__compare(other, operator.le)

    def __eq__(self, other):
        return self.__compare(other, operator.eq)

    def __ge__(self, other):
        return self.__compare(other, operator.ge)

    def __gt__(self, other):
        return self.__compare(other, operator.gt)
