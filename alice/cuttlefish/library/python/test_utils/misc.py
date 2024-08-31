import asyncio
import logging
from copy import deepcopy
import functools
from contextlib import asynccontextmanager

from library.python import sanitizers


_logger = logging.getLogger("test_utils")
_logger.setLevel(logging.WARNING)


def deepupdate(target, *updates, copy=False):
    if copy:
        target = deepcopy(target)

    for update in updates:
        for key, value in update.items():
            if isinstance(value, dict) and isinstance(target.get(key), dict):
                deepupdate(target[key], value)
            else:
                target[key] = value

    return target


def get_at(x, *path, default=None):
    try:
        for p in path:
            x = x[p]
        return x
    except KeyError:
        return default


def autowait(func):
    """Decorator - turns async function into a task running within asynchronous context manager
    The task will be awaited when returned context is exited.
    """

    @functools.wraps(func)
    @asynccontextmanager
    async def wrapper(*args, **kwargs):
        task = asyncio.create_task(func(*args, **kwargs))
        try:
            yield
        finally:
            await task

    return wrapper


def autocancel(func):
    """Decorator - turns async function into a task running within asynchronous context manager.
    The task will be cancelled when returned context is exited.
    """

    @functools.wraps(func)
    @asynccontextmanager
    async def wrapper(*args, **kwargs):
        task = asyncio.create_task(func(*args, **kwargs))
        try:
            yield task
        finally:
            task.cancel()
            _logger.debug(f"autocancel: {task} is cancelled")
            try:
                await task
            except asyncio.CancelledError:
                pass
            finally:
                _logger.debug(f"autocancel: {task} is finished")

    return wrapper


@asynccontextmanager
async def many(_, context_manager, *args, **kwargs):
    """Run given number of async context managers"""

    if not isinstance(_, int) or _ == 0:
        raise RuntimeError("count must be integer >0")

    try:
        async with context_manager(*args, **kwargs) as ctx:
            try:
                if _ == 1:
                    yield [ctx]
                else:
                    async with many(_ - 1, context_manager, *args, **kwargs) as rest:
                        yield [ctx] + rest
            except Exception as exc:
                _logger.debug(
                    f"many async contexts: #{_} is to be exited due to exception in outer scope {type(exc)}: {exc}"
                )
            else:
                _logger.debug(f"many async contexts: #{_} is to be exited normally")
    except Exception as exc:
        _logger.debug(f"many async contexts: #{_} rose exception {type(exc)}: {exc}")
    else:
        _logger.debug(f"many async contexts: #{_} is exited normally")


def unistat_diff(a, b):
    diff = {}
    for k, bv in b.items():
        diff[k] = bv

        av = a.get(k)

        if av is None:
            continue

        if isinstance(bv, (int, float)):
            assert isinstance(av, (int, float))
            diff[k] -= av
            continue

        if isinstance(bv, list):
            assert isinstance(av, (int, list))
            assert len(av) == len(bv)
            for i in range(len(av)):
                diff[k][i][1] -= av[i][1]
            continue

        raise RuntimeError(f"unknown metric value: {bv}")

    return diff


def fix_timeout_for_sanitizer(timeout):
    multiplier = 2
    if sanitizers.asan_is_on():
        # https://github.com/google/sanitizers/wiki/AddressSanitizer
        multiplier = 6
    elif sanitizers.msan_is_on():
        # via https://github.com/google/sanitizers/wiki/MemorySanitizer
        multiplier = 5
    elif sanitizers.tsan_is_on():
        # via https://clang.llvm.org/docs/ThreadSanitizer.html
        multiplier = 15

    return timeout * multiplier
