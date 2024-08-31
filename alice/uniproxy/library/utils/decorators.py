import functools


def ignore_exceptions(*exceptions):

    def decorator(func):

        @functools.wraps(func)
        def wrapped(*args, **kwargs):
            try:
                return func(*args, **kwargs)
            except Exception as exc:
                if isinstance(exc, tuple(exceptions)):
                    return None
                raise

        return wrapped

    return decorator
