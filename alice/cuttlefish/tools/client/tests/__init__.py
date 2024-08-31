import functools
import logging
import asyncio


TESTS = {}
SECRET = {}


def register_test(func):
    prefix_len = len(register_test.__module__) + 1
    name = f"{func.__module__[prefix_len:]}.{func.__name__}"

    if name in TESTS:
        raise RuntimeError(f"Test function named '{name}' already exists")

    @functools.wraps(func)
    def wrap(*args, **kwargs):
        logging.info(f"Run '{name}' test...")
        try:
            asyncio.run(func(*args, **kwargs))
            return True
        except:
            logging.exception(f"Test '{name}' failed with exception")
            return False

    TESTS[name] = wrap

    return wrap


if "__path__" in locals():
    import pkgutil
    import importlib

    # import all direct submodules to register tests
    for _, module_name, ispkg in pkgutil.iter_modules(__path__):
        if not ispkg:
            module = importlib.import_module("." + module_name, __package__)
