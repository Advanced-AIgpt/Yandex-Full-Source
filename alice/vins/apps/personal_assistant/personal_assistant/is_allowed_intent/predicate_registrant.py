import inspect


class PredicateRegistrant(object):
    def __init__(self):
        self.predicates = dict()

    def __call__(self, f):
        if f.__name__ in self.predicates:
            raise ValueError('Predicate %s is registered already' % f.__name__)
        self.predicates[f.__name__] = {'func': f, 'params': inspect.getargspec(f)[0]}

        return f
