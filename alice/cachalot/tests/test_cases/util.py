def get_client(cachalot, use_grpc=False):
    """ makes client if @cachalot is LocalCachalot,
        returns @cachalot otherwise.
    """
    if hasattr(cachalot, "get_sync_client"):
        return cachalot.get_sync_client(use_grpc=use_grpc)
    return cachalot


class ExceptionWithAdditionalMessage(Exception):
    def __init__(self, error_text, message=None):
        if message is not None:
            super().__init__(f"{error_text}. {message}")
        else:
            super().__init__(error_text)


class FalseException(ExceptionWithAdditionalMessage):
    def __init__(self, message):
        super().__init__("Value is not True", message)


class NotEqualException(ExceptionWithAdditionalMessage):
    def __init__(self, lhs, rhs, message):
        super().__init__(f"Values are not equal: {lhs} != {rhs}", message)


class GreaterException(ExceptionWithAdditionalMessage):
    def __init__(self, lhs, rhs, message):
        super().__init__(f"{lhs} > {rhs}", message)


class GreaterOrEqualException(ExceptionWithAdditionalMessage):
    def __init__(self, lhs, rhs, message):
        super().__init__(f"{lhs} >= {rhs}", message)


class WrongTypeException(ExceptionWithAdditionalMessage):
    def __init__(self, expected_type, wrong_type, message):
        super().__init__(f"Wrong type of value. Expected {expected_type}, but found {wrong_type}", message)


def assert_same_type(lhs, rhs, message=None):
    if not isinstance(lhs, type(rhs)):
        raise WrongTypeException(type(rhs), type(lhs), message)


def assert_eq(lhs, rhs, message=None):
    assert_same_type(lhs, rhs, message)
    if lhs != rhs:
        raise NotEqualException(lhs, rhs, message)


def assert_leq(lhs, rhs, message=None):
    assert_same_type(lhs, rhs, message)
    if lhs > rhs:
        raise GreaterException(lhs, rhs, message)


def assert_le(lhs, rhs, message=None):
    assert_same_type(lhs, rhs, message)
    if lhs >= rhs:
        raise GreaterOrEqualException(lhs, rhs, message)


def assert_true(value, message):
    if not value:
        raise FalseException(message)


class Scenario:
    def __init__(self, events):
        self.events = events

    def run(self, args1=tuple(), args2=tuple()):
        for callback, validator in self.events:
            validator(callback(*args1), *args2)


def _class_stable_permutations_impl(table, current_indexes, prefix):
    last_level = True
    for i in range(len(current_indexes)):
        if current_indexes[i] < len(table[i]):
            last_level = False
            new_indexes = current_indexes[:i] + [current_indexes[i] + 1] + current_indexes[i + 1:]
            for result in _class_stable_permutations_impl(table, new_indexes, prefix + [table[i][current_indexes[i]]]):
                yield result

    if last_level:
        yield prefix


def class_stable_permutations(objects):
    """
        Iterates over all permutations maintaining the relative order of records with equal class_id.
    """
    return _class_stable_permutations_impl(objects, [0 for _ in range(len(objects))], [])


assert len(list(class_stable_permutations([[10, 11], [23, 24, 25], [37]]))) == 60
