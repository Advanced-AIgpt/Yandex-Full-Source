from alice.boltalka.telegram_bot.lib.module import Module
from alice.boltalka.telegram_bot.lib.registered_modules import MODULES
import requests


MODULE_CACHE = {}


class Context:
    MAX_TURN = 100

    def __init__(self):
        self._context = []

    def push(self, turn):
        self._context.append(turn)
        if len(self._context) > self.MAX_TURN:
            del self._context[:-self.MAX_TURN]

    def get(self):
        return list(self._context)


class BoltalkaInstance(Module):
    class Options:
        use_variants = False
        variants_per_source = 1
        entity_id = 1

    def __init__(self, config):
        self.context = Context()
        self.module_options = {'global': config.get('global', {})}
        self.set_args(self.module_options['global'])
        self.module_options.update(config['modules'])
        self.modules = {'global': self}
        for name, options in config['modules'].items():
            module_type = options['type']
            if module_type not in MODULE_CACHE:
                MODULE_CACHE[module_type] = MODULES[module_type]()
            self.modules[name] = MODULE_CACHE[options['type']]
        self.replier_names = config['repliers']
        self.replier = config['replier']
        self.candidates = []
        self.wait_variant = None

    def get_options_recursive(self):
        return [
            '** {} **\n{}\n\n'.format(name, '\n'.join(['{}{} {} {}'.format('+ ' if modified else '', name, option, repr(value)) for option, value, modified in module.get_options(self.module_options[name])]))
            for name, module in self.modules.items()
        ]
    
    def substitute_args(self):
        global_options = self.module_options['global']
        return {
            section: {
                k: global_options[v[1:]] if isinstance(v, str) and v.startswith('$') else v
                for k, v in options.items()
            } for section, options in self.module_options.items()
        }

    def get_context_candidates(self, context):
        return self.modules[self.replier].get_replies(
                    self.module_options[self.replier],
                    context,
                    self.modules,
                    self.substitute_args()
                )
    
    def get_candidates(self):
        self.candidates = self.get_context_candidates(self.context.get())

    def select_movie(self, name):
        try:
            data = requests.get('http://entitysearch.yandex.net/search', params=dict(text=name)).json()
            print(data)
            data = data['base_info']
            movie_id = data['ids']['kinopoisk'].split('/')[-1]
            self.module_options['global']['entity_id'] = movie_id
            return movie_id, data['name']
        except KeyError:
            return None, None