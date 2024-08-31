import os.path


def ya_tool():
    ya_fn = None
    try:
        ya_fn = ya_tool.fn
    except AttributeError:
        d = os.path.realpath(os.path.dirname(__file__))
        while d != '/' and not ya_fn:
            d = os.path.abspath(os.path.join(d, os.pardir))
            fn = os.path.join(d, 'ya')
            if os.path.exists(fn):
                ya_fn = fn
        ya_tool.fn = ya_fn

    return ya_tool.fn
