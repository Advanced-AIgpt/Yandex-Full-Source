# coding: utf-8

import importlib
import sys


def find_module(name):
    for module_name in sys.extra_modules:
        if module_name.endswith('.' + name):
            return module_name
    return None


if __name__ == '__main__':
    if len(sys.argv) == 1:
        print>>sys.stderr, 'main module is not specified'
        sys.exit(1)

    module_name = find_module(sys.argv[1])

    if not module_name:
        print>>sys.stderr, 'can\'t find module', sys.argv[1]
        sys.exit(1)

    sys.argv = sys.argv[1:]
    importlib.import_module(module_name).main()
