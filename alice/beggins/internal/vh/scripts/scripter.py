import inspect
import types
import typing
import textwrap


def get_function_body(function: types.FunctionType) -> str:
    lines = inspect.getsourcelines(function)[0]
    return textwrap.dedent(''.join(lines[1:]))


class Scripter:
    """
    String representation for Nirvana cubes
    There is support:
        1. Python file
        2. Python function body

    Also, you have opportunity to avoid functions and classes defining within function
    Just define them outside function, and they will be substituted automatically
    """
    @staticmethod
    def to_string(source: typing.Union[types.ModuleType, types.FunctionType]) -> str:
        if inspect.ismodule(source):
            return inspect.getsource(source)
        elif inspect.isfunction(source):
            function = source
            module = inspect.getmodule(function)
            if not inspect.ismodule(module):
                raise Exception('The specified source location is not within module')

            args = inspect.getargs(function.__code__).args
            args_number = len(args)
            if args_number > 3:
                raise Exception('Python3 any* to any* cubes do not support functions with more than 3 args')

            args_names = ['v', 'w', 'x']
            args_names_for_using = args_names[:args_number]
            args_part = f'({", ".join(args_names_for_using)})'

            return f'{inspect.getsource(module)}\n\nreturn {function.__name__}{args_part}\n'
