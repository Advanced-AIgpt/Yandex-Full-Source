from .deepupdate import deepupdate, deepsetdefault
from .srcrwr import Srcrwr
from .graph_overrides import GraphOverrides
from .experiments import conducting_experiment, experiment_value, mm_experiment_value
from .decorators import ignore_exceptions
from contextlib import contextmanager


__all__ = ['deepupdate', 'deepsetdefault', 'Srcrwr', 'conducting_experiment', 'experiment_value', 'ignore_exceptions', 'mm_experiment_value', 'GraphOverrides']


@contextmanager
def rtlog_child_activation(rtlog, name, finish=True):
    # if not `finish` than activation won't be finished on normal exit from the context

    if rtlog is None:
        yield None
        return

    token = rtlog.log_child_activation_started(name)
    try:
        yield token
        if finish:
            rtlog.log_child_activation_finished(token, True)
    except:
        rtlog.log_child_activation_finished(token, False)
        raise
