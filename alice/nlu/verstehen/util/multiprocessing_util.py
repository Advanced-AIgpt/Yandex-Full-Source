import logging

from multiprocessing import Pool

logger = logging.getLogger(__name__)


class PickleableStaticMethod(object):
    """
    Class to use @staticmethod and @classmethod in multiprocessing.
    Otherwise, if passed in a row way, such methods will be unpickleable.
    """

    def __init__(self, fn, cls=None):
        self.cls = cls
        self.fn = fn

    def __call__(self, *args, **kwargs):
        return self.fn(*args, **kwargs)

    def __get__(self, obj, cls):
        return PickleableStaticMethod(self.fn, cls)

    def __getstate__(self):
        return self.cls, self.fn.__name__

    def __setstate__(self, state):
        self.cls, name = state
        self.fn = getattr(self.cls, name)


def mp_staticmethod(static_method, cls):
    """
    Util function to instantiate PickleableStaticMethod to utilise
    @staticmethod and @classmethod in multiprocessing since they are not
    pickleable in the raw form.
    """
    return PickleableStaticMethod(static_method, cls)


def mp_map(func, iterable_args, processes=20):
    if processes < 1 or not isinstance(processes, int):
        raise ValueError(
            'Number of processes should be a positive integer number, but was {}'.format(processes)
        )
    n_tasks = len(iterable_args)
    if n_tasks == 0:
        raise ValueError(
            'Cannot use map for empty iterable_args collection, at least on element should be present'
        )

    if processes > n_tasks:
        logger.debug('Number of processes ({}) is higher than number of tasks ({}), so number of processes '
                     'will be reduced to number of tasks ({})'.format(processes, n_tasks, n_tasks))
        processes = n_tasks

    if processes == 1:
        logger.debug('Number of processes is equal to 1, no multiprocessing is used, '
                     '{} tasks will be traversed by the function sequentially'.format(n_tasks))
        result = [func(arg) for arg in iterable_args]
    else:
        pool = Pool(processes=processes)
        try:
            logger.debug('Launching map for pool with {} processes, number of map tasks is {}'.format(
                processes, n_tasks
            ))
            result = pool.map_async(func, iterable_args).get(10e30)  # making not hanging on KeyboardInterrupt
        finally:
            pool.close()
            logger.debug('Pool map is finished, pool is closed')

    return result
