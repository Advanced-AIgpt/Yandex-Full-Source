import inspect

LAMBDA = lambda: 0


def is_fn_lambda(fn):
    global LAMBDA
    return isinstance(fn, type(LAMBDA)) and fn.__name__ == LAMBDA.__name__


def get_fn_text(fn):
    if is_fn_lambda(fn):
        raise ValueError('Lambdas are not supported')

    return inspect.getsource(fn)
