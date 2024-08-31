from copy import copy


class Module:
    class Options:
        pass

    def set_args(self, args):
        for option, value in vars(type(self).Options).items():
            if option[0] == '_':
                continue
            if option in args:
                value = args[option]
            else:
                value = copy(value)
            setattr(self, option, value)

    def get_options(self, args):
        return [
            (option, args.get(option, value), option in args)
            for option, value in vars(type(self).Options).items() if option[0] != '_'
        ]

    def set_option(self, args, option, value):
        if option not in vars(type(self).Options):
            raise KeyError
        args[option] = value
        self.set_args(args)
    
    @classmethod
    def add_to_parser(cls, parser):
        for option, value in vars(cls.Options).items():
            name = '--' + option.replace('_', '-')
            parser.add_argument(name, default=value)

def get_module_dict(modules):
    return {module.__name__: module for module in modules}
